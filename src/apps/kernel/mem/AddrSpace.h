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

#include <base/Common.h>

#include "mem/MainMemory.h"

namespace kernel {

class AddrSpace {
public:
    explicit AddrSpace(int ep, capsel_t gate);
    ~AddrSpace();

    int ep() const {
        return _ep;
    }
    capsel_t gate() const {
        return _gate;
    }
    uint64_t root_pt() const {
        return m3::DTU::build_noc_addr(_rootpt.pe(), _rootpt.addr);
    }

    int _ep;
    capsel_t _gate;
    MainMemory::Allocation _rootpt;
};

}
