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

#include <base/Common.h>
#include <base/util/Profile.h>
#include <base/benchmark/capbench.h>
#include <base/Compiler.h>

#include <m3/server/Server.h>
#include <m3/server/RequestHandler.h>
#include <m3/stream/Standard.h>

#include "operations.h"

// verbosity
//#define CAPLOGV(msg)
#define CAPLOGV(msg) m3::cout << m3::VPE::self().pe().core_id() << ": " << msg << "\n";
#define CAPLOG(msg) m3::cout << msg << "\n";

using namespace m3;

class CapHandler;

class CapSessionData : public RequestSessionData {
public:
};
using reqhandler_t = RequestHandler<
    CapHandler, CapServerSession::CapOperation, CapServerSession::CapOperation::COUNT, CapSessionData>;

class CapHandler : public reqhandler_t {
public:
    CapHandler(bool isRoot, int numcaps, CapServerSession **sess = nullptr, uint numSessions = 0) : reqhandler_t(),
        _isRoot(isRoot), _obtained(0), _initCap(m3::CapRngDesc::OBJ, VPE::self().alloc_caps(1), 1),
        _lastCap(_initCap), _numcaps(numcaps), _revokeTime(0), _revokes(0), _sess(sess),
        _numSessions(numSessions), _handedOut(0), _obtainsDone(0)
    {
        if(_isRoot)
            new MemGate(MemGate::create_global(8, MemGate::RW, _lastCap.start()));
        add_operation(CapServerSession::CapOperation::SIGNAL, &CapHandler::signal);
        add_operation(CapServerSession::CapOperation::OBTAINDONE, &CapHandler::obtainDone);
    }

    void setSession(CapServerSession **sess) {
        _sess = sess;
    }

    // used to connect the root server to the link server
    void signal(GateIStream &);

    void obtainDone(GateIStream &) {
        _obtainsDone++;
        // when all links have obtained the capability, start the revocation
        if(_obtainsDone == _numSessions) {
            revcap();
            CAPLOGV("Revokes handled: " << _revokes);
            CAPLOG("Avg. server revoke time: " << _revokeTime);
            Syscalls::get().exit(0);
        }
    }

    // revoke capability chain root
    void revcap() {
        CAPLOGV("revcap()");

        cycles_t start = CAP_BENCH_TRACE_S(APP_REV_START);
        UNUSED m3::Errors::Code res = Syscalls::get().revoke(_initCap, true);
        _revokeTime += CAP_BENCH_TRACE_F(APP_REV_FINISH) - start;
        _revokes++;
        CAPLOGV("Revoked in " << (m3::Profile::stop(9) - start) << " cycles. Code: " << res);
    };

    virtual void handle_obtain(CapSessionData *sess, RecvBuf *rcvbuf, GateIStream &args,
            uint capcount) override;

    virtual void handle_close(CapSessionData *sess, GateIStream &args) override {
        CAPLOGV("Client closed connection.");
        reqhandler_t::handle_close(sess, args);
    }
    virtual void handle_shutdown() override {
        CAPLOGV("Kernel wants to shut down.");
    }

    void obtaincap();

private:
    virtual size_t credits() {
        return 256;
    }

    bool _isRoot;           ///> indicates the root of the capability chain
    int _obtained;
    CapRngDesc _initCap;    ///> Root capability of the chain
    CapRngDesc _lastCap;       ///> Cap received from the other CapHanlder
    int _numcaps;           ///> Number of capabilities this handler contributes to the chain
    cycles_t _revokeTime;
    uint _revokes;
    CapServerSession **_sess;
    uint _numSessions;
    int _handedOut;
    uint _obtainsDone;
};

