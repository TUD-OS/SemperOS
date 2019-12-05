/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of M3 (Microkernel for Minimalist Manycores).
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

#include <m3/com/RecvGate.h>
#include <m3/com/SendGate.h>
#include <m3/server/RequestHandler.h>
#include <m3/com/GateStream.h>
#include <base/com/Marshalling.h>
#include <functional>

#include "KernelcallHandler.h"
#include "KVStore.h"
#include "ddl/MHTTypes.h"
#include "Platform.h"
#include "mem/MemoryModule.h"
#include "mem/SlabCache.h"
#include "pes/VPE.h"

namespace kernel {

struct BootModule;

struct KernelAllocation {
    explicit KernelAllocation() : rootpt(0), ptes(0), rtspace(0), elf_segments(0), elf_segments_end(0),
        kenv(0), heap(0), offset(0) {
        memset(mem_mods, 0, sizeof(mem_mods));
    }
    // allocator for elf segments
    uintptr_t alloc_elf_segment(size_t size) {
        // check if elf segment fits into initial memory module
        if((elf_segments_end - m3::DTU::build_noc_addr(mem_mods[1]->pe(), mem_mods[1]->addr()))
                + size > mem_mods[1]->size())
            PANIC("Not enough memory for kernel");
        assert(!heap);
        uintptr_t allocation = m3::Math::round_up<uintptr_t>(elf_segments_end, PAGE_SIZE);
        elf_segments_end = allocation + size;
        return allocation;
    }

    uintptr_t alloc_pte() {
        assert(ptes + PAGE_SIZE <= rtspace);
        uintptr_t pte = ptes;
        ptes += PAGE_SIZE;
        return pte;
    }

    void set_root_pt(uint64_t addr) {
        rootpt = addr;
        ptes = rootpt + PAGE_SIZE;
    }

    void set_max_ptes(uint n);
    void reserve_kenv(size_t addr) {
        kenv = addr;
        offset = m3::Math::round_up<uintptr_t>(sizeof(Platform::KEnv), DTU_PKG_SIZE);
    }

    void alloc_heap(size_t size) {
        heap = elf_segments_end;
        offset = m3::Math::round_up<size_t>(heap - rootpt + size, PAGE_SIZE);
    }

    // all NoC addresses and currently assumed to be located in the first memory module
    uintptr_t rootpt;
    uintptr_t ptes;
    uintptr_t rtspace;
    uintptr_t elf_segments;
    uintptr_t elf_segments_end;
    uintptr_t kenv;
    uintptr_t heap;
    size_t offset;
    MemoryModule* mem_mods[Platform::MAX_MEM_MODS];
};

uintptr_t load_kernel_mod(const VPEDesc &vpe, BootModule *mod, bool copy, bool needs_heap,
        KernelAllocation& kernMem, bool vm);

class KPE {
    friend struct MigratingPartitionEntry;
public:
    enum class ShutdownState {
        NONE,
        INFLIGHT,
        READY
    };

    struct WaitingKPE : m3::SListItem, SlabObject<WaitingKPE> {
        explicit WaitingKPE(int _tid, bool _revocation = false) : tid(_tid),
            revocation(_revocation) {}
        int tid;
        bool revocation;
    };

    /**
     *
     * @param prog  Name of the kernel binary
     * @param id    Kernel ID
     */
    KPE(m3::String &&prog, size_t id, size_t core, int localEP = -1, int remoteEP = -1)
        : _id(id), _name(prog), _core(core), _readyForShutdown(ShutdownState::NONE), _nextcallbackID(0),
        _localEP(localEP), _remoteEP(remoteEP), _msgsInflight(0), _lastMsgReply(false), _waitingThrds() {}
    KPE(const KPE &) = delete;
    KPE &operator=(const KPE &) = delete;
    ~KPE();

    size_t id() const {
        return _id;
    }
    size_t core() const {
        return _core;
    }
    const m3::String &name() const {
        return _name;
    }
//    RecvGate &kcrecv_gate() {
//        return _kcrecvgate;
//    }

