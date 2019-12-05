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

#include <m3/com/GateStream.h>
#include <m3/server/RequestHandler.h>
#include <m3/server/Server.h>
#include <m3/session/Pipe.h>

#include "VarRingBuf.h"

class PipeReadHandler;
class PipeWriteHandler;

class PipeSessionData : public m3::RequestSessionData {
public:
    // unused
    explicit PipeSessionData()
        : m3::RequestSessionData(), flags(), reader(), writer(), rbuf(0) {
    }
    explicit PipeSessionData(size_t _memsize)
        : m3::RequestSessionData(), flags(), reader(), writer(), rbuf(_memsize) {
    }
    virtual ~PipeSessionData();

    uint flags;
    PipeReadHandler *reader;
    PipeWriteHandler *writer;
    VarRingBuf rbuf;
private:
    static int _nextid;
};

template<class SUB>
class PipeHandler {
public:
    using handler_func = void (SUB::*)(m3::GateIStream &is);

    struct RdWrRequest : public m3::SListItem {
        explicit RdWrRequest(size_t _amount, const m3::DTU::Message* _lastmsg)
            : m3::SListItem(), amount(_amount), lastmsg(_lastmsg) {
        }

        size_t amount;
        const m3::DTU::Message *lastmsg;
    };

    enum {
        READ_EOF    = 1,
        WRITE_EOF   = 2,
    };

    static const size_t BUFSIZE     = 512;
    static const size_t MSGSIZE     = 64;

    explicit PipeHandler(PipeSessionData *sess)
        : refs(1),
          _rbuf(m3::RecvBuf::create(m3::VPE::self().alloc_ep(), m3::nextlog2<BUFSIZE>::val,
            m3::nextlog2<MSGSIZE>::val, 0)),
          _rgate(m3::RecvGate::create(&_rbuf, sess)),
          _sgate(m3::SendGate::create(MSGSIZE, &_rgate)),
          _lastid(),
          _pending(),
          _callbacks() {
        using std::placeholders::_1;
        using std::placeholders::_2;
        _rgate.subscribe(std::bind(&PipeHandler<SUB>::handle_message, this, _1, _2));
    }
    ~PipeHandler() {
        m3::VPE::self().free_ep(_rbuf.epid());
    }

    m3::SendGate &sendgate() {
        return _sgate;
    }

    void add_operation(handler_func func) {
        _callbacks[0] = func;
    }

    void handle_message(m3::GateIStream &msg, m3::Subscriber<m3::GateIStream&> *) {
        EVENT_TRACER_handle_message();
        (static_cast<SUB*>(this)->*_callbacks[0])(msg);
    }

protected:
    int refs;
    m3::RecvBuf _rbuf;
    m3::RecvGate _rgate;
    m3::SendGate _sgate;
    int _lastid;
    m3::SList<RdWrRequest> _pending;

private:
    handler_func _callbacks[1];
};

class PipeReadHandler : public PipeHandler<PipeReadHandler> {
public:
    explicit PipeReadHandler(PipeSessionData *sess)
        : PipeHandler<PipeReadHandler>(sess), lastread() {
        add_operation(&PipeReadHandler::read);
    }

    m3::Errors::Code attach(PipeSessionData *sess);
    m3::Errors::Code close(PipeSessionData *sess, int id);

    void read(m3::GateIStream &is);
    void handle_pending_read(PipeSessionData *sess);

private:
    void append_request(PipeSessionData *sess, m3::GateIStream &is, size_t amount);

    size_t lastread;
};

class PipeWriteHandler : public PipeHandler<PipeWriteHandler> {
public:
    explicit PipeWriteHandler(PipeSessionData *sess)
        : PipeHandler<PipeWriteHandler>(sess), lastwrite() {
        add_operation(&PipeWriteHandler::write);
    }

    m3::Errors::Code attach(PipeSessionData *sess);
    m3::Errors::Code close(PipeSessionData *sess, int id);

    void write(m3::GateIStream &is);
    void handle_pending_write(PipeSessionData *sess);

private:
    void append_request(PipeSessionData *sess, m3::GateIStream &is, size_t amount);

    size_t lastwrite;
};
