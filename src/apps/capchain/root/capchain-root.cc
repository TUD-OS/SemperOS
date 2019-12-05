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
#include <base/stream/IStringStream.h>
#include <base/util/Profile.h>
#include <base/benchmark/capbench.h>

#include <m3/server/Server.h>
#include <m3/server/RequestHandler.h>
#include <m3/stream/Standard.h>

#include "../common/operations.h"
#include "../common/caphandler.h"

using namespace m3;

int main(int argc, char *argv[]) {
    if(argc < 2) {
        cout << "Usage: " << argv[0] << " <numCaps>\n";
        return 1;
    }

    // create session
    CapServerSession *sess = new CapServerSession("capchain-link");
    if(Errors::occurred()) {
        cout << "Unable to create sess at capchain-root server. Error= " << Errors::last <<
            " (" << Errors::to_string(Errors::last) << ").\n";
        return 1;
    }
    CAPLOGV("Created session successfully");

    Server<CapHandler> *srv;
    int numcaps = IStringStream::read_from<int>(argv[1]);
    CAPLOG("capchain-root [numcaps=" << numcaps << "]");
    CapHandler hdl(true, numcaps, sess);
    srv = new Server<CapHandler>("capchain-root", &hdl, nextlog2<4096>::val, nextlog2<128>::val);
    if(Errors::occurred())
        CAPLOG("Error creating capchain-root server: " << Errors::to_string(Errors::last));

    // tell link server that we are ready to start cap exchange
    sess->signal();

    env()->workloop()->run();
    delete srv;
    CAPLOG("Capchain Root Server shut down . Return code: " << Errors::to_string(Errors::last));
    return 0;
}
