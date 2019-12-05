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

#include <base/tracing/Tracing.h>
#include <base/Errors.h>

#include <m3/com/GateStream.h>
#include <m3/com/SendGate.h>
#include <m3/com/RecvGate.h>
#include <m3/server/Handler.h>

namespace m3 {

template<typename CLS, typename OP, size_t OPCNT, class SESS>
class RequestHandler;

class RequestSessionData : public SessionData {
    template<typename CLS, typename OP, size_t OPCNT, class SESS>
    friend class RequestHandler;
};

template<typename CLS, typename OP, size_t OPCNT, class SESS = RequestSessionData>
class RequestHandler : public Handler<SESS> {
    template<class HDL>
    friend class Server;

    using handler_func = void (CLS::*)(GateIStream &is);

public:
    void add_operation(OP op, handler_func func) {
        _callbacks[op] = func;
    }

protected:
    virtual void handle_obtain(SESS *sess, RecvBuf *ctrlbuf, GateIStream &args,
                               uint capcount) override {
        using std::placeholders::_1;
        using std::placeholders::_2;
        // TODO can't check that atm, because messages are size-aligned. so we don't know whether
        // there are actually more arguments or it's just padding
        if(sess->send_gate() || /*args.remaining() > 0 || */capcount != 1) {
            reply_vmsg(args, Errors::INV_ARGS);
            return;
        }

        sess->_rgate = new RecvGate(RecvGate::create(recvbuf(ctrlbuf), sess));
        sess->_sgate = new SendGate(SendGate::create(credits(), sess->_rgate));
        sess->_rgate->subscribe(std::bind(&RequestHandler::handle_message, this, _1, _2));
        reply_vmsg(args, Errors::NO_ERROR, CapRngDesc(CapRngDesc::OBJ, sess->send_gate()->sel()));
    }

public:
    void handle_message(GateIStream &msg, Subscriber<GateIStream&> *) {
        EVENT_TRACER_handle_message();
        OP op;
        msg >> op;
        if(static_cast<size_t>(op) < sizeof(_callbacks) / sizeof(_callbacks[0])) {
            (static_cast<CLS*>(this)->*_callbacks[op])(msg);
            return;
        }
        reply_vmsg(msg, Errors::INV_ARGS);
    }

private:
    virtual RecvBuf *recvbuf(RecvBuf *ctrlbuf) {
        return ctrlbuf;
    }
    virtual size_t credits() {
        return SendGate::UNLIMITED;
    }

    handler_func _callbacks[OPCNT];
};

}
