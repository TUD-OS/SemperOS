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

#pragma once

#include <base/Common.h>

#include <m3/server/LocalPipeServer.h>
#include <m3/stream/Standard.h>
#include <m3/pipe/LocalDirectPipe.h>

#define DEBUG_PIPE_SERVER  0
#if DEBUG_PIPE_SERVER
#   include <base/stream/Serial.h>
#   define DBG_PIPE_SERVER(expr)   Serial::get() << expr << "\n"
#else
#   define DBG_PIPE_SERVER(...)
#endif

namespace m3 {

class LocalDirectPipeSessionData : public SessionData {
public:
    explicit LocalDirectPipeSessionData() : SessionData(), _pipe() {
    }
    ~LocalDirectPipeSessionData() {
    }

    LocalDirectPipe *pipe() const {
        return _pipe;
    }
    void pipe(LocalDirectPipe *pipe) {
        _pipe = pipe;
    }
protected:
    LocalDirectPipe *_pipe;
};

class PipeHandler : public Handler<LocalDirectPipeSessionData> {
public:
    PipeHandler() : Handler<LocalDirectPipeSessionData>() {
    }

    virtual session_type *handle_open(GateIStream &args) override;
    virtual void handle_obtain(session_type *sess, RecvBuf *, GateIStream &args, uint) override;
    virtual void handle_delegate(session_type *sess, GateIStream &args, uint) override;
    virtual void handle_close(session_type *sess, GateIStream &args) override {
        DBG_PIPE_SERVER("Pipe client closed connection");
        Handler<LocalDirectPipeSessionData>::handle_close(sess, args);

        // shut down the server - we need a special server for that
        DBG_PIPE_SERVER("Initiating pipe server shutdown");
        _server->shutdown();
        env()->workloop()->stop();
    }
    virtual void handle_shutdown() override {
        DBG_PIPE_SERVER("Kernel wants to shut down");
    }

    LocalDirectPipe *pipe() {
        return _pipe;
    }

    LocalPipeServer<PipeHandler> *_server;
private:
    LocalDirectPipe *_pipe;
};
}