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
#include <base/col/SList.h>
#include <base/util/Profile.h>

#include <m3/session/Session.h>
#include <m3/stream/Standard.h>
#include <m3/pipe/AggregateDirectPipe.h>

#include "LoadGenWorkLoop.h"

using namespace m3;

int main(int argc, char **argv) {
    if(argc < 3) {
        cout << "Missing argmuents\nUsage " << argv[0] << " <numRequestsPerServer> <servername>\n";
        return 1;
    }

    int requestsPerServer = IStringStream::read_from<int>(argv[1]);
    cout << "Starting load generator #srv=" << (argc - 2) << ", #requ/srv=" <<
            requestsPerServer << "\n";

    LoadGenWorkLoop wl;

    LoadGenWorkItem **workitems = new LoadGenWorkItem*[argc - 2];

    // Hack
    // we use one receive gate for 3 servers
    RecvBuf *rbuf = nullptr;
    RecvGate *rgate = nullptr;
    for(int i = 0; i < argc - 2; i++) {
        String server = String(argv[i + 2]);
        AggregateDirectPipe * pipe = new AggregateDirectPipe(server, rbuf, rgate);
        if(Errors::occurred()) {
            cout << "Unable to create sess at " << server << "\n";
            return 1;
        }
//        cout << "created server " << server << " rbuf@ " << rbuf << "\n";
        // we use 3 clients per receive gate
        if(i % 3 == 2) {
            rbuf = nullptr;
            rgate = nullptr;
        }
        else {
            rbuf = pipe->rbuf();
            rgate = pipe->rgate();
        }

        // Create work item for the server
        workitems[i] = new LoadGenWorkItem(pipe, server, requestsPerServer, 236, 18201, &wl);
        wl.add(workitems[i], false);
        if(i % 3 == 0)
            wl.add_rgate(pipe->rgate());
    }


    cycles_t start = Profile::start(0);
    wl.run();
    cycles_t end = Profile::stop(0);

    //debug
    float reqpersec = (static_cast<float>(2000000000) / static_cast<float>(end-start)) * static_cast<float>(requestsPerServer*(argc-2));

    cout << "loadgen: runtime: " << (end - start) << " cycles, requests: "
        << (requestsPerServer*(argc-2)) //<< "\n";
            //debug
            << " , requests per second: " << reqpersec << "\n";

    for(int i = 0; i < argc - 2; i++) {
        workitems[i]->shutdown();
        delete workitems[i];
    }

    return 0;
}