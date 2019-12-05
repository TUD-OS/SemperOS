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
#include <base/util/Profile.h>

#include <m3/com/MemGate.h>
#include <m3/stream/Standard.h>

using namespace m3;

#define SIZE        (64 * 1024)

int main(int argc, char *argv[]) {
    size_t size = 1024;
    if(argc > 1)
        size = IStringStream::read_from<size_t>(argv[1]);
    char *buffer = new char[size];

    cycles_t start1 = Profile::start(0);
    MemGate mem = MemGate::create_global(SIZE, MemGate::R);
    cycles_t end1 = Profile::stop(0);

    cycles_t start2 = Profile::start(1);
    for(size_t i = 0; i < SIZE / size; ++i)
        mem.read_sync(buffer, size, 0x0);
    cycles_t end2 = Profile::stop(1);

    cout << "Setup time: " << (end1 - start1) << "\n";
    cout << "Read time: " << (end2 - start2) << "\n";
    return 0;
}
