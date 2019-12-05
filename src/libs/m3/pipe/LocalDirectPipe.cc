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

#include <m3/pipe/LocalDirectPipe.h>
#include <m3/pipe/DirectPipeReader.h>
#include <m3/pipe/DirectPipeWriter.h>
#include <m3/vfs/FileTable.h>
#include <m3/session/Session.h>
#include <m3/stream/Standard.h>

#include <m3/pipe/AggregateDirectPipe.h>

namespace m3 {

LocalDirectPipe::LDPWriter::LDPWriter(capsel_t msgcap, size_t size)
    : _size(size), _sgate(SendGate::bind(msgcap)) {
}

LocalDirectPipe::LocalDirectPipe(capsel_t msgcap, Session *sess, size_t size)
    : _recvep(VPE::self().alloc_ep()), _size(size), _msgcap(msgcap),
      _rbuf(RecvBuf::create(VPE::self().alloc_ep(), getnextlog2(size), nextlog2<MSG_SIZE>::val, RecvBuf::NONE)),
      _rgate(RecvGate::create(&_rbuf, sess)),
        /* HACK: Assuming an aggregate pipe on the other end */
      _sgate(SendGate::create(AggregateDirectPipe::MSG_SIZE, &_rgate)) {
    assert(size % MSG_BUF_SIZE == 0);
}

// TODO args should contain the size
LocalDirectPipe::LocalDirectPipe(String &srv, const GateOStream &args, size_t size)
    : _recvep(VPE::self().alloc_ep()), _size(size), _msgcap(ObjCap::INVALID),
      _rbuf(RecvBuf::create(VPE::self().alloc_ep(), getnextlog2(size), nextlog2<MSG_SIZE>::val, RecvBuf::NONE)),
      _rgate(RecvGate::create(&_rbuf, static_cast<void*>(new Session(srv, args)))),
      _sgate(SendGate::create(MSG_SIZE, &_rgate)) {
    assert(size % MSG_SIZE == 0);

    // delegate our send gate to the server side
    _rgate.session<Session>()->delegate(CapRngDesc(CapRngDesc::OBJ, _sgate.sel()));
    if(Errors::last != Errors::NO_ERROR)
        cerr << "Delegating local pipe's receive gate failed!\n";

    // obtain the send gate from the server side
    capsel_t msgcap = _rgate.session<Session>()->obtain(1).start();
    if(Errors::last != Errors::NO_ERROR)
        cerr << "Creating local pipe's send gate failed!\n";
    _writer = new LDPWriter(msgcap, size);

    // close session so the server exits
//    _rgate.session<Session>()->close();
    delete _rgate.session<Session>();
}

LocalDirectPipe::~LocalDirectPipe() {
    // TODO
    // cleanup
}




}
