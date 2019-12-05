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

#include <m3/com/MemGate.h>

#include <test/TestSuite.h>

#include "BaseTestCase.h"

class MemoryTestSuite : public test::TestSuite {
private:
    class SyncTestCase : public BaseTestCase {
    public:
        explicit SyncTestCase(m3::MemGate & mem) : BaseTestCase("Synchronous"), _mem(mem) {
        }
        virtual void run() override;
    private:
        m3::MemGate &_mem;
    };

    class DeriveTestCase : public BaseTestCase {
    public:
        explicit DeriveTestCase(m3::MemGate & mem) : BaseTestCase("Derive memory"), _mem(mem) {
        }
        virtual void run() override;
    private:
        m3::MemGate &_mem;
    };

public:
    explicit MemoryTestSuite()
        : TestSuite("Memory"), _mem(m3::MemGate::create_global(0x4000, m3::MemGate::RWX)) {
        add(new SyncTestCase(_mem));
        add(new DeriveTestCase(_mem));
    }

private:
    m3::MemGate _mem;
};
