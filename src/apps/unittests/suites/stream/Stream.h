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

#include <base/Heap.h>

#include <test/TestSuite.h>

class StreamTestSuite : public test::TestSuite {
private:
    class IStreamTestCase : public test::TestCase {
    public:
        explicit IStreamTestCase() : test::TestCase("IStream") {
        }
        virtual void run() override;
    };
    class OStreamTestCase : public test::TestCase {
    public:
        explicit OStreamTestCase() : test::TestCase("OStream") {
        }
        virtual void run() override;
    };
    class FStreamTestCase : public test::TestCase {
    public:
        explicit FStreamTestCase() : test::TestCase("FStream") {
        }
        virtual void run() override;
    };

public:
    explicit StreamTestSuite()
        : TestSuite("Stream") {
        add(new IStreamTestCase());
        add(new OStreamTestCase());
        add(new FStreamTestCase());
    }
};
