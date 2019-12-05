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
#include <base/stream/OStringStream.h>
#include <base/util/Profile.h>
#include <base/benchmark/capbench.h>

#include <m3/server/Server.h>
#include <m3/server/RequestHandler.h>
#include <m3/stream/Standard.h>

#include "../common/operations.h"
#include "../common/caphandler.h"

using namespace m3;

int main(int argc, char *argv[]) {
    if(argc < 3) {
        cout << "Usage: " << argv[0] << " <numCaps> <numClients>\nNote: the numCaps should "
            "give the total number of capabilities.";
        return 1;
    }

    int numcaps = IStringStream::read_from<int>(argv[1]);
    int numclients = IStringStream::read_from<int>(argv[2]);
    CAPLOG("capforest-root [numcaps=" << numcaps << ", numClients=" << numclients << "]");

    CapServerSession **sessions = static_cast<CapServerSession**>(
        m3::Heap::alloc(sizeof(CapServerSession*) * numclients));
    // create sessions
    for(uint i = 0; i < static_cast<uint>(numclients); i++) {
        OStringStream linkName;
        linkName << "capforest-link-" << i;
        sessions[i] = new CapServerSession(linkName.str());
        if(Errors::occurred()) {
            cout << "Unable to create sess at capforest-root server. Error= " << Errors::last <<
                " (" << Errors::to_string(Errors::last) << ").\n";
            return 1;
        }
        CAPLOGV("Created session with " << linkName.str() << " successfully");
    }

    Server<CapHandler> *srv;

    CapHandler hdl(true, numcaps, sessions, numclients);
    srv = new Server<CapHandler>("capforest-root", &hdl, nextlog2<4096>::val, nextlog2<128>::val);
    if(Errors::occurred())
        CAPLOG("Error creating capforest-root server: " << Errors::to_string(Errors::last));

    // tell link servers that we are ready to start cap exchange
    for(uint i = 0; i < static_cast<uint>(numclients); i++)
        sessions[i]->signal();

    env()->workloop()->run();
    delete srv;
    CAPLOG("Capforest Root Server shut down . Return code: " << Errors::to_string(Errors::last));
    return 0;
}
