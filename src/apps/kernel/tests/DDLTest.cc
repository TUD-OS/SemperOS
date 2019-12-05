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

#if defined KTEST_reserve || defined KTEST_hash

#include "DDLTest.h"
#include "ddl/MHTTypes.h"

namespace kernel {

#ifdef KTEST_reserve
void DDLTestSuite::DDLReserveTestCase::run() {
    // TODO test
    KLOG(MHT, "test running");
}
#endif

#ifdef KTEST_hash
void DDLTestSuite::HashUtilTestCase::run() {
    // TODO
    // what's the actual meaning of this function?
    mht_key_t key = HashUtil::structured_hash(2, VPECAP, 3);
    KLOG(MHT, "test running");
//    assert_uint(key = 0x0009U); // TODO
}
#endif

}

#endif