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

#pragma once

#include <base/stream/OStringStream.h>
#include <base/PEDesc.h>

#include "pes/VPE.h"
#include "DTU.h"
#include "Platform.h"
#include "SyscallHandler.h"
#include "ddl/MHTInstance.h"

namespace kernel {

class VPE;

class PEManager {
    friend class VPE;
    friend class Coordinator;

    struct Pending : public m3::SListItem {
        explicit Pending(VPE *_vpe, int _argc, char **_argv)
            : vpe(_vpe), argc(_argc), argv(_argv) {
        }

        VPE *vpe;
        int argc;
        char **argv;
    };

public:
    static void create() {
        _inst = new PEManager();
    }
    static PEManager &get() {
        return *_inst;
    }
    static void shutdown();
    static void destroy() {
        if(_inst) {
            delete _inst;
            _inst = nullptr;
        }
    }
    bool terminate();

private:
    explicit PEManager();
    ~PEManager();

public:
    void load(int argc, char **argv);

    /**
     * Get the next free (locally maintained) core
     * @return The coreid of a free core, Platform::MAX_PES if no free cores left.
     */
    size_t free_core(const m3::PEDesc &pe) {
        size_t i;
        for(i = Platform::first_pe_id(); i <= Platform::pe_count(); ++i) {
            size_t coreId = Platform::pe_by_index(i).core_id();
            if(MHTInstance::getInstance().responsibleKrnl(coreId) == Coordinator::get().kid() &&
                !Coordinator::get().isKPE(coreId) &&
                _vpes[coreId] == nullptr &&
                Platform::pe_by_index(i).type() == pe.type())
            {
                return coreId;
            }
        }
        return Platform::MAX_PES;
    }
    /**
     * Get the next free (locally maintained) core that is not a memory PE
     * @return The coreid of a free core, Platform::MAX_PES if no free cores left.
     */
    size_t free_compute_core() {
        size_t i;
        for(i = Platform::first_pe_id(); i <= Platform::pe_count(); ++i) {
            size_t coreId = Platform::pe_by_index(i).core_id();
            if(MHTInstance::getInstance().responsibleKrnl(coreId) == Coordinator::get().kid() &&
                !Coordinator::get().isKPE(coreId) &&
                _vpes[coreId] == nullptr &&
                Platform::pe_by_index(i).type() != m3::PEType::MEM)
            {
                return coreId;
            }
        }
        return Platform::MAX_PES;
    }
    /**
     * Get the next free (locally maintained) core that is not a memory PE
     * @param offset    Number of free cores to the end of the free list
     * @return The coreid of a free core, Platform::MAX_PES if no free cores left.
     */
    size_t free_compute_core_from_back(uint offset = 0) {
        size_t selectedCore = Platform::MAX_PES;
        for(size_t i = Platform::pe_count() - 1; i >= Platform::first_pe_id(); --i) {
            size_t coreId = Platform::pe_by_index(i).core_id();

            if(MHTInstance::getInstance().responsibleKrnl(coreId) ==
                    Coordinator::get().kid() &&
                !Coordinator::get().isKPE(coreId) &&
                _vpes[coreId] == nullptr &&
                Platform::pe_by_index(i).type() != m3::PEType::MEM)
            {
                if(offset == 0) {
                    selectedCore = coreId;
                    break;
                }
                else {
                    offset--;
                }
            }
        }
        return selectedCore;
    }

    VPE *create(m3::String &&name, const m3::PEDesc &pe, int ep, capsel_t pfgate);
    void remove(int id, bool daemon);

    size_t used() const {
        return _count;
    }
    size_t daemons() const {
        return _daemons;
    }
    bool exists(int id) {
        return id < (int)Platform::pe_count() && _vpes[id];
    }
    VPE &vpe(int id) {
        assert(_vpes[id]);
        return *_vpes[id];
    }

    void start_pending(ServiceList &serv, RemoteServiceList &rsrv);
    uint open_requirements(ServiceList &serv, RemoteServiceList &rsrv);

private:
    void deprivilege_pes() {
        for(size_t i = Platform::first_pe_id(); i < Platform::pe_count(); ++i)
            if(Platform::pe_by_index(i).type() != m3::PEType::MEM)
                DTU::get().deprivilege(Platform::pe_by_index(i).core_id());
    }
    void deprivilege_pes(size_t start, size_t end) {
        for(; start <= end; ++start) {
            assert(MHTInstance::getInstance().responsibleKrnl(start) == Coordinator::get().kid());
            DTU::get().deprivilege(start);
        }
    }

    static m3::String path_to_name(const m3::String &path, const char *suffix);
    static m3::String fork_name(const m3::String &name);

    VPE **_vpes;
    size_t _count;
    size_t _daemons;
    m3::SList<Pending> _pending;
    static bool _shutdown;
    static PEManager *_inst;
};

}
