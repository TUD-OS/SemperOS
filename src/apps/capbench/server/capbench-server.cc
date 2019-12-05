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

#include <base/Common.h>
#include <base/stream/IStringStream.h>
#include <base/util/Profile.h>
#include <base/benchmark/capbench.h>

#include <m3/server/Server.h>
#include <m3/server/RequestHandler.h>
#include <m3/stream/Standard.h>

#include "../operations.h"

// verbosity
#define CAPLOGV(msg)
//#define CAPLOGV(msg) m3::cout << msg << "\n";
#define CAPLOG(msg) m3::cout << msg << "\n";

using namespace m3;

class CapHandler;

static Server<CapHandler> *srv;
static bool run = true;

class CapSessionData : public RequestSessionData {
public:
    CapRngDesc *caps;
    // we assume consecutive calls of obtain and revoke and therefore keep pointers here
    uint obtainPtr;
    uint revokePtr;
};
using reqhandler_t = RequestHandler<
    CapHandler, CapServerSession::CapOperation, CapServerSession::CapOperation::COUNT, CapSessionData>;

class CapHandler : public reqhandler_t {
public:
    CapHandler(int numcaps) : reqhandler_t(), _count(), _handedOut(0),
        _caps(m3::CapRngDesc::OBJ, VPE::self().alloc_caps(numcaps), numcaps),
        _numcaps(numcaps), _revokeTime(new cycles_t[numcaps]), _revokes(0) {
        for(int i = 0; i < _numcaps; i++)
            new MemGate(MemGate::create_global(8, MemGate::RW, _caps.start() + i));
        add_operation(CapServerSession::CapOperation::REVCAP, &CapHandler::revcap);
    }

    // revoke caps
    virtual void revcap(GateIStream &is) {
        CapRngDesc caps;
        is >> caps;
        CapSessionData *sess = is.gate().session<CapSessionData>();
        assert(static_cast<int>(sess->revokePtr) < _numcaps);
        CAPLOGV("revcap(caps=" << caps << ") [servercaps=" << sess->caps[sess->revokePtr] << "]");

        cycles_t start = CAP_BENCH_TRACE_S(APP_REV_START);
        m3::Errors::Code res = Syscalls::get().revoke(sess->caps[sess->revokePtr++], true);
        _revokeTime[_revokes] = CAP_BENCH_TRACE_F(APP_REV_FINISH) - start;
        _revokes++;
        CAPLOGV("Revoked in " << (m3::Profile::stop(9) - start) << " cycles");
        reply_vmsg(is, res);
    }

    virtual void handle_obtain(CapSessionData *sess, RecvBuf *rcvbuf, GateIStream &args,
            uint capcount) override {
        if(!sess->send_gate()) {
            reqhandler_t::handle_obtain(sess, rcvbuf, args, capcount);
            sess->caps = new CapRngDesc[_numcaps];
            sess->obtainPtr = 0;
            sess->revokePtr = 0;
            return;
        }

        CAPLOGV("handle_obtain(count=" << capcount << ")");
        if(_handedOut + static_cast<int>(capcount) > _numcaps) {
            reply_vmsg(args, Errors::NO_SPACE);
            return;
        }

        assert(static_cast<int>(sess->obtainPtr) < _numcaps);
        sess->caps[sess->obtainPtr] = CapRngDesc(m3::CapRngDesc::OBJ,
            _caps.start() + _handedOut, capcount);
        _handedOut += capcount;
        CAPLOGV("Handing out caps: " << sess->caps[sess->obtainPtr]);
        reply_vmsg(args, Errors::NO_ERROR, sess->caps[sess->obtainPtr++]);

    }

    virtual void handle_close(CapSessionData *sess, GateIStream &args) override {
        CAPLOGV("Client closed connection.");
        reqhandler_t::handle_close(sess, args);
    }
    virtual void handle_shutdown() override {
        CAPLOGV("Kernel wants to shut down.");
        cycles_t avgRevokeTime = 0;
        cycles_t varianceRevoke = 0;
        CAPLOG("Warming up revoke: " << _revokeTime[0]);
        for(int i = 1; i < _numcaps; ++i) {
            avgRevokeTime += _revokeTime[i];
            CAPLOG("Round " << i << " revoke: " << _revokeTime[i]);
        }
        if(_revokes != static_cast<uint>(_numcaps))
            CAPLOG("WARNING: Not as many revokes as capabilities handed out! (" <<
                _revokes << " != " << _numcaps);
        avgRevokeTime = avgRevokeTime / (_numcaps - 1);
        for(int i = 1; i < _numcaps; ++i) {
            varianceRevoke += static_cast<uint>( static_cast<int>(static_cast<int>(avgRevokeTime) - _revokeTime[i]) *
                static_cast<int>(static_cast<int>(avgRevokeTime) - _revokeTime[i]) );
        }
        varianceRevoke = varianceRevoke / (_numcaps - 1);
        CAPLOGV("Revokes handled: " << _revokes);
        CAPLOG("Avg. server revoke time: " << avgRevokeTime);
        CAPLOG("Variance revoke: " << varianceRevoke);
        run = false;
    }

private:
    virtual size_t credits() {
        return 256;
    }

    int _count;
    int _handedOut;
    CapRngDesc _caps;
    int _numcaps;
    cycles_t *_revokeTime;
    uint _revokes;
};

int main(int argc, char *argv[]) {
    for(int i = 0; run && i < 10; ++i) {
        if(argc < 2) {
            cout << "Usage: " << argv[0] << " <numCaps>\n";
            return 1;
        }
        int numcaps = IStringStream::read_from<int>(argv[1]);
        CAPLOG("capbench-server [numcaps=" << numcaps << "]");
        CapHandler hdl(numcaps);
        srv = new Server<CapHandler>("capbench-server", &hdl, nextlog2<4096>::val, nextlog2<128>::val);
        if(Errors::occurred())
            break;
        env()->workloop()->run();
        delete srv;
    }
    CAPLOG("Cap Server shut down . Return code: " << Errors::to_string(Errors::last));
    return 0;
}
