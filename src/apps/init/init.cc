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

#include <m3/session/M3FS.h>
#include <m3/stream/Standard.h>
#include <m3/vfs/VFS.h>
#include <m3/vfs/Executable.h>
#include <m3/Syscalls.h>
#include <m3/VPE.h>

using namespace m3;

int main(int argc, const char **argv) {
    if(argc < 2)
        exitmsg("Usage: " << argv[0] << " <program> [<arg>...]");

    if(VFS::mount("/", new M3FS("m3fs")) < 0) {
        if(Errors::last != Errors::EXISTS)
            exitmsg("Mounting root-fs failed");
    }

    VPE sh(argv[1], VPE::self().pe(), "pager");

    sh.mountspace(*VPE::self().mountspace());
    sh.obtain_mountspace();

    Executable exec(argc - 1, argv + 1);
    sh.exec(exec);

    sh.wait();
    return 0;
}

