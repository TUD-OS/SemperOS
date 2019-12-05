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

#include <base/util/Profile.h>

#include <m3/stream/Standard.h>
#include <m3/vfs/Executable.h>
#include <m3/VPE.h>

using namespace m3;

int main(int argc, char **argv) {
    if(argc < 2)
        exitmsg("Usage: " << argv[0] << " <file>...");

    int res;
    cycles_t start = Profile::start(0);
    {
        VPE child(argv[1]);
        if(Errors::last != Errors::NO_ERROR)
            exitmsg("Creating VPE for " << argv[1] << " failed");

        child.fds()->set(STDIN_FD, VPE::self().fds()->get(STDIN_FD));
        child.fds()->set(STDOUT_FD, VPE::self().fds()->get(STDOUT_FD));
        child.fds()->set(STDERR_FD, VPE::self().fds()->get(STDERR_FD));
        child.obtain_fds();

        child.mountspace(*VPE::self().mountspace());
        child.obtain_mountspace();

        Executable exec(argc - 1, const_cast<const char**>(argv) + 1);
        Errors::Code err = child.exec(exec);
        if(err != Errors::NO_ERROR)
            exitmsg("Executing " << argv[1] << " failed");

        res = child.wait();
    }

    cycles_t end = Profile::stop(0);

    cerr << "VPE (" << argv[1] << ") terminated with exit-code " << res << "\n";
    cerr << "Runtime: " << (end - start) << " cycles\n";
    return 0;
}
