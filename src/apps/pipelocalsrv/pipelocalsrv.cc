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

#include <base/stream/IStringStream.h>
#include <base/DTU.h>

#include <m3/stream/Standard.h>
#include <m3/pipe/PipeHandler.h>

#define ITERATIONS      5
using namespace m3;

static LocalPipeServer<PipeHandler> *pipesrv;

int main(int argc, char **argv) {
    String srv;
    if(argc < 2) {
        cout << "Missing argmuents\nUsage " << argv[0] << " <servername>\n";
        return 1;
    }
    else {
        srv = IStringStream::read_from<m3::String>(argv[1]);
    }
    PipeHandler pipeHndlr;
    cout << "starting service\n";
    pipesrv = new LocalPipeServer<PipeHandler>(srv, &pipeHndlr);

    // set up pipe
    env()->workloop()->run();
    cout << "server: exited workloop\n";
    // after set up delete the server to free the EPs
    LocalDirectPipe *pipe = pipeHndlr.pipe();
    delete pipesrv;
    if(pipe == nullptr) {
        cout << "BROKEN PIPE\n";
        return 1;
    }
    cout << "done with setup, ready to use pipe\n";

    for(int i = 0; i < ITERATIONS; i++) {
        // wait for a message
        GateIStream is = pipe->wait();
        cout << "server: gatestream length: " << is.length() << "\n";
        int output;
        is >> output;
        cout << "server: received message: " << output << "\n";

        output = 20;
        pipe->writer()->send((void*)&output, sizeof(int));
    }

    cout << "server: start polling\n";
    for(int i = 0; i < ITERATIONS; i++) {
        // poll for a message
        DTU::Message *msg = nullptr;
        while(!msg)
            msg = pipe->fetch_msg();

        GateIStream is(*pipe->rgate(), msg);
        cout << "server: gatestream length: " << is.length() << "\n";
        int output;
        is >> output;
        cout << "server: received message: " << output << "\n";

        output = 20;
        pipe->writer()->send((void*)&output, sizeof(int));
    }



    cout << "server: finished " << 2*ITERATIONS << " iterations\n";


    return 0;
}
