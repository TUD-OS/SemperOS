/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 *  This file is part of SemperOS.
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

#include <m3/pipe/AggregatePipeHandler.h>

namespace m3 {

AggregatePipeHandler::session_type *AggregatePipeHandler::handle_open(GateIStream &args) {
    DBG_PIPE_SERVER("Opening Pipe Session");
    session_type *s = add_session(new session_type());
    // TODO
    // read size from args
    reply_vmsg(args, Errors::NO_ERROR, s);
    return s;
}

void AggregatePipeHandler::handle_obtain(session_type *, RecvBuf *, GateIStream &args, uint) {
    DBG_PIPE_SERVER("Handing out pipe capability " << pipe()->sgate()->sel());
    pipe()->create_writer();
    // hand out pipe capability
    reply_vmsg(args, Errors::NO_ERROR, CapRngDesc(CapRngDesc::OBJ,
            pipe()->sgate()->sel()));
}

void AggregatePipeHandler::handle_delegate(session_type *, GateIStream &args, uint) {
    // receive capability to client's send gate
    capsel_t msgcap = VPE::self().alloc_cap();
    _pipe = new AggregateDirectPipe(msgcap, nullptr, nullptr);
    DBG_PIPE_SERVER("Receiving client's send gate @ " << msgcap);
    reply_vmsg(args, Errors::NO_ERROR, CapRngDesc(CapRngDesc::OBJ, msgcap));
}

}
