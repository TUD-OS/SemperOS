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

#include <test/TestSuiteContainer.h>

#include "suites/dtu/Ringbuffer.h"
#include "suites/dtu/Commands.h"
#include "suites/dtu/Memory.h"

int main() {
    test::TestSuiteContainer con;
    con.add(new RingbufferTestSuite());
    con.add(new CommandsTestSuite());
    con.add(new MemoryTestSuite());
    return con.run();
}
