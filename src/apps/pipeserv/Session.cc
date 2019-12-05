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

#include <base/log/Services.h>

#include "Session.h"

using namespace m3;

template<typename... Args>
static void reply_vmsg_late(RecvGate &gate, const DTU::Message *msg,
        const Args &... args) {
    auto reply = create_vmsg(args...);
    size_t idx = DTU::get().get_msgoff(gate.epid(), msg);
    gate.reply_sync(reply.bytes(), reply.total(), idx);
}

int PipeSessionData::_nextid = 0;

PipeSessionData::~PipeSessionData() {
    delete reader;
    delete writer;
}

Errors::Code PipeReadHandler::attach(PipeSessionData *sess) {
    refs++;
    SLOG(PIPE, fmt((word_t)sess, "#x") << ": attach: read-refs=" << refs);
    return Errors::NO_ERROR;
}

Errors::Code PipeReadHandler::close(PipeSessionData *sess, int id) {
    if(sess->flags & READ_EOF)
        return Errors::INV_ARGS;

    if(lastread && _lastid == id) {
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": read-pull: " << lastread << " [" << id << "]");
        sess->rbuf.pull(lastread);
        lastread = 0;
    }

    if(--refs > 0) {
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": close: read-refs=" << refs);
        handle_pending_read(sess);
        sess->writer->handle_pending_write(sess);
        return Errors::NO_ERROR;
    }

    sess->flags |= READ_EOF;
    SLOG(PIPE, fmt((word_t)sess, "#x") << ": close: read end");

    sess->writer->handle_pending_write(sess);
    return Errors::NO_ERROR;
}

void PipeReadHandler::read(GateIStream &is) {
    PipeSessionData *sess = is.gate().session<PipeSessionData>();
    size_t amount;
    int id;
    is >> amount >> id;

    if(lastread) {
        if(_lastid != id) {
            append_request(sess, is, amount);
            return;
        }

        SLOG(PIPE, fmt((word_t)sess, "#x") << ": read-pull: " << lastread << " [" << id << "]");
        sess->rbuf.pull(lastread);
        lastread = 0;

        sess->writer->handle_pending_write(sess);
    }

    if(_pending.length() > 0) {
        handle_pending_read(sess);
        if(!(sess->flags & WRITE_EOF)) {
            append_request(sess, is, amount);
            return;
        }
    }

    ssize_t pos = sess->rbuf.get_read_pos(&amount);
    if(pos == -1) {
        if(sess->flags & WRITE_EOF) {
            SLOG(PIPE, fmt((word_t)sess, "#x") << ": read: " << amount << " EOF");
            amount = 0;
            reply_vmsg(is, Errors::NO_ERROR, pos, amount, 0);
        }
        else
            append_request(sess, is, amount);
    }
    else {
        _lastid++;
        lastread = amount;
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": read: " << amount << " @" << pos
            << " [" << _lastid << "]");
        reply_vmsg(is, Errors::NO_ERROR, pos, amount, _lastid);
    }
}

void PipeReadHandler::append_request(PipeSessionData *sess, GateIStream &is, size_t amount) {
    SLOG(PIPE, fmt((word_t)sess, "#x") << ": read: " << amount << " waiting");
    _pending.append(new RdWrRequest(amount, &is.message()));
    is.claim();
}

void PipeReadHandler::handle_pending_read(PipeSessionData *sess) {
    while(_pending.length() > 0) {
        RdWrRequest *req = &*_pending.begin();
        size_t ramount = req->amount;
        ssize_t rpos = sess->rbuf.get_read_pos(&ramount);
        if(rpos != -1) {
            _lastid++;
            lastread = ramount;
            SLOG(PIPE, fmt((word_t)sess, "#x") << ": late-read: " << ramount << " @" << rpos
                << " [" << _lastid << "]");
            reply_vmsg_late(_rgate, req->lastmsg, Errors::NO_ERROR, rpos, ramount, _lastid);
            delete _pending.remove_first();
            break;
        }
        else if(sess->flags & WRITE_EOF) {
            SLOG(PIPE, fmt((word_t)sess, "#x") << ": late-read: EOF");
            reply_vmsg_late(_rgate, req->lastmsg, Errors::NO_ERROR, (size_t)0, (size_t)0, 0);
            delete _pending.remove_first();
        }
        else
            break;
    }
}

