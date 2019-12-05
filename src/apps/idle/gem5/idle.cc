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
#include <base/Env.h>

typedef void (*start_func)();

extern void *_start;

EXTERN_C void try_run() {
    // is there something to run?
    start_func ptr = reinterpret_cast<start_func>(m3::env()->entry);
    if(ptr) {
        // remember exit location
        m3::env()->exitaddr = reinterpret_cast<uintptr_t>(&_start);
        asm volatile (
            // tell crt0 that we're set the stackpointer
            "mov %2, %%rsp;"
            "jmp *%1;"
            : : "a"(0xDEADBEEF), "r"(ptr), "r"(m3::env()->sp)
        );
    }
}
