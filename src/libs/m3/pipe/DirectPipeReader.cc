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

#include <m3/pipe/DirectPipe.h>
#include <m3/pipe/DirectPipeReader.h>

namespace m3 {

DirectPipeReader::State::State(capsel_t caps, size_t rep)
    : _mgate(MemGate::bind(caps)),
      _rbuf(RecvBuf::create(rep,
        nextlog2<DirectPipe::MSG_BUF_SIZE>::val, nextlog2<DirectPipe::MSG_SIZE>::val, 0)),
      _rgate(RecvGate::create(&_rbuf)),
      _pos(), _rem(), _pkglen(-1), _eof(0), _is(_rgate, nullptr) {
}

DirectPipeReader::DirectPipeReader(capsel_t caps, size_t rep, State *state)
    : _noeof(), _caps(caps), _rep(rep), _state(state) {
}

DirectPipeReader::~DirectPipeReader() {
    send_eof();
    delete _state;
}

void DirectPipeReader::send_eof() {
    if(_noeof)
        return;

    if(!_state)
        _state = new State(_caps, _rep);
    if(~_state->_eof & DirectPipe::READ_EOF) {
        // if we have not fetched a message yet, do so now
        if(_state->_pkglen == static_cast<size_t>(-1))
            _state->_is = receive_vmsg(_state->_rgate, _state->_pos, _state->_pkglen);
        DBG_PIPE("[read] replying len=0\n");
        reply_vmsg(_state->_is, (size_t)0);
        _state->_eof |= DirectPipe::READ_EOF;
    }
}

ssize_t DirectPipeReader::read(void *buffer, size_t count) {
    if(!_state)
        _state = new State(_caps, _rep);
    if(_state->_eof)
        return 0;

    assert((reinterpret_cast<uintptr_t>(buffer) & (DTU_PKG_SIZE - 1)) == 0);
    assert((count & (DTU_PKG_SIZE - 1)) == 0);
    if(_state->_rem == 0) {
        if(_state->_pos > 0) {
            DBG_PIPE("[read] replying len=" << _state->_pkglen << "\n");
            reply_vmsg(_state->_is, _state->_pkglen);
            _state->_is.finish();
        }
        _state->_is = receive_vmsg(_state->_rgate, _state->_pos, _state->_pkglen);
        _state->_rem = _state->_pkglen;
    }

    size_t amount = Math::min(count, _state->_rem);
    DBG_PIPE("[read] read from pos=" << _state->_pos << ", len=" << amount << "\n");
    if(amount == 0)
        _state->_eof |= DirectPipe::WRITE_EOF;
    else {
        size_t aligned_amount = Math::round_up(amount, DTU_PKG_SIZE);
        _state->_mgate.read_sync(buffer, aligned_amount, _state->_pos);
        _state->_pos += aligned_amount;
        _state->_rem -= amount;
    }
    return amount;
}

size_t DirectPipeReader::serialize_length() {
    return ostreamsize<capsel_t, size_t>();
}

void DirectPipeReader::delegate(VPE &vpe) {
    vpe.delegate(CapRngDesc(CapRngDesc::OBJ, _caps, 1));
}

void DirectPipeReader::serialize(Marshaller &m) {
    // we can't share the reader between two VPEs atm anyway, so don't serialize the current state
    m << _caps << _rep;
}

File *DirectPipeReader::unserialize(Unmarshaller &um) {
    capsel_t caps;
    size_t rep;
    um >> caps >> rep;
    return new DirectPipeReader(caps, rep, nullptr);
}

}
