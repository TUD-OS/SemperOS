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

#include <base/util/Profile.h>

#include "caphandler.h"

void CapHandler::handle_obtain(CapSessionData* sess, RecvBuf* rcvbuf, GateIStream& args, uint capcount) {
    if (!sess->send_gate()) {
        reqhandler_t::handle_obtain(sess, rcvbuf, args, capcount);
        return;
    }

    CAPLOGV("handle_obtain(count=" << capcount << ")");
    reply_vmsg(args, Errors::NO_ERROR, _lastCap);
}

void CapHandler::obtaincap() {
    CAPLOGV("obtaincap()");
    assert(_sess != nullptr);
    _lastCap = _sess->obtain(1);
}
