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

#include <base/col/SList.h>

#include "ddl/MHTTypes.h"
#include "Kernelcalls.h"
#include "Gate.h"

namespace kernel {
class KernelcallHandler {
    explicit KernelcallHandler();

public:
    using handler_func = void (KernelcallHandler::*)(GateIStream &is);

    static const int MAX_MSG_INFLIGHT       = 4;
    static const int KRNLC_SLOTS            = (m3::DTU::MAX_MSG_SLOTS * DTU::KRNLC_GATES) / MAX_MSG_INFLIGHT;

    struct ConnectionRequest : m3::SListItem {
        explicit ConnectionRequest(membership_entry::krnl_id_t _kid, int _epid) : kid(_kid), epid(_epid) {}
        membership_entry::krnl_id_t kid;
        int epid;
    };

    static KernelcallHandler &get() {
        return _inst;
    }

    void add_operation(Kernelcalls::Operation op, handler_func func) {
        _callbacks[op] = func;
    }

    void handle_message(GateIStream &msg, m3::Subscriber<GateIStream&> *) {
        EVENT_TRACER_handle_message();
        Kernelcalls::Operation op;
        msg >> op;
        if(static_cast<size_t>(op) < sizeof(_callbacks) / sizeof(_callbacks[0])) {
            (this->*_callbacks[op])(msg);
            return;
        }
        reply_vmsg(msg, m3::Errors::INV_ARGS);
    }

    size_t epid(uint offset) const {
        return DTU::KRNLC_EP + offset;
    }

    RecvGate &rcvgate(int gate) {
        return _rcvgate[gate];
    }

    int ep_for(membership_entry::krnl_id_t krnl) {
        for(size_t i = 0; i < KRNLC_SLOTS; i++)
            if(_epOccup[i] == krnl)
                return (i * MAX_MSG_INFLIGHT / m3::DTU::MAX_MSG_SLOTS) + DTU::KRNLC_EP;
        return -1;
    }

    int reserve_ep(membership_entry::krnl_id_t krnl) {
        for(size_t i = 0; i < KRNLC_SLOTS; i++)
            if(_epOccup[i] == -1) {
                _epOccup[i] = krnl;
                return (i * MAX_MSG_INFLIGHT / m3::DTU::MAX_MSG_SLOTS) + DTU::KRNLC_EP;
            }
        return -1;
    }

    void addConnectionReq(ConnectionRequest *request) {
        _connectionReqs.append(request);
    }

    void sigvital(GateIStream &is);
    void kexit(GateIStream &is);
    void kcreatevpe(GateIStream &is);
    void mhtget(GateIStream &is);
    void mhtgetLocking(GateIStream &is);
    void mhtput(GateIStream &is);
    void mhtputUnlocking(GateIStream &is);
    void mhtlock(GateIStream &is);
    void mhtunlock(GateIStream &is);
    void mhtreserve(GateIStream &is);
    void mhtrelease(GateIStream &is);
    void membershipUpdate(GateIStream &is);
    void migratePartition(GateIStream &is);
    void createSessFwd(GateIStream &is);
    void createSessResp(GateIStream &is);
    void createSessFail(GateIStream &is);
    void exchangeOverSession(GateIStream &is);
    void exchangeOverSessionReply(GateIStream &is);
    void exchangeOverSessionAck(GateIStream &is);
    void removeChildCapPtr(GateIStream &is);
    void recvBufisAttached(GateIStream &is);
    void recvBufAttached(GateIStream &is);
    void announceSrv(GateIStream &is);
    void revoke(GateIStream &is);
    void revokeFinish(GateIStream &is);
    void requestShutdown(GateIStream &is);
    void shutdown(GateIStream &is);
    void announceKrnls(GateIStream &is);
    void connect(GateIStream &is);
    void reply(GateIStream &is);
    void startApps(GateIStream &is);

private:
    RecvGate _rcvgate[DTU::KRNLC_GATES];
    int _epOccup[KRNLC_SLOTS];
    m3::SList<ConnectionRequest> _connectionReqs;
    handler_func _callbacks[Kernelcalls::COUNT];
    static KernelcallHandler _inst;
};
}