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

#include <m3/server/Server.h>
#include <m3/stream/Standard.h>

using namespace m3;

class MyHandler;

static Server<MyHandler> *srv;
static bool run = true;

class MyHandler : public Handler<> {
public:
    MyHandler() : Handler<>(), _count() {
    }

    virtual void handle_obtain(SessionData *, RecvBuf *, GateIStream &args, uint) override {
        reply_vmsg(args, Errors::NOT_SUP);
        if(++_count == 5)
            srv->shutdown();
    }
    virtual void handle_close(SessionData *sess, GateIStream &args) override {
        cout << "Client closed connection.\n";
        Handler<>::handle_close(sess, args);
    }
    virtual void handle_shutdown() override {
        cout << "Kernel wants to shut down.\n";
        run = false;
    }

private:
    int _count;
};

int main() {
    for(int i = 0; run && i < 10; ++i) {
        MyHandler hdl;
        srv = new Server<MyHandler>("srvtest-server", &hdl, nextlog2<512>::val, nextlog2<128>::val);
        if(Errors::occurred())
            break;
        env()->workloop()->run();
        delete srv;
    }
    return 0;
}
