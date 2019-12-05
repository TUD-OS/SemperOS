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

#include <base/Config.h>
#include <base/Init.h>

#include <sys/mman.h>

#include "mem/MainMemory.h"
#include "DTU.h"
#include "Platform.h"

namespace kernel {

INIT_PRIO_USER(2) Platform::KEnv Platform::_kenv;

Platform::KEnv::KEnv() {
    // no modules
    mods[0] = 0;

    // init PEs
    pe_count = MAX_CORES;
    for(int i = 0; i < MAX_CORES; ++i)
        pes[i] = m3::PEDesc(m3::PEType::COMP_IMEM, 1024 * 1024);

    const size_t TOTAL_MEM   = 512 * 1024 * 1024;

    // create memory
    uintptr_t base = reinterpret_cast<uintptr_t>(
        mmap(0, TOTAL_MEM, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0));
    DTU::get().config_recv_local(m3::DTU::MEM_EP, 0, 0, 0,
        m3::DTU::FLAG_NO_HEADER | m3::DTU::FLAG_NO_RINGBUF);

    MainMemory &mem = MainMemory::get();
    mem.add(new MemoryModule(false, 0, base, FS_MAX_SIZE));
    mem.add(new MemoryModule(true, 0, base + FS_MAX_SIZE, TOTAL_MEM - FS_MAX_SIZE));
}

size_t Platform::kernel_pe() {
    return 0;
}
size_t Platform::first_pe() {
    return 1;
}
size_t Platform::last_pe() {
    return _kenv.pe_count - 1;
}

uintptr_t Platform::def_recvbuf(size_t) {
    // unused
    return 0;
}

uintptr_t Platform::rw_barrier(size_t) {
    // no rw barrier here
    return 1;
}

}