    void init_memory(int argc, char **argv, size_t pe_count, m3::PEDesc PEs[]);
    void start(int argc, char** argv, size_t pe_count, m3::PEDesc PEs[]);

    void sendTo(const void* data, size_t size);
    void sendRevocationTo(const void* data, size_t size);

    void forwardTo(const void* data, size_t size, label_t label);

    void reply(const void* data, size_t size);

    void msg_received() {
        _msgsInflight--;
        _lastMsgReply = false;
        assert(_msgsInflight >= 0);
        if(_waitingThrds.length()) {
            // Keep the reply slot and the revocation slot free.
            // If all normal msg slots are used, continue only with a revocation if there is one
            if(!(_msgsInflight < KernelcallHandler::MAX_MSG_INFLIGHT - 2)
                    && !(_waitingThrds.begin()->revocation))
                return;
            auto it = _waitingThrds.begin();
            m3::ThreadManager::get().notify(reinterpret_cast<void*>(it->tid));
            _waitingThrds.remove(&*it);
            delete &*it;
        }
    }

    void checkShutdown();

    uint waitingThreads() {
        return _waitingThrds.length();
    }
    int numMsgsInflight() {
        return _msgsInflight;
    }

    /**
     * Call it with std::bind(&actualCB, KPE* this, std::placeholders::_1, std::placeholders::_2) as cb
     * to access members in the callback function easily.
     * Send the returned callback ID to the remote kernel, so it can include it
     * in its response, that kicks off the callback.
     * @param cb    The callback function
     * @param data  Marshaller contains data consumed by the callback, but not by the gate
     * @return      ID of the callback object
     */
    unsigned int addCallback(std::function<void(GateIStream&, m3::Unmarshaller)> cb, m3::Unmarshaller data);

    /**
     * Execute the callback with the given id
     * @param cbID  ID of the callback to be executed
     * @param is    The stream of the kernel call response
     * @param remove    Set to false to not remove the callback after execution (default = true)
     */
    void notify(unsigned int cbID, GateIStream& is, bool remove = true);

    void removeCallback(unsigned int cbID);

    ShutdownState getShutdownState() {
        return _readyForShutdown;
    }
    void setShutdownState(ShutdownState s) {
        _readyForShutdown = s;
    }

#ifdef KERNEL_STATISTICS
    static unsigned long normalMsgs;
    static unsigned long revocationMsgs;
    static unsigned long replies;
    static unsigned long delayedNormalMsgs;
    static unsigned long delayedRevocationMsgs;
    static unsigned long delayedReplies;
#endif
private:
    /**
     * Creates a KPE stub to resemble kernels which are migrating targets
     *
     * @param id
     * @param core
     */
    KPE(size_t id, size_t core) : _id(id), _name(), _core(core),
        _localEP(-1), _remoteEP(-1), _msgsInflight(0), _waitingThrds() {}

    struct cbData {
        explicit cbData(std::function<void(GateIStream&, m3::Unmarshaller)> _func, m3::Unmarshaller _data)
            : func(_func), data(_data){
        };
        std::function<void(GateIStream&, m3::Unmarshaller)> func;
        m3::Unmarshaller data;
    };

    size_t _id;
    m3::String _name;
    size_t _core;
    ShutdownState _readyForShutdown;
    unsigned int _nextcallbackID;
    // Store callback and related state
    KVStore<unsigned int, cbData*>_callbacks;
    int _localEP;
    int _remoteEP;
    int _msgsInflight;
    bool _lastMsgReply;
    m3::SList<WaitingKPE> _waitingThrds;
    static bool _shutdownReplySent;
};

/**
 * Used by the DDL to store migration targets
 */
struct MigratingPartitionEntry : public m3::SListItem {
    explicit MigratingPartitionEntry(membership_entry::pe_id_t partID,
        membership_entry::krnl_id_t destID, membership_entry::pe_id_t destCore)
        : partitionID(partID), destKrnl(destID, destCore) {}
    membership_entry::pe_id_t partitionID;
    KPE destKrnl;
};

}