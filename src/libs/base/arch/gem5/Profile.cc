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
#include <base/DTU.h>

namespace m3 {

static cycles_t rdtsc() {
    uint32_t u, l;
    asm volatile ("rdtsc" : "=a" (l), "=d" (u) : : "memory");
    return (cycles_t)u << 32 | l;
}

cycles_t Profile::start(unsigned msg) {
    Sync::memory_barrier();
    DTU::get().debug_msg(START_TSC | msg);
    return rdtsc();
}

cycles_t Profile::stop(unsigned msg) {
    DTU::get().debug_msg(STOP_TSC | msg);
    cycles_t res = rdtsc();
    Sync::memory_barrier();
    return res;
}

}
