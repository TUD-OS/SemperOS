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

#include <base/log/Lib.h>
#include <base/Errors.h>
#include <base/KIF.h>
#include <base/Panic.h>

#include <m3/com/RecvGate.h>
#include <m3/server/Handler.h>
#include <m3/Syscalls.h>
#include <m3/VPE.h>

#include <algorithm>

namespace m3 {

template<class HDL>
class Server : public ObjCap {
    using handler_func = void (Server::*)(GateIStream &is);

public:
    static constexpr size_t DEF_BUFSIZE     = 8192;
    static constexpr size_t DEF_MSGSIZE     = 256;
    static const size_t MAX_RECVBUFS        = 9;
    static const int MAX_KRNL_MSGS          = 1;

    explicit Server(const String &name, HDL *handler, int buford = nextlog2<DEF_BUFSIZE>::val,
                    int msgord = nextlog2<DEF_MSGSIZE>::val, uint recvbufs = 1)
        : ObjCap(SERVICE, VPE::self().alloc_cap()), _handler(handler), _ctrl_handler(),
            _recv_bufs(recvbufs), _used_slots{0},
          _epid{VPE::self().alloc_ep(), VPE::self().alloc_ep(), VPE::self().alloc_ep(),
                VPE::self().alloc_ep(), VPE::self().alloc_ep(), VPE::self().alloc_ep(),
                VPE::self().alloc_ep(), VPE::self().alloc_ep(), VPE::self().alloc_ep()},
            _rcvbuf{RecvBuf::create(_epid[0], buford, msgord, 0), RecvBuf::create(_epid[1], buford, msgord, 0),
                RecvBuf::create(_epid[2], buford, msgord, 0), RecvBuf::create(_epid[3], buford, msgord, 0),
                RecvBuf::create(_epid[4], buford, msgord, 0), RecvBuf::create(_epid[5], buford, msgord, 0),
                RecvBuf::create(_epid[6], buford, msgord, 0), RecvBuf::create(_epid[7], buford, msgord, 0),
                RecvBuf::create(_epid[8], buford, msgord, 0)},
            _ctrl_rgate{RecvGate::create(&_rcvbuf[0]), RecvGate::create(&_rcvbuf[1]), RecvGate::create(&_rcvbuf[2]),
                RecvGate::create(&_rcvbuf[3]), RecvGate::create(&_rcvbuf[4]), RecvGate::create(&_rcvbuf[5]),
                RecvGate::create(&_rcvbuf[6]), RecvGate::create(&_rcvbuf[7]), RecvGate::create(&_rcvbuf[8])},
            _ctrl_sgate{SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[0]), SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[1]),
                SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[2]), SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[3]),
                SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[4]), SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[5]),
                SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[6]), SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[7]),
                SendGate::create(DEF_MSGSIZE, &_ctrl_rgate[8])} {
        LLOG(SERV, "create(" << name << ")");
        if(_recv_bufs > MAX_RECVBUFS)
            PANIC("Server requires too many recieve buffers (" << _recv_bufs << " max: " << MAX_RECVBUFS);
        Syscalls::get().createsrv(_ctrl_sgate[0].sel(), sel(), name);

        using std::placeholders::_1;
        using std::placeholders::_2;
        for(uint i = 0; i < _recv_bufs; i++)
            _ctrl_rgate[i].subscribe(std::bind(&Server::handle_message, this, _1, _2));

        _ctrl_handler[KIF::Service::OPEN] = &Server::handle_open;
        _ctrl_handler[KIF::Service::OBTAIN] = &Server::handle_obtain;
        _ctrl_handler[KIF::Service::DELEGATE] = &Server::handle_delegate;
        _ctrl_handler[KIF::Service::CLOSE] = &Server::handle_close;
        _ctrl_handler[KIF::Service::SHUTDOWN] = &Server::handle_shutdown;
    }
    ~Server() {
        LLOG(SERV, "destroy()");
        // if it fails, there are pending requests. this might happen multiple times because
        // the kernel might have them still in the send-queue.
        CapRngDesc caps(CapRngDesc::OBJ, sel());
        while(Syscalls::get().revoke(caps) == Errors::MSGS_WAITING) {
            // handle all requests
            LLOG(SERV, "handling pending requests...");
            for(uint i = 0; i < _recv_bufs; i++) {
                DTU::Message *msg;
                while((msg = DTU::get().fetch_msg(_ctrl_rgate[i].epid()))) {
                    GateIStream is(_ctrl_rgate[i], msg, Errors::NO_ERROR);
                    handle_message(is, nullptr);
                }
            }
        }
        // don't revoke it again
        flags(ObjCap::KEEP_CAP);
        // free endpoint
        for(uint i = 0; i < MAX_RECVBUFS; i++)
            VPE::self().free_ep(_epid[i]);
    }

    void shutdown() {
        // by disabling the receive buffer, we remove it from the WorkLoop, which in the end
        // stops the WorkLoop
        for(uint i = 0; i < MAX_RECVBUFS; i++)
            _rcvbuf[i].disable();
    }

    HDL &handler() {
        return *_handler;
    }

