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
#include <base/util/Profile.h>

#include <m3/com/MemGate.h>
#include <m3/stream/Standard.h>
#include <m3/Syscalls.h>

using namespace m3;

#define COUNT   100

static word_t buffer[4];

int main() {
    MemGate mem = MemGate::create_global(0x1000, MemGate::RW);
    mem.read_sync(buffer, sizeof(buffer), 0);
    cycles_t total = 0;
    for(int i = 0; i < COUNT; ++i) {
        cycles_t start = Profile::start(0);
        Syscalls::get().activate(mem.epid(), mem.sel(), mem.sel());
        cycles_t end = Profile::stop(0);
        total += end - start;
    }
    cout << "Per activate: " << (total / COUNT) << "\n";
    return 0;
}
