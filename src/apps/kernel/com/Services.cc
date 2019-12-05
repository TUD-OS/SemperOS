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

#include "com/Services.h"
#include "SyscallHandler.h"

namespace kernel {

ServiceList ServiceList::_inst;
RemoteServiceList RemoteServiceList::_inst;

Service::~Service() {
    // we have allocated the selector and stored it in our cap-table on creation; undo that
    ServiceList::get().remove(this);
}

void ServiceList::send_and_receive(m3::Reference<Service> serv, const void *msg, size_t size, bool free) {
    // better use a new RecvGate here to not interfere with other syscalls
    RecvGate *rgate = new RecvGate(SyscallHandler::get().srvepid(), nullptr);
    rgate->subscribe([this, rgate, serv] (GateIStream &, m3::Subscriber<GateIStream&> *s) {
        m3::Reference<Service> srvcpy = serv;
        srvcpy->received_reply();
        // unsubscribe will delete the lambda
        RecvGate *rgatecpy = rgate;
        rgate->unsubscribe(s);
        delete rgatecpy;
    });

    serv->send(rgate, msg, size, free);
}

}
