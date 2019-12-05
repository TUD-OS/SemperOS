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

#include <m3/stream/Standard.h>

using namespace m3;

alignas(DTU_PKG_SIZE) static char buffer[4096];

static void replace(char *buffer, long res, char c1, char c2) {
    for(ssize_t i = 0; i < res; ++i) {
        if(buffer[i] == c1)
            buffer[i] = c2;
    }
}

int main(int argc, char **argv) {
    if(argc != 3)
        exitmsg("Usage: " << argv[0] << " <s> <r>");

    char c1 = argv[1][0];
    char c2 = argv[2][0];

    ssize_t res;
    while((res = cin.read(buffer, sizeof(buffer))) > 0) {
        replace(buffer, res, c1, c2);
        cout.write(buffer, res);
    }
    return 0;
}