Errors::Code PipeWriteHandler::attach(PipeSessionData *sess) {
    refs++;
    SLOG(PIPE, fmt((word_t)sess, "#x") << ": attach: write-refs=" << refs);
    return Errors::NO_ERROR;
}

Errors::Code PipeWriteHandler::close(PipeSessionData *sess, int id) {
    if(sess->flags & WRITE_EOF)
        return Errors::INV_ARGS;

    if(lastwrite && _lastid == id) {
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": write-push: " << lastwrite << " [" << id << "]");
        sess->rbuf.push(lastwrite);
        lastwrite = 0;
    }

    if(--refs > 0) {
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": close: write-refs=" << refs);
        handle_pending_write(sess);
        sess->reader->handle_pending_read(sess);
        return Errors::NO_ERROR;
    }

    sess->flags |= WRITE_EOF;
    SLOG(PIPE, fmt((word_t)sess, "#x") << ": close: write end");

    sess->reader->handle_pending_read(sess);
    return Errors::NO_ERROR;
}

void PipeWriteHandler::write(GateIStream &is) {
    PipeSessionData *sess = is.gate().session<PipeSessionData>();
    size_t amount;
    int id;
    is >> amount >> id;

    if(sess->flags & READ_EOF) {
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": write: " << amount << " EOF");
        reply_vmsg(is, Errors::END_OF_FILE);
        return;
    }

    if(lastwrite) {
        if(_lastid != id) {
            append_request(sess, is, amount);
            return;
        }

        SLOG(PIPE, fmt((word_t)sess, "#x") << ": write-push: " << lastwrite << " [" << id << "]");
        sess->rbuf.push(lastwrite);
        lastwrite = 0;

        sess->reader->handle_pending_read(sess);
    }

    if(_pending.length() > 0) {
        handle_pending_write(sess);
        append_request(sess, is, amount);
        return;
    }

    ssize_t pos = sess->rbuf.get_write_pos(amount);
    if(pos == -1)
        append_request(sess, is, amount);
    else {
        lastwrite = amount;
        _lastid++;
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": write: " << amount
            << " @" << pos << " [" << _lastid << "]");
        reply_vmsg(is, Errors::NO_ERROR, pos, _lastid);
    }
}

void PipeWriteHandler::append_request(PipeSessionData *sess, GateIStream &is, size_t amount) {
    SLOG(PIPE, fmt((word_t)sess, "#x") << ": write: " << amount << " waiting");
    _pending.append(new RdWrRequest(amount, &is.message()));
    is.claim();
}

void PipeWriteHandler::handle_pending_write(PipeSessionData *sess) {
    if(sess->flags & READ_EOF) {
        SLOG(PIPE, fmt((word_t)sess, "#x") << ": late-write: EOF");
        while(_pending.length() > 0) {
            RdWrRequest *req = _pending.remove_first();
            reply_vmsg_late(_rgate, req->lastmsg, Errors::END_OF_FILE);
            delete req;
        }
    }
    else if(_pending.length() > 0) {
        RdWrRequest *req = &*_pending.begin();
        ssize_t wpos = sess->rbuf.get_write_pos(req->amount);
        if(wpos != -1) {
            lastwrite = req->amount;
            _lastid++;
            SLOG(PIPE, fmt((word_t)sess, "#x") << ": late-write: " << req->amount
                << " @" << wpos << " [" << _lastid << "]");
            reply_vmsg_late(_rgate, req->lastmsg, Errors::NO_ERROR, wpos, _lastid);
            _pending.remove_first();
            delete req;
        }
    }
}
