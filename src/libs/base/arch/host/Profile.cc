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

#include <sys/time.h>

namespace m3 {

cycles_t Profile::start(unsigned u) {
    return stop(u);
}

cycles_t Profile::stop(unsigned) {
#if defined(__i386__) or defined(__x86_64__)
    uint32_t u, l;
    asm volatile ("rdtsc" : "=a" (l), "=d" (u) : : "memory");
    return (cycles_t)u << 32 | l;
#elif defined(__arm__)
    struct timeval tv;
    gettimeofday(&tv,nullptr);
    return (cycles_t)tv.tv_sec * 1000000 + tv.tv_usec;
#else
#   warning "Cycle counter not supported"
    return 0;
#endif
}

}
