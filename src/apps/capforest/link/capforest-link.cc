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

#include <m3/session/Session.h>
#include <m3/stream/Standard.h>
#include <m3/VPE.h>

#include "../common/operations.h"
#include "../common/caphandler.h"

using namespace m3;

int main(int argc, char *argv[]) {
    if(argc < 3) {
        cout << "Usage: " << argv[0] << " <numCaps> <clientID>\n";
        return 1;
    }
    int clientID = IStringStream::read_from<int>(argv[2]);
    int numcaps = IStringStream::read_from<int>(argv[1]);
    CAPLOG("capforest-link [numcaps=" << numcaps << ", clientID=" << clientID << "]");

    Server<CapHandler> *srv;
    CapHandler hdl(false, numcaps);
    OStringStream srvName;
    srvName << "capforest-link-" << clientID;
    srv = new Server<CapHandler>(srvName.str(), &hdl, nextlog2<4096>::val, nextlog2<128>::val);
    if(Errors::occurred())
        CAPLOG("Error creating " << srvName.str() << " server: " << Errors::to_string(Errors::last));

    env()->workloop()->run();

    delete srv;
    CAPLOG("Capforest Server " << srvName.str() << "shut down . Return code: " << Errors::to_string(Errors::last));
    return 0;
}
