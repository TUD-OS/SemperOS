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

#pragma once

#if defined KTEST_reserve || defined KTEST_hash

#include "KTestSuite.h"
#include "KTestCase.h"

namespace kernel {
    class DDLTestSuite : public kernel::KTestSuite {
    private:
        #ifdef KTEST_reserve
        class DDLReserveTestCase : public kernel::KTestCase {
        public:
            explicit DDLReserveTestCase() : kernel::KTestCase("DDL Reserve") { }
            ~DDLReserveTestCase() { }
            virtual void run() override;
        };
        #endif
        #ifdef KTEST_hash
        class HashUtilTestCase : public kernel::KTestCase {
        public:
            explicit HashUtilTestCase() : kernel::KTestCase("DDL HashUtil") { }
            ~HashUtilTestCase() { }
            virtual void run() override;
        };
        #endif
    public:
        explicit DDLTestSuite() : KTestSuite("DDL") {
            #ifdef KTEST_reserve
            add(new DDLReserveTestCase());
            #endif
            #ifdef KTEST_hash
            add(new HashUtilTestCase());
            #endif
        }
    };
}

#endif
