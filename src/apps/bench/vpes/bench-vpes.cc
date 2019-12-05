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
#include <base/stream/Serial.h>
#include <base/util/Profile.h>

#include <m3/com/MemGate.h>
#include <m3/stream/Standard.h>
#include <m3/vfs/Executable.h>
#include <m3/vfs/File.h>
#include <m3/vfs/VFS.h>
#include <m3/VPE.h>

using namespace m3;

#define COUNT   4

static cycles_t exec_time = 0;

int main() {
    if(VFS::mount("/", new M3FS("m3fs")) < 0)
        exitmsg("Mounting root-fs failed");

    {
        for(int i = 0; i < COUNT; ++i) {
            cycles_t start = Profile::start(0);
            VPE vpe("hello");
            exec_time += Profile::stop(0) - start;
        }

        cout << "Time for VPE-creation: " << (exec_time / COUNT) << " cycles\n";
    }

    exec_time = 0;

    {
        for(int i = 0; i < COUNT; ++i) {
            VPE vpe("hello");
            cycles_t start2 = Profile::start(1);
            Errors::Code res = vpe.run([start2]() {
                cycles_t end = Profile::stop(1);
                return end - start2;
            });
            if(res != Errors::NO_ERROR)
                exitmsg("VPE::run failed");

            int time = vpe.wait();
            exec_time += time;
        }
    }

    cout << "Time for run: " << (exec_time / COUNT) << " cycles\n";

    exec_time = 0;

    {
        for(int i = 0; i < COUNT; ++i) {
            VPE vpe("hello");
            cycles_t start = Profile::start(2);
            Errors::Code res = vpe.run([]() {
                return 0;
            });
            if(res != Errors::NO_ERROR)
                exitmsg("VPE::run failed");

            vpe.wait();
            cycles_t end = Profile::stop(2);
            exec_time += end - start;
        }
    }

    cout << "Time for run+wait: " << (exec_time / COUNT) << " cycles\n";

    exec_time = 0;

    {
        for(int i = 0; i < COUNT; ++i) {
            VPE vpe("hello");
            cycles_t start = Profile::start(3);
            const char *args[] = {"/bin/noop"};
            Executable exec(ARRAY_SIZE(args), args);
            Errors::Code res = vpe.exec(exec);
            if(res != Errors::NO_ERROR)
                exitmsg("Unable to load " << args[0]);

            vpe.wait();
            cycles_t end = Profile::stop(3);
            exec_time += end - start;
        }
    }

    cout << "Time for exec: " << (exec_time / COUNT) << " cycles\n";
    return 0;
}
