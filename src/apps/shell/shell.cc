/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of SemperOS.
 *
 * SemperOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * SemperOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <base/stream/IStringStream.h>

#include <m3/stream/Standard.h>
#include <m3/pipe/IndirectPipe.h>
#include <m3/vfs/Dir.h>
#include <m3/vfs/Executable.h>
#include <m3/vfs/VFS.h>
#include <m3/VPE.h>

#include "Args.h"
#include "Parser.h"

using namespace m3;

static struct {
    const char *name;
    PEDesc pe;
} petypes[] = {
    /* COMP_IMEM */ {"imem", PEDesc(PEType::COMP_IMEM)},
    /* COMP_EMEM */ {"emem", PEDesc(PEType::COMP_EMEM)},
    /* MEM       */ {"mem",  PEDesc(PEType::MEM)},
};

static PEDesc get_pe_type(const char *name) {
    for(size_t i = 0; i < ARRAY_SIZE(petypes); ++i) {
        if(strcmp(name, petypes[i].name) == 0)
            return petypes[i].pe;
    }
    return VPE::self().pe();
}

static bool execute(CmdList *list) {
    VPE *vpes[MAX_CMDS] = {nullptr};
    IndirectPipe *pipes[MAX_CMDS] = {nullptr};
    Executable *exec[MAX_CMDS] = {nullptr};

    for(size_t i = 0; i < list->count; ++i) {
        Command *cmd = list->cmds[i];

        PEDesc pe = VPE::self().pe();
        for(size_t i = 0; i < cmd->vars->count; ++i) {
            if(strcmp(cmd->vars->vars[i].name, "PE") == 0) {
                pe = get_pe_type(cmd->vars->vars[i].value);
                break;
            }
        }

        vpes[i] = new VPE(cmd->args->args[0], pe);
        if(Errors::last != Errors::NO_ERROR) {
            errmsg("Unable to create VPE for " << cmd->args->args[0]);
            break;
        }

        // TODO support I/O redirection
        if(i > 0)
            vpes[i]->fds()->set(STDIN_FD, VPE::self().fds()->get(pipes[i - 1]->reader_fd()));
        else
            vpes[i]->fds()->set(STDIN_FD, VPE::self().fds()->get(STDIN_FD));

        if(i + 1 < list->count) {
            pipes[i] = new IndirectPipe(64 * 1024);
            vpes[i]->fds()->set(STDOUT_FD, VPE::self().fds()->get(pipes[i]->writer_fd()));
        }
        else
            vpes[i]->fds()->set(STDOUT_FD, VPE::self().fds()->get(STDOUT_FD));

        vpes[i]->fds()->set(STDERR_FD, VPE::self().fds()->get(STDERR_FD));
        vpes[i]->obtain_fds();

        vpes[i]->mountspace(*VPE::self().mountspace());
        vpes[i]->obtain_mountspace();

        Errors::Code err;
        exec[i] = new Executable(cmd->args->count, cmd->args->args);
        if((err = vpes[i]->exec(*exec[i])) != Errors::NO_ERROR) {
            errmsg("Unable to execute '" << cmd->args->args[0] << "'");
            break;
        }

        if(i > 0) {
            pipes[i - 1]->close_reader();
            pipes[i - 1]->close_writer();
        }
    }

    for(size_t i = 0; i < list->count; ++i) {
        if(vpes[i]) {
            int res = vpes[i]->wait();
            if(res != 0)
                cerr << "Program terminated with exit code " << res << "\n";
            delete vpes[i];
            delete exec[i];
        }
    }
    for(size_t i = 0; i < list->count; ++i)
        delete pipes[i];
    return true;
}

int main() {
    if(VFS::mount("/", new M3FS("m3fs")) != Errors::NO_ERROR) {
        if(Errors::last != Errors::EXISTS)
            exitmsg("Unable to mount filesystem\n");
    }

    cout << "========================\n";
    cout << "Welcome to the M3 shell!\n";
    cout << "========================\n";
    cout << "\n";

    while(!cin.eof()) {
        cout << "$ ";
        cout.flush();

        // String input("echo < foo");
        // IStringStream is(input);
        // CmdList *list = get_command(&is);

        CmdList *list = get_command(&cin);
        if(!list)
            continue;

        // extract core type
        // String core;
        // if(strncmp(args[0], "CORE=", 5) == 0) {
        //     core = args[0] + 5;
        //     args++;
        //     argc--;
        // }

        for(size_t i = 0; i < list->count; ++i) {
            Args::prefix_path(list->cmds[i]->args);
            Args::expand(list->cmds[i]->args);
        }

        if(!execute(list))
            break;

        ast_cmds_destroy(list);
    }
    return 0;
}
