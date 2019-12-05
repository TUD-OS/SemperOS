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

struct ClientData : public SListItem {
    explicit ClientData(int epid, size_t msgoff) : epId(epid), msgOff(msgoff) {}
    // notify client and free the message slot
    void release() {
        auto msg = m3::create_vmsg(m3::Errors::NO_ERROR);
        DTU::get().reply(epId, msg.bytes(), msg.total(), msgOff);
//        DTU::get().mark_read(epId, msgOff);
    }
    int epId;
    size_t msgOff;
};

class CapSessionData : public RequestSessionData {
};
using reqhandler_t = RequestHandler<
    CapHandler, CapServerSession::CapOperation, CapServerSession::CapOperation::COUNT, CapSessionData>;

class CapHandler : public reqhandler_t {
public:
    CapHandler(int numcaps, int rounds = 11) : reqhandler_t(), _count(), _handedOut(0),
        _initCap(m3::CapRngDesc::OBJ, VPE::self().alloc_caps(1), 1),
        _numcaps(numcaps), _revokeTime(new cycles_t[rounds]), _revokes(0), _waitingClients(), _rounds(rounds) {
        new MemGate(MemGate::create_global(8, MemGate::RW, _initCap.start()));
        add_operation(CapServerSession::CapOperation::SIGNAL, &CapHandler::signal);
    }

    // signal to indicate number of readily obtained caps
    virtual void signal(GateIStream &is) {
        _handedOut++;
        CAPLOGV("signal() handedout: " << _handedOut << " revokes: " << _revokes);

        // store client in list to reply later when everyone is done to start second round
        is.claim();
        ClientData *client = new ClientData(is.gate().epid(),
            DTU::get().get_msgoff(is.gate().epid(), &is.message()));
        CAPLOGV("saving client at epid: " << client->epId << " msgoff: " << client->msgOff);
        _waitingClients.append(client);

        if(_handedOut == _numcaps) {
            cycles_t start = CAP_BENCH_TRACE_S(APP_REV_START);
            UNUSED m3::Errors::Code res = Syscalls::get().revoke(_initCap, true);
            _revokeTime[_revokes] = CAP_BENCH_TRACE_F(APP_REV_FINISH) - start;
            _revokes++;
            CAPLOGV("Revocation done. Return Code " << res);

            // print statistics after last round
            if(_revokes == _rounds) {
                cycles_t revokeTimeAvg = 0;
                CAPLOG("Revoke 0: " << _revokeTime[0]);
                for(uint i = 1; i < _rounds; i++) {
                    CAPLOG("Revoke " << i << ": " << _revokeTime[i]);
                    revokeTimeAvg += _revokeTime[i];
                }

                revokeTimeAvg = revokeTimeAvg / (_revokes - 1);

                cycles_t varianceRevoke = 0;
                for(uint i = 1; i < _rounds; ++i) {
                    varianceRevoke += static_cast<uint>( static_cast<int>(static_cast<int>(revokeTimeAvg) - _revokeTime[i]) *
                        static_cast<int>(static_cast<int>(revokeTimeAvg) - _revokeTime[i]) );
                }
                varianceRevoke = varianceRevoke / (_rounds - 1);

                CAPLOGV("Revokes handled: " << (_revokes - 1));
                CAPLOG("Avg. server revoke time: " << revokeTimeAvg);
                CAPLOG("server revoke time variance: " << varianceRevoke);
            }
            else {
                // get a new MemCap
                new MemGate(MemGate::create_global(8, MemGate::RW, _initCap.start()));
                _handedOut = 0;
            }

            // let clients continue
            CAPLOGV("Replying to clients");
            for(auto it = _waitingClients.begin(); it != _waitingClients.end();) {
                auto curIt = it++;
                CAPLOGV("reply");
                curIt->release();
                delete &*curIt;
            }
            _waitingClients.remove_all();
        }
    }

    virtual void handle_obtain(CapSessionData *sess, RecvBuf *rcvbuf, GateIStream &args,
            uint capcount) override {
        if(!sess->send_gate()) {
            reqhandler_t::handle_obtain(sess, rcvbuf, args, capcount);
            return;
        }

        reply_vmsg(args, Errors::NO_ERROR, _initCap);
    }

    virtual void handle_close(CapSessionData *sess, GateIStream &args) override {
        CAPLOGV("Client closed connection.");
        reqhandler_t::handle_close(sess, args);
    }
    virtual void handle_shutdown() override {
        CAPLOGV("Kernel wants to shut down.");
//        _revokeTime = _revokeTime / _revokes;
//        CAPLOGV("Revokes handled: " << _revokes);
//        CAPLOG("Avg. server revoke time: " << _revokeTime);
//        run = false;
    }

private:
    virtual size_t credits() {
        return 256;
    }

    int _count;
    int _handedOut;
    CapRngDesc _initCap;    ///> Root capability of the tree
    int _numcaps;
    cycles_t *_revokeTime;
    uint _revokes;
    SList<ClientData> _waitingClients;
    uint _rounds;
};

int main(int argc, char *argv[]) {
    if(argc < 2) {
        cout << "Usage: " << argv[0] << " <numCaps>\n";
        return 1;
    }
    int numcaps = IStringStream::read_from<int>(argv[1]);
    CAPLOG("capbench-server [numcaps=" << numcaps << "]");
    CapHandler hdl(numcaps);
    srv = new Server<CapHandler>("captree-server", &hdl, nextlog2<4096>::val, nextlog2<128>::val, 8);
    if(Errors::occurred()) {
        CAPLOG("Error during server creation: " << Errors::to_string(Errors::last) <<
            " (" << Errors::last << ")");
        return 1;
    }
    env()->workloop()->run();
    delete srv;
    CAPLOG("Cap Server shut down . Return code: " << Errors::to_string(Errors::last));
    return 0;
}
