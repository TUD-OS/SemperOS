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

#pragma once

#include <base/PEDesc.h>

#include "mem/MemoryModule.h"

namespace kernel {
class KPE;

class Platform {
friend KPE;

public:
    static const size_t MAX_MODS        = 64;
    static const size_t MAX_PES         = 1024;
    static const size_t MAX_MEM_MODS    = 4;

    struct MemPEDesc {
        size_t pe;
        uintptr_t offs;
        uintptr_t size;
    } PACKED;

    struct KEnv {
        explicit KEnv();
        explicit KEnv(uintptr_t* _mods, size_t _pe_count, m3::PEDesc _pes[], uint32_t _kernelId,
            uint32_t _creatorKernelId, uint32_t _creatorCore, int32_t _creatorThread,
            int32_t _creatorEp, MemoryModule* _mem_mods[], size_t memOffset);

        uintptr_t mods[MAX_MODS];
        size_t pe_count;
        m3::PEDesc pes[MAX_PES];
        uint32_t kernelId;
        uint32_t creatorKernelId;
        uint32_t creatorCore;
        int32_t creatorThread;
        int32_t creatorEp;
        MemPEDesc mem_mods[MAX_MEM_MODS];
        size_t memOffset;
        uintptr_t memberTable;
        uintptr_t ddlPartitions;
        size_t ddlPartitionsSize;
    } PACKED;

    static size_t kernel_pe();
    static m3::PEDesc first_pe();
    static size_t first_pe_id();
    static size_t last_pe();

    static uintptr_t mod(size_t i) {
        return _kenv.mods[i];
    }
    static uintptr_t* mods() {
        return _kenv.mods;
    }
    static size_t pe_count() {
        return _kenv.pe_count;
    }
    static m3::PEDesc pe_by_core(size_t no) {
        for(size_t i = 0; i < MAX_PES; i++)
            if(_kenv.pes[i].core_id() == no)
                return _kenv.pes[i];
        UNREACHED;
    }
    static m3::PEDesc pe_by_index(size_t idx) {
        return _kenv.pes[idx];
    }

    static void release_pe(size_t no) {
        for(size_t i = 0; i < MAX_PES; i++)
            if(_kenv.pes[i].core_id() == no) {
                _kenv.pes[i] = _kenv.pes[_kenv.pe_count - 1];
                _kenv.pe_count--;
                return;
            }
        UNREACHED;
    }
    static void add_pe(m3::PEDesc pe) {
        _kenv.pe_count++;
        _kenv.pes[_kenv.pe_count] = pe;
    }

    static unsigned int kernelId() {
        return _kenv.kernelId;
    }
    static unsigned int creatorKernelId() {
        return _kenv.creatorKernelId;
    }
    static unsigned int creatorCore() {
        return _kenv.creatorCore;
    }
    static int creatorThread() {
        return _kenv.creatorThread;
    }
    static int creatorEp() {
        return _kenv.creatorEp;
    }
    static uint64_t ddl_member_table() {
        return _kenv.memberTable;
    }
    static uint64_t ddl_partitions() {
        return _kenv.ddlPartitions;
    }
    static size_t ddl_partitions_size() {
        return _kenv.ddlPartitionsSize;
    }

    static uintptr_t def_recvbuf(size_t no);
    static uintptr_t rw_barrier(size_t no);

private:
    static KEnv _kenv;
};

}
