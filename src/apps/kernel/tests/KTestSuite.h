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

#ifdef KERNEL_TESTS

#include <base/col/SList.h>
#include <test/Testable.h>

#include "KTestCase.h"

namespace kernel {

class KTestSuite : public m3::SListItem, public test::Testable {
public:
    explicit KTestSuite(const m3::String& name)
        : m3::SListItem(), test::Testable(name), _cases() {
    }
    ~KTestSuite() {
        for(auto it = _cases.begin(); it != _cases.end(); ) {
            auto old = it++;
            delete &*old;
        }
    }

    void add(KTestCase* tc) {
        _cases.append(tc);
    }

private:
    m3::SList<KTestCase> _cases;
};

}

#endif
