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
#include <base/log/Kernel.h>

#include <test/Testable.h>
#include <test/Assert.h>

namespace kernel {
class KTestCase : public m3::SListItem, public test::Testable {
public:
    explicit KTestCase(const m3::String& name)
        : m3::SListItem(), Testable(name) {
    }

protected:
    void do_assert(const test::Assert& a) {
        if(!a) {
            KLOG(ERR, "  \033[0;31mAssert failed\033[0m in " << a.get_file()
                     << ", line " << a.get_line() << ": " << a.get_desc());
            failed();
        }
        else
            success();
    }
};

}

#endif
