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

#include <m3/server/Handler.h>
#include <m3/com/GateStream.h>
#include <m3/VPE.h>

namespace m3 {

class EventHandler;

class EventSessionData : public SessionData {
    friend class EventHandler;
};

class EventHandler : public Handler<SessionData> {
    template<class HDL>
    friend class Server;

public:
    template<typename... Args>
    void broadcast(const Args &... args) {
        auto msg = create_vmsg(args...);
        for(auto &h : *this)
            send_msg(*static_cast<SendGate*>(h.send_gate()), msg.bytes(), msg.total());
    }

protected:
    virtual void handle_delegate(SessionData *sess, GateIStream &args, uint capcount) override {
        // TODO like in RequestHandler, don't check additional argument size
        if(sess->send_gate() || /*args.remaining() > 0 || */capcount != 1) {
            reply_vmsg(args, Errors::INV_ARGS);
            return;
        }

        sess->_sgate = new SendGate(SendGate::bind(VPE::self().alloc_cap(), 0));
        reply_vmsg(args, Errors::NO_ERROR, CapRngDesc(CapRngDesc::OBJ, sess->send_gate()->sel()));
    }

private:
    virtual size_t credits() {
        return SendGate::UNLIMITED;
    }
};

}
