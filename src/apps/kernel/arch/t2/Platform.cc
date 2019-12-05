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

#include <base/Init.h>

#include "mem/MainMemory.h"
#include "DTU.h"
#include "Platform.h"

namespace kernel {

INIT_PRIO_USER(2) Platform::KEnv Platform::_kenv;

Platform::KEnv::KEnv() {
    // no modules
    mods[0] = 0;

    // init PEs
    pe_count = 10;
    for(int i = 0; i < 8; ++i)
        pes[i] = m3::PEDesc(m3::PEType::COMP_IMEM, 64 * 1024);
    pes[8] = m3::PEDesc(m3::PEType::COMP_IMEM, 128 * 1024);
    pes[9] = m3::PEDesc(m3::PEType::MEM, 512 * 1024 * 1024);

    // register memory modules
    MainMemory &mem = MainMemory::get();
    const size_t USABLE_MEM  = 64 * 1024 * 1024;
    mem.add(new MemoryModule(false, 9, 0, USABLE_MEM));
    mem.add(new MemoryModule(true, 9, USABLE_MEM, pes[9].mem_size() - USABLE_MEM));
}

size_t Platform::kernel_pe() {
    return 0;
}
size_t Platform::first_pe() {
    return 1;
}
size_t Platform::last_pe() {
    return _kenv.pe_count - 2;
}

uintptr_t Platform::def_recvbuf(size_t) {
    return DEF_RCVBUF;
}

uintptr_t Platform::rw_barrier(size_t) {
    // no rw barrier here
    return 1;
}

}
