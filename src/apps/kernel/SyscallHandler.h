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

#include <base/KIF.h>

#include "cap/CapTable.h"
#include "com/Services.h"
#include "pes/VPE.h"
#include "Gate.h"

namespace kernel {

class SyscallHandler {
    explicit SyscallHandler();

public:
    using handler_func = void (SyscallHandler::*)(GateIStream &is);

    static const int SYSC_SLOTS         = m3::DTU::MAX_MSG_SLOTS * DTU::SYSC_GATES;

    static SyscallHandler &get() {
        return _inst;
    }

    void add_operation(m3::KIF::Syscall::Operation op, handler_func func) {
        _callbacks[op] = func;
    }

    void handle_message(GateIStream &msg, m3::Subscriber<GateIStream&> *) {
        EVENT_TRACER_handle_message();
        m3::KIF::Syscall::Operation op;
        msg >> op;
        if(static_cast<size_t>(op) < sizeof(_callbacks) / sizeof(_callbacks[0])) {
            (this->*_callbacks[op])(msg);
            return;
        }
        reply_vmsg(msg, m3::Errors::INV_ARGS);
    }

    size_t epid(uint offset) const {
        // we can use it here because we won't issue syscalls ourself
        return m3::DTU::SYSC_EP + offset;
    }
    size_t srvepid() const {
        return _serv_ep;
    }

    RecvGate create_gate(VPE *vpe, int epID) {
        return RecvGate(epID, vpe);
    }

    int reserve_ep(int vpeID) {
        for(size_t i = 0; i < SYSC_SLOTS; i++)
            if(_epOccup[i] == -1) {
                _epOccup[i] = vpeID;
                return (i / m3::DTU::MAX_MSG_SLOTS) + m3::DTU::SYSC_EP;
            }
        return -1;
    }
    int ep_for(int vpeID) {
        for(size_t i = 0; i < SYSC_SLOTS; i++)
            if(_epOccup[i] == vpeID)
                return (i / m3::DTU::MAX_MSG_SLOTS) + m3::DTU::SYSC_EP;
        return -1;
    }

    void tryTerminate();

    void pagefault(GateIStream &is);
    void createsrv(GateIStream &is);
    void createsess(GateIStream &is);
    void createsessat(GateIStream &is);
    void creategate(GateIStream &is);
    void createvpe(GateIStream &is);
    void createmap(GateIStream &is);
    void attachrb(GateIStream &is);
    void detachrb(GateIStream &is);
    void exchange(GateIStream &is);
    void vpectrl(GateIStream &is);
    void delegate(GateIStream &is);
    void obtain(GateIStream &is);
    void activate(GateIStream &is);
    void reqmem(GateIStream &is);
    void derivemem(GateIStream &is);
    void revoke(GateIStream &is);
    void exit(GateIStream &is);
    void noop(GateIStream &is);

#if defined(__host__)
    void init(GateIStream &is);
#endif

private:
    m3::Errors::Code do_exchange(VPE *v1, VPE *v2, const m3::CapRngDesc &c1, const m3::CapRngDesc &c2, bool obtain);
    void exchange_over_sess(GateIStream &is, bool obtain);

    int _serv_ep;
    // +1 for init on host
    handler_func _callbacks[m3::KIF::Syscall::COUNT + 1];
    int _epOccup[SYSC_SLOTS];
    static SyscallHandler _inst;
};

}
