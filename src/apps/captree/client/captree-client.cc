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

#include <m3/session/Session.h>
#include <m3/stream/Standard.h>
#include <m3/VPE.h>

#include "../operations.h"

// verbosity
#define CAPLOGV(msg)
//#define CAPLOGV(msg) m3::cout << msg << "\n";
#define CAPLOG(msg) m3::cout << msg << "\n";

using namespace m3;

int main(int argc, char *argv[]) {
    if(argc < 2) {
        cout << "Usage: " << argv[0] << " <numCaps>\n";
        return 1;
    }
    int numcaps = IStringStream::read_from<int>(argv[1]);
    uint rounds = 11;
    CAPLOG("capbench-client [numcaps=" << numcaps << ", rounds=" << rounds << "]");

    // create session
    CapServerSession *sess = new CapServerSession("captree-server");
    if(Errors::occurred()) {
        cout << "Unable to create sess at captree-server. Error= " << Errors::last <<
            " (" << Errors::to_string(Errors::last) << ").\n";
        return 1;
    }
    CAPLOGV("Created session successfully");

    // rounds
    for(uint k = 0; k < rounds; k++) {
        CAPLOGV("Round " << k);
        // obtain caps
        for(int i = 0; i < numcaps; ++i) {
            sess->obtain(1);
            if(Errors::last == Errors::NO_ERROR) {
                CAPLOGV("successfully obtained");
                sess->signal();
            }
            else {
                CAPLOG("Obtain failed: " << Errors::to_string(Errors::last) << "(" << Errors::last << ")");
            }

            if(Errors::last != Errors::NO_ERROR) {
                CAPLOG("Error, got " << Errors::to_string(Errors::last));
            }
        }
    }

    CAPLOGV("Shutting client down");
    return 0;
}
