/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
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
#include "mem/MemoryModule.h"
#include "pes/VPE.h"
#include "DTU.h"
#include "Platform.h"

namespace kernel {

INIT_PRIO_USER(2) Platform::KEnv Platform::_kenv;

// note that we currently assume here, that memory PEs are first, followed by all compute PEs
static size_t _first_pe_id;

Platform::KEnv::KEnv() {
    // the KernelEnv is stored in the first PE (memory PE)
    uintptr_t addr = m3::DTU::noc_to_virt(reinterpret_cast<uintptr_t>(m3::env()->kenv));
    DTU::get().read_mem(VPEDesc(0, 0), addr, this, sizeof(*this));

    // set the correct kernel ID in the DTU
    DTU::get().unset_vpeid(VPEDesc(m3::env()->coreid, 0));
    DTU::get().set_vpeid(VPEDesc(m3::env()->coreid, kernelId));

    // register memory modules
    int count = 0;
    const size_t USABLE_MEM  = 6160UL * 1024UL * 1024UL;
    MainMemory &mem = MainMemory::get();
    for(size_t i = 0; i < MAX_MEM_MODS && mem_mods[i].size != 0; i++) {
        // the first memory module hosts the FS image and other stuff
        if(count == 0 ) {
            // only the init kernel needs to split this
            if(kernelId == creatorKernelId) {
                mem.add(new MemoryModule(false, mem_mods[i].pe, 0, USABLE_MEM));
                mem.add(new MemoryModule(true, mem_mods[i].pe, USABLE_MEM, mem_mods[i].size - USABLE_MEM));
            }
            else
                mem.add(new MemoryModule(false, mem_mods[i].pe, mem_mods[i].offs, mem_mods[i].size));
        }
        else if(count == 1) {
            // the second memory module contains the kernel's writable segments, stack and initial heap
            if(memOffset) {
                mem.add(new MemoryModule(false, mem_mods[i].pe, mem_mods[i].offs, memOffset));
                mem.add(new MemoryModule(true, mem_mods[i].pe, mem_mods[i].offs + memOffset,
                    mem_mods[i].size - memOffset));
            }
            else
                mem.add(new MemoryModule(true, mem_mods[i].pe, mem_mods[i].offs, mem_mods[i].size));
        }
        else
            mem.add(new MemoryModule(true, mem_mods[i].pe, mem_mods[i].offs, mem_mods[i].size));
        count++;
    }
    for(size_t i = 0; i < pe_count; i++) {
        if(pes[i].core_id() != kernel_pe() && pes[i].type() != m3::PEType::MEM) {
            _first_pe_id = i;
            break;
        }
    }
}

Platform::KEnv::KEnv(uintptr_t* _mods, size_t _pe_count, m3::PEDesc _pes[], uint32_t _kernelId,
    uint32_t _creatorKernelId, uint32_t _creatorCore, int32_t _creatorThread,
    int32_t _creatorEp, MemoryModule* _mem_mods[], size_t _memOffset)
{
    memset(mods, 0, sizeof(mods));
    for(uint i = 0; i < MAX_MODS; i++) {
        mods[i] = _mods[i];
    }
    pe_count = _pe_count;
    kernelId = _kernelId;
    creatorKernelId = _creatorKernelId;
    creatorCore = _creatorCore;
    creatorThread = _creatorThread;
    creatorEp = _creatorEp;
    memset(pes, 0, sizeof(pes));
    for(uint i = 0; i < pe_count; i++) {
        pes[i] = _pes[i];
    }
    memset(mem_mods, 0, sizeof(mem_mods));
    for(uint i = 0; i < MAX_MEM_MODS; i++) {
        if(!_mem_mods[i])
            break;
        mem_mods[i] = {_mem_mods[i]->pe(), _mem_mods[i]->addr(), _mem_mods[i]->size()};
    }
    memOffset = _memOffset;
    memberTable = 0;
    ddlPartitions = 0;
}

size_t Platform::kernel_pe() {
    // gem5 initializes the coreid for us
    return m3::env()->coreid;
}
m3::PEDesc Platform::first_pe() {
    return _kenv.pes[_first_pe_id];
}
size_t Platform::first_pe_id() {
    return _first_pe_id;
}

uintptr_t Platform::def_recvbuf(size_t no) {
    return rw_barrier(no);
}

uintptr_t Platform::rw_barrier(size_t no) {
    if(pe_by_core(no).has_virtmem())
        return RECVBUF_SPACE;
    return pe_by_core(no).mem_size() - RECVBUF_SIZE_SPM;
}

}
