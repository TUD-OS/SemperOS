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

#include <base/Common.h>
#include <base/stream/OStringStream.h>
#include <base/util/Math.h>

#include <m3/com/MemGate.h>
#include <m3/com/SendGate.h>
#include <m3/com/GateStream.h>
#include <m3/VPE.h>
#include <m3/session/Session.h>
#include <m3/pipe/LocalDirectPipe.h>

#define DEBUG_LOCAL_PIPE  0
#if DEBUG_LOCAL_PIPE
#   include <base/stream/Serial.h>
#   define DBG_LOCAL_PIPE(expr)   Serial::get() << expr << "\n"
#else
#   define DBG_LOCAL_PIPE(...)
#endif

namespace m3 {

class PipeHandler;

class AggregateDirectPipe {
    friend class PipeHandler;
public:
    static const size_t MSG_SIZE        = 4096;
    // 448KB "should be enough space for everyone"
    static const size_t MSG_BUF_SIZE    = MSG_SIZE * 16;

//    class LDPWriter {
//    friend class LocalDirectPipe;
//        explicit LDPWriter(capsel_t msgcap, size_t size = MSG_BUF_SIZE);
//
//    public:
//        Errors::Code send(const void *data, size_t len) {
//            return _sgate.send(data, len);
//        }
//    private:
//        size_t _size;
//        SendGate _sgate;
//    };

    // use this on the server side, the pipe is created during session creation
    explicit AggregateDirectPipe(capsel_t msgcap, RecvBuf *rbuf, RecvGate *rgate, size_t size = MSG_BUF_SIZE);

    // use this on the client side
    explicit AggregateDirectPipe(String &srv, RecvBuf *rbuf, RecvGate *rgate,
        const GateOStream &args = StaticGateOStream<1>(),
        size_t size = MSG_BUF_SIZE);
    AggregateDirectPipe(const AggregateDirectPipe&) = delete;
    AggregateDirectPipe &operator=(const AggregateDirectPipe&) = delete;
    ~AggregateDirectPipe();

    /**
     * @return the receive endpoint
     */
    size_t receive_ep() const {
        return _recvep;
    }
    /**
     * @return the size of the shared memory area
     */
    size_t size() const {
        return _size;
    }

    SendGate *sgate() const {
        return _sgate;
    }
    RecvGate *rgate() const {
        return _rgate;
    }
    RecvBuf *rbuf() const {
        return _rbuf;
    }

    void create_writer() {
        assert(_writer == nullptr);
        assert(_msgcap != ObjCap::INVALID);
        _writer = new LocalDirectPipe::LDPWriter(_msgcap, _size);
    }
    LocalDirectPipe::LDPWriter *writer() const {
        return _writer;
    }

    GateIStream wait() {
        m3::DTU::Message *msg;
        DBG_LOCAL_PIPE("Waiting on EP " << _rgate->epid());
        // why is there a message?
        DTU::get().wait();
        Errors::Code res = _rgate->wait(_sgate, &msg);
        return GateIStream(*_rgate, msg, res);
    }

    DTU::Message *fetch_msg() {
        return DTU::get().fetch_msg(_rgate->epid());
    }

    void mark_read(const DTU::Message *msg) {
        DTU::get().mark_read(_rgate->epid(), DTU::get().get_msgoff(_rgate->epid(), msg));
    }

private:

    size_t _recvep;
    size_t _size;
    capsel_t _msgcap;
    SendGate *_sgate;
    // contains the gate granted to us by the other side
    LocalDirectPipe::LDPWriter *_writer;
    RecvBuf *_rbuf;
    RecvGate *_rgate;
};

}
