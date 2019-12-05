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

#include <test/TestSuite.h>

#include "BaseTestCase.h"

class CommandsTestSuite : public test::TestSuite {
private:
    class ReadCmdTestCase : public BaseTestCase {
    public:
        explicit ReadCmdTestCase() : BaseTestCase("Read command") {
        }
        virtual void run() override;
    };

    class WriteCmdTestCase : public BaseTestCase {
    public:
        explicit WriteCmdTestCase() : BaseTestCase("Write command") {
        }
        virtual void run() override;
    };

    class CmpxchgCmdTestCase : public BaseTestCase {
    public:
        explicit CmpxchgCmdTestCase() : BaseTestCase("Cmpxchg command") {
        }
        virtual void run() override;
    };

public:
    explicit CommandsTestSuite()
        : TestSuite("Commands") {
        add(new ReadCmdTestCase());
        add(new WriteCmdTestCase());
        add(new CmpxchgCmdTestCase());
    }
};
