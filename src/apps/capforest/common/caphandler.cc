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
        // when all links are connected start signal them to start obtaining caps
        if(_isRoot)
            for(uint i = 0; i < static_cast<uint>(_numSessions); i++)
                _sess[i]->signal();
        return;
    }

    CAPLOGV("handle_obtain(count=" << capcount << ") handed out before: " << _handedOut);
    reply_vmsg(args, Errors::NO_ERROR, _lastCap);
    if(_isRoot) {
        _handedOut++;
    }
    else {
        // links send acknowlegement when obtains are finished
        _sess[0]->obtainDone();
    }
}

void CapHandler::signal(GateIStream&) {
    CAPLOGV("signal()");
    if (_isRoot) {

        // check if the chain reached the desired length
//        if (_handedOut == _numcaps - 1) {
//            revcap();
//            _revokeTime = _revokeTime / _revokes;
//            CAPLOGV("Revokes handled: " << _revokes);
//            CAPLOG("Avg. server revoke time: " << _revokeTime);
//            Syscalls::get().exit(0);
//            return;
//        }
    } else {
        if (!_sess) {
            _sess = static_cast<CapServerSession**> (m3::Heap::alloc(sizeof (CapServerSession*)));
            _sess[0] = new CapServerSession("capforest-root");
            if (Errors::occurred()) {
                CAPLOG("Unable to create sess at capforest-root server. Error= " << Errors::last <<
                    " (" << Errors::to_string(Errors::last) << ").");
                return;
            }
            _numSessions++;
        } else {
            // obtain capability
            obtaincap();
            _obtained++;

            // only supporting one layer forests
            // signal causes the other server to issue the next obtain
//            if (_obtained <= _numcaps) {
//                _sess[0]->signal();
//            }
        }
    }
}

void CapHandler::obtaincap() {
    CAPLOGV("obtaincap()");
    assert(_sess != nullptr);
    _lastCap = _sess[0]->obtain(1);
}
