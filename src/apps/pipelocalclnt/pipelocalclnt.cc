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

#include <base/Common.h>
#include <base/util/String.h>
#include <base/stream/IStringStream.h>

#include <m3/session/Session.h>
#include <m3/stream/Standard.h>
#include <m3/pipe/LocalDirectPipe.h>

#define ITERATIONS      5

using namespace m3;

int main(int argc, char **argv) {
    String srv;
    if(argc < 2) {
        cout << "Missing argmuents\nUsage " << argv[0] << " <servername>\n";
        return 1;
    }
    else {
        srv = IStringStream::read_from<m3::String>(argv[1]);
    }

    LocalDirectPipe pipe(srv);
    if(Errors::occurred()) {
        cout << "Unable to create sess at " << srv << "\n";
        return 1;
    }

    cout << "Created pipe successfully\n";

    for(int i = 0; i < ITERATIONS; i++) {
        // send a message
        int input = 18;
        pipe.writer()->send((void*)&input, sizeof(int));

        // recv a message, blocking on it
        GateIStream is = pipe.wait();
        cout << "client: gatestream length: " << is.length() << "\n";
        is >> input;
        cout << "client: received message: " << input << "\n";
    }

    cout << "client: start polling\n";
    for(int i = 0; i < ITERATIONS; i++) {
        // send a message
        int input = 18;
        pipe.writer()->send((void*)&input, sizeof(int));

        // recv a message, polling
        DTU::Message *msg = nullptr;
        while(!msg)
            msg = pipe.fetch_msg();

        GateIStream is(*pipe.rgate(), msg);
        cout << "client: gatestream length: " << is.length() << "\n";
        is >> input;
        cout << "client: received message: " << input << "\n";
    }

    cout << "client: finished " << 2*ITERATIONS << " iterations\n";
    return 0;
}