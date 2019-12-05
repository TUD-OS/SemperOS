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

#include <base/Machine.h>

#include <stdlib.h>
#include <unistd.h>

namespace m3 {

void Machine::shutdown() {
    asm volatile (
        ".byte 0x0F, 0x04;"
        ".word 0x21;"
        : : "D"(0)
    );
    while(1)
        asm volatile ("hlt");
}

int Machine::write(const char *str, size_t len) {
    static const char *fileAddr = "stdout";
    asm volatile (
        ".byte 0x0F, 0x04;"
        ".word 0x4F;"
        : : "D"(str), "S"(len), "d"(0), "c"(fileAddr) : "rax"
    );
    return 0;
}

ssize_t Machine::read(char *dst, size_t max) {
    ssize_t res;
    asm volatile (
        ".byte 0x0F, 0x04;"
        ".word 0x50;"
        : "=a"(res) : "D"(dst), "S"(max), "d"(0)
    );
    return res;
}

}
