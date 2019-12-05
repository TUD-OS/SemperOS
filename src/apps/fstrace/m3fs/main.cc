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

#include <base/Common.h>

#include <m3/session/M3FS.h>
#include <m3/stream/Standard.h>
#include <base/stream/IStringStream.h>
#include <m3/vfs/VFS.h>

#include <cstring>

#include "common/traceplayer.h"
#include "platform.h"

using namespace m3;

int main(int argc, char **argv) {
    Platform::init(argc, argv);

    m3::String fssrv("m3fs");
    if(argc > 1)
        fssrv = IStringStream::read_from<m3::String>(argv[1]);
    VFS::mount("/", new M3FS(fssrv));

    // defaults
    long num_iterations = 1;
    bool keep_time   = true;
    bool make_ckpt   = false;
    bool warmup      = false;
    unsigned int rounds = 1;

    // playback / revert to init-trace contents
    const char *prefix = "";
    if(argc > 2) {
        prefix = argv[2];
        char *tmpfolder = new char[strlen(prefix) + 5];
        strcpy(tmpfolder, prefix);
        strcat(tmpfolder, "/tmp");
        VFS::mkdir(prefix, 0755);
        VFS::mkdir(tmpfolder, 0755);
    }
    TracePlayer player(prefix);

    // print parameters for reference
    cout << "VPFS trace_bench started ["
         << "prefix=" << prefix << " ,"
         << "service=" << fssrv << ","
         << "n=" << num_iterations << ","
         << "keeptime=" << (keep_time   ? "yes," : "no,")
         << "warmup=" << (warmup ? "yes," : "no,")
         << "rounds=" << rounds
         << "]\n";

    // identify for runtime extraction script
    m3::DTU::get().debug_msg(0x13000000);
    player.play(keep_time, make_ckpt, warmup);

    cout << "VPFS trace_bench benchmark terminated\n";

    // done
    Platform::shutdown();
    return 0;
}
