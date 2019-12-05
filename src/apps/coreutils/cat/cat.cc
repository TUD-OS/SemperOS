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

#include <m3/stream/FStream.h>
#include <m3/stream/Standard.h>
#include <m3/vfs/VFS.h>

using namespace m3;

alignas(DTU_PKG_SIZE) static char buffer[1024];

static void copy(FStream &in, FStream &out) {
    size_t res;
    while((res = in.read(buffer, sizeof(buffer))) > 0)
        out.write(buffer, res);
}

int main(int argc, char **argv) {
    if(argc == 1)
        copy(cin, cout);
    else {
        for(int i = 1; i < argc; ++i) {
            FStream input(argv[i], FILE_R);
            if(Errors::occurred()) {
                errmsg("Open of " << argv[i] << " failed");
                continue;
            }

            copy(input, cout);
        }
    }
    return 0;
}
