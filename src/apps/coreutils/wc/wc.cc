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

#include <base/util/Chars.h>

#include <m3/stream/Standard.h>

using namespace m3;

alignas(DTU_PKG_SIZE) static char buffer[4096];

static void count(FStream &in) {
    long lines = 0;
    long words = 0;
    long bytes = 0;

    ssize_t res;
    int last_space = false;
    while((res = in.read(buffer, sizeof(buffer))) > 0) {
        for(ssize_t i = 0; i < res; ++i) {
            if(buffer[i] == '\n')
                lines++;
            int space = Chars::isspace(buffer[i]);
            if(!last_space && space)
                words++;
            last_space = space;
        }
        bytes += res;
    }

    cout << fmt(lines, 7) << " " << fmt(words, 7) << " " << fmt(bytes, 7) << "\n";
}

int main(int argc, char **argv) {
    if(argc == 1)
        count(cin);
    else {
        FStream in(argv[1], FILE_R);
        if(in.error())
            exitmsg("open of " << argv[1] << " failed");
        count(in);
    }
    return 0;
}
