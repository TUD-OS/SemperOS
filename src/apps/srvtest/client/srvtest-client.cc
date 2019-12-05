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

#include <m3/session/Session.h>
#include <m3/stream/Standard.h>
#include <m3/VPE.h>

using namespace m3;

int main() {
    int failed = 0;
    for(int j = 0; j < 10; ++j) {
        Session sess("srvtest-server");
        if(Errors::occurred()) {
            cout << "Unable to create sess at srvtest-server. Retrying.\n";
            if(++failed == 20)
                break;
            --j;
            continue;
        }

        failed = 0;
        for(int i = 0; i < 10; ++i) {
            capsel_t sel = sess.obtain(1).start();
            VPE::self().free_cap(sel);

            if(Errors::last == Errors::INV_ARGS)
                break;
            if(Errors::last != Errors::NOT_SUP)
                cout << "Expected NOT_SUP, got " << Errors::to_string(Errors::last) << "\n";
        }
    }
    return 0;
}