private:
    void handle_message(GateIStream &msg, Subscriber<GateIStream&> *) {
        KIF::Service::Command op;
        msg >> op;
        if(static_cast<size_t>(op) < ARRAY_SIZE(_ctrl_handler)) {
            (this->*_ctrl_handler[op])(msg);
            return;
        }
        reply_vmsg(msg, Errors::INV_ARGS);
    }

    void handle_open(GateIStream &is) {
        EVENT_TRACER_Service_open();

        word_t sessptr = reinterpret_cast<word_t>(_handler->handle_open(is));

        LLOG(SERV, fmt(sessptr, "#x") << ": open()");
    }

    void handle_obtain(GateIStream &is) {
        EVENT_TRACER_Service_obtain();
        word_t sessptr;
        uint capcount;
        is >> sessptr >> capcount;

        LLOG(SERV, fmt(sessptr, "#x") << ": obtain(caps=" << capcount << ")");

        typename HDL::session_type *sess = reinterpret_cast<typename HDL::session_type*>(sessptr);
        uint i;
        if(!sess->send_gate()) {
            for(i = 0; i < _recv_bufs; i++)
                // Note: This assumes that each client (including the kernel)
                //       is only allowed to send one msg at a time.
                // Keep one msg slot at the first EP free for communication with the kernel
                if((!i && _used_slots[i] < DTU::MAX_MSG_SLOTS - MAX_KRNL_MSGS)
                        || (i && _used_slots[i] < DTU::MAX_MSG_SLOTS))
                    break;
            if(i == _recv_bufs)
                PANIC("No further clients can be accepted, all msg slots used");
            _used_slots[i]++;
            _handler->handle_obtain(sess, &_rcvbuf[i], is, capcount);

            // free slot if session creation failed
            if(!sess->send_gate())
                _used_slots[i]--;
        }
        else
            _handler->handle_obtain(sess, sess->recv_gate()->buffer(), is, capcount);
    }

    void handle_delegate(GateIStream &is) {
        EVENT_TRACER_Service_delegate();
        word_t sessptr;
        uint capcount;
        is >> sessptr >> capcount;

        LLOG(SERV, fmt(sessptr, "#x") << ": delegate(caps=" << capcount << ")");

        typename HDL::session_type *sess = reinterpret_cast<typename HDL::session_type*>(sessptr);
        _handler->handle_delegate(sess, is, capcount);
    }

    void handle_close(GateIStream &is) {
        EVENT_TRACER_Service_close();
        word_t sessptr;
        is >> sessptr;

        LLOG(SERV, fmt(sessptr, "#x") << ": close()");

        typename HDL::session_type *sess = reinterpret_cast<typename HDL::session_type*>(sessptr);
        // TODO
        // Handing out endpoints twice doesn't work at the moment.
        // If we do so, messages get lost, but I don't know why.
//        _used_slots[sess->rgateIdx()]--;
        _handler->handle_close(sess, is);
    }

    void handle_shutdown(GateIStream &is) {
        EVENT_TRACER_Service_shutdown();

        LLOG(SERV, "shutdown()");

        _handler->handle_shutdown();
        shutdown();
        reply_vmsg(is, Errors::NO_ERROR);
    }

protected:
    HDL *_handler;
    handler_func _ctrl_handler[5];
    uint _recv_bufs;
    uint _used_slots[MAX_RECVBUFS];
    // store a copy of the endpoint-id. the RecvBuf sets it to UNBOUND on detach().
    size_t _epid[MAX_RECVBUFS];
    RecvBuf _rcvbuf[MAX_RECVBUFS];
    RecvGate _ctrl_rgate[MAX_RECVBUFS];
    SendGate _ctrl_sgate[MAX_RECVBUFS];
};

}
