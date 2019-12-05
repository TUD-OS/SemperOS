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

#include <base/col/SList.h>
#include <base/KIF.h>

#include <cstring>

#include "cap/CapTable.h"
#include "cap/Capability.h"
#include "mem/AddrSpace.h"
#include "mem/SlabCache.h"

namespace kernel {

struct VPEDesc {
    explicit VPEDesc(int _core, int _id) : core(_core), id(_id) {
    }

    int core;
    int id;
};

#if defined(__gem5__)
struct BootModule {
    char name[128];
    uint64_t addr;
    uint64_t size;
} PACKED;

BootModule *get_mod(const char *name, bool *first);
void copy_clear(const VPEDesc &vpe, uintptr_t dst, uintptr_t src, size_t size, bool clear);
void read_from_mod(BootModule *mod, void *data, size_t size, size_t offset);
void map_idle(VPE &vpe);
void map_idle(const VPEDesc &vpe, KernelAllocation& kernMem, bool vm);
#endif

class VPE : public SlabObject<VPE> {
    // use an object to set the VPE id at first and unset it at last
    struct VPEId {
        VPEId(int _id, int _core);
        ~VPEId();

        VPEDesc desc;
    };

public:
    static const uint16_t INVALID_ID = static_cast<uint16_t>(
                                        ((static_cast<size_t>(1) << (VPE_BITS)) - 1));

    enum State {
        RUNNING,
        DEAD
    };

    enum Flags {
        BOOTMOD     = 1 << 0,
        DAEMON      = 1 << 1,
        MEMINIT     = 1 << 2,
    };

    struct ServName : public m3::SListItem {
        explicit ServName(const m3::String &_name) : name(_name) {
        }
        m3::String name;
    };

    static constexpr int SYSC_CREDIT_ORD    = m3::nextlog2<512>::val;

#if defined(__gem5__)
    // TODO this will move to another place in order to be used by VPE and KPE
    static size_t count;
    static BootModule mods[Platform::MAX_MODS];
    static uint64_t loaded;
    static BootModule *idles[Platform::MAX_PES];
#endif

    explicit VPE(m3::String &&prog, size_t id, size_t _core, bool bootmod, int syscEP,
        int ep = -1, capsel_t pfgate = m3::KIF::INV_SEL);
    VPE(const VPE &) = delete;
    VPE &operator=(const VPE &) = delete;
    ~VPE();

    int refcount() const {
        return _refs;
    }
    void ref() {
        _refs++;
    }
    void unref();

    void start(int argc, char **argv, int pid);
    void exit(int exitcode);

    void init();
    void activate_sysc_ep(void *addr);
    m3::Errors::Code xchg_ep(size_t epid, MsgCapability *oldcapobj, MsgCapability *newcapobj);

    const VPEDesc &desc() const {
        return _id.desc;
    }
    int id() const {
        return desc().id;
    }
    int pid() const {
        return _pid;
    }
    int core() const {
        return desc().core;
    }
    uint flags() const {
        return _flags;
    }
    int state() const {
        return _state;
    }
    int exitcode() const {
        return _exitcode;
    }
    int sessAwaitingResp() const {
        return _sessAwaitingResp;
    }
    void sessAwaitingResp(int awaiting) {
        assert(!_sessAwaitingResp);
        _sessAwaitingResp = awaiting;
    }
    int sessRespArrived() {
        return --_sessAwaitingResp;
    }
    GateOStream *srvLookupResult() {
        return _srvLookupResult;
    }
    void srvLookupResult(GateOStream *stream) {
        if(_srvLookupResult) {
            m3::Heap::free(const_cast<void*>(static_cast<const void*>(_srvLookupResult->bytes())));
            m3::Heap::free(_srvLookupResult);
        }
        _srvLookupResult = stream;
    }
    AddrSpace *address_space() {
        return _as;
    }
    void subscribe_exit(const m3::Subscriptions<int>::callback_type &cb) {
        _exitsubscr.subscribe(cb);
    }
    void unsubscribe_exit(m3::Subscriber<int> *sub) {
        _exitsubscr.unsubscribe(sub);
    }
    const m3::SList<ServName> &requirements() const {
        return _requires;
    }
    void add_requirement(const m3::String &name) {
        _requires.append(new ServName(name));
    }
    const m3::String &name() const {
        return _name;
    }
    CapTable &objcaps() {
        return _objcaps;
    }
    CapTable &mapcaps() {
        return _mapcaps;
    }
    RecvGate &syscall_gate() {
        return _syscgate;
    }
    RecvGate &service_gate() {
        return _srvgate;
    }
    void *eps() {
        return _eps;
    }
    void make_daemon() {
        _flags |= DAEMON;
    }
    mht_key_t mht_key(ItemType type, uint objId) {
        return HashUtil::structured_hash(_id.desc.core, _id.desc.id, type, objId);
    }
    void replace_last_arg(const m3::String &replacement) {
        _replaceLastArg = replacement;
    }

private:
    void init_memory(int argc, const char *argv);
    void write_env_file(int pid, label_t label, size_t epid);
    void activate_sysc_ep();

    void free_reqs() {
        for(auto it = _requires.begin(); it != _requires.end(); ) {
            auto old = it++;
            delete &*old;
        }
    }
    void detach_rbufs();

    VPEId _id;
    uint _flags;
    int _refs;
    int _pid;
    int _state;
    int _exitcode;
    int _sessAwaitingResp;
    int _syscEP;
    GateOStream *_srvLookupResult;
    m3::String _name;
    CapTable _objcaps;
    CapTable _mapcaps;
    void *_eps;
    RecvGate _syscgate;
    RecvGate _srvgate;
    AddrSpace *_as;
    m3::SList<ServName> _requires;
    m3::Subscriptions<int> _exitsubscr;
    m3::String _replaceLastArg;
};

}
