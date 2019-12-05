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
#include <base/Panic.h>

#include <m3/pipe/AggregateDirectPipe.h>
#include <m3/session/Session.h>
#include <m3/stream/Standard.h>
#include <m3/Syscalls.h>

//m3::RecvBuf *m3::AggregateDirectPipe::_rbuf = nullptr;
//m3::RecvGate *m3::AggregateDirectPipe::_rgate = nullptr;

namespace m3 {

//LocalDirectPipe::LDPWriter::LDPWriter(capsel_t msgcap, size_t size)
//    : _size(size), _sgate(SendGate::bind(msgcap)) {
//}

AggregateDirectPipe::AggregateDirectPipe(capsel_t msgcap, RecvBuf *rbuf, RecvGate *rgate, size_t size)
    : _recvep(VPE::self().alloc_ep()), _size(size), _msgcap(msgcap),
      _sgate(), _writer(), _rbuf(rbuf), _rgate(rgate) {
    assert(size % MSG_BUF_SIZE == 0);
    // initialize global receive buffer for first pipe
    if(!_rbuf) {
        _rbuf = new RecvBuf(VPE::self().alloc_ep(), RecvBuf::allocate(1UL << getnextlog2(size)),
                getnextlog2(size), nextlog2<MSG_SIZE>::val, RecvBuf::DELETE_BUF);
        _rgate = new RecvGate(_rbuf, nullptr);
    }
    else if(static_cast<size_t>(_rbuf->order()) != getnextlog2(size)) {
        cout << "Warning: Requested size for pipe does not match initialized size (" <<
                getnextlog2(size) << " != " << _rbuf->order() << "\n";
    }
    // create sendgate
    _sgate = new SendGate(VPE::self().alloc_ep(), SendGate::KEEP_SEL, _rgate);
    Syscalls::get().creategate(VPE::self().sel(), _sgate->sel(), _rgate->label(), _rgate->epid(), MSG_SIZE);
    if(Errors::occurred())
        PANIC("Could not create send gate for pipe");
}

// TODO args should contain the size
AggregateDirectPipe::AggregateDirectPipe(String &srv, RecvBuf *rbuf, RecvGate *rgate,
        const GateOStream &args, size_t size)
    : _recvep(VPE::self().alloc_ep()), _size(size), _msgcap(ObjCap::INVALID),
      _sgate(), _writer(), _rbuf(rbuf), _rgate(rgate) {
    assert(size % MSG_SIZE == 0);

    // initialize global receive buffer for first pipe
    if(!_rbuf) {
        _rbuf = new RecvBuf(VPE::self().alloc_ep(), RecvBuf::allocate(1UL << getnextlog2(size)),
                getnextlog2(size), nextlog2<MSG_SIZE>::val, RecvBuf::DELETE_BUF);
        _rgate = new RecvGate(_rbuf, nullptr);
    }
    else if(static_cast<size_t>(_rbuf->order()) != getnextlog2(size)) {
        cout << "Warning: Requested size for pipe does not match initialized size (" <<
                getnextlog2(size) << " != " << _rbuf->order() << "\n";
    }
    // create sendgate
    _sgate = new SendGate(VPE::self().alloc_cap(), SendGate::KEEP_SEL, _rgate);
    Syscalls::get().creategate(VPE::self().sel(), _sgate->sel(), _sgate->label(), _rgate->epid(), MSG_SIZE);
    if(Errors::occurred())
        PANIC("Could not create send gate for pipe");

    Session *sess = new Session(srv, args);

    // delegate our send gate to the server side
    sess->delegate(CapRngDesc(CapRngDesc::OBJ, _sgate->sel()));
    if(Errors::last != Errors::NO_ERROR)
        cerr << "Delegating local pipe's receive gate failed!\n";

    // obtain the send gate from the server side
    capsel_t msgcap = sess->obtain(1).start();
    if(Errors::last != Errors::NO_ERROR)
        cerr << "Creating local pipe's send gate failed!\n";
    _writer = new LocalDirectPipe::LDPWriter(msgcap, size);

    // close session so the server exits
//    sess->close();
    delete sess;
}

AggregateDirectPipe::~AggregateDirectPipe() {
    // TODO
    // cleanup
}




}
