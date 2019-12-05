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

#include <m3/com/GateStream.h>
#include <m3/server/RequestHandler.h>
#include <m3/server/Server.h>

using namespace m3;

enum TestOp {
    TEST
};

class TestRequestHandler;

static Server<TestRequestHandler> *srv;

class TestRequestHandler : public RequestHandler<TestRequestHandler, TestOp, 1> {
public:
    explicit TestRequestHandler() : RequestHandler<TestRequestHandler, TestOp, 1>() {
        add_operation(TEST, &TestRequestHandler::test);
    }

    virtual size_t credits() override {
        return Server<TestRequestHandler>::DEF_MSGSIZE;
    }

    void test(GateIStream &is) {
        String str;
        is >> str;
        char *resp = new char[str.length() + 1];
        for(size_t i = 0; i < str.length(); ++i)
            resp[str.length() - i - 1] = str[i];
        reply_vmsg(is, String(resp,str.length()));
        delete[] resp;

        // pretend that we crash after some requests
        static int count = 0;
        if(++count == 6)
            srv->shutdown();
    }
};

int main() {
    srv = new Server<TestRequestHandler>("test", new TestRequestHandler());
    env()->workloop()->run();
    return 0;
}
