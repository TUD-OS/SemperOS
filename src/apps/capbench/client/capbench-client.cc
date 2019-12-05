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
    CAPLOG("capbench-client [numcaps=" << numcaps << "]");

    cycles_t *obtainTime = new cycles_t[numcaps];
    cycles_t *revokeTime = new cycles_t[numcaps];
    cycles_t start, time;
    CapRngDesc *caps = new CapRngDesc[numcaps];

    // create session
    start = m3::Profile::start(1);
    CapServerSession *sess = new CapServerSession("capbench-server");
    if(Errors::occurred()) {
        cout << "Unable to create sess at capbench-server. Error= " << Errors::last <<
            " (" << Errors::to_string(Errors::last) << ").\n";
        return 1;
    }
    time = CAP_BENCH_TRACE_F(TRACE_START_ANALYSIS) - start;
    CAPLOGV("Created session successfully in "<< time << " cycles");

    // obtain caps
    for(int i = 0; i < numcaps; ++i) {
        start = CAP_BENCH_TRACE_S(APP_OBT_START);
        caps[i] = sess->obtain(1);
        time = CAP_BENCH_TRACE_F(APP_OBT_FINISH) - start;
        obtainTime[i] = time;
        if(Errors::last == Errors::NO_ERROR) {
            CAPLOGV("successfully obtained in " << time << " cycles");
        }
        else {
            CAPLOG("Obtain failed: " << Errors::to_string(Errors::last) << "(" << Errors::last << ")");
        }

        if(Errors::last != Errors::NO_ERROR) {
            CAPLOG("Error, got " << Errors::to_string(Errors::last));
        }
    }

    // revoke caps
    for(int i = 0; i < numcaps; ++i) {
        start = m3::Profile::start(5);
        sess->revcap(caps[i]);
        time = m3::Profile::stop(6) - start;
        CAPLOGV(" successfully revoked in " << time << " cycles");
        revokeTime[i] = time;
        if(Errors::last != Errors::NO_ERROR) {
            CAPLOG("Error, got " << Errors::to_string(Errors::last));
        }

        VPE::self().free_caps(caps[i].start(), caps[i].count());
    }
    CAP_BENCH_TRACE_F(TRACE_FINISH_ANALYSIS);

    // calculate statistics
    cycles_t avgObtainTime = 0;
    cycles_t avgRevokeTime = 0;
    cycles_t varianceObtain = 0;
    cycles_t varianceRevoke = 0;
    CAPLOG("Warming up obtain: " << obtainTime[0]);
    for(int i = 1; i < numcaps; ++i) {
        avgObtainTime += obtainTime[i];
        avgRevokeTime += revokeTime[i];
        CAPLOG("Round " << i << " obtain: " << obtainTime[i]);
    }

    CAPLOG("Warming up revoke: " << revokeTime[0]);
    for(int i = 1; i < numcaps; ++i)
        CAPLOG("Round " << i << " revoke: " << revokeTime[i]);

    avgObtainTime = avgObtainTime / (numcaps - 1);
    avgRevokeTime = avgRevokeTime / (numcaps - 1);
    for(int i = 1; i < numcaps; ++i) {
        varianceObtain += static_cast<uint>( static_cast<int>(static_cast<int>(avgObtainTime) - obtainTime[i]) *
                static_cast<int>(static_cast<int>(avgObtainTime) - obtainTime[i]) );
        varianceRevoke += static_cast<uint>( static_cast<int>(static_cast<int>(avgRevokeTime) - revokeTime[i]) *
                static_cast<int>(static_cast<int>(avgRevokeTime) - revokeTime[i]) );
    }
    varianceObtain = varianceObtain / (numcaps - 1);
    varianceRevoke = varianceRevoke / (numcaps -1);
    CAPLOG("Avg. obtain time: " << avgObtainTime);
    CAPLOG("Variance obtain: " << varianceObtain);
    CAPLOG("Avg. revoke time: " << avgRevokeTime);
    CAPLOG("Variance revoke: " << varianceRevoke);
    CAPLOGV("Shutting client down");
    return 0;
}
