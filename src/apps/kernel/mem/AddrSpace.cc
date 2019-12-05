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
#include <base/Config.h>
#include <base/DTU.h>

#include "mem/AddrSpace.h"
#include "mem/MainMemory.h"

namespace kernel {

AddrSpace::AddrSpace(int ep, capsel_t gate)
    : _ep(ep),
      _gate(gate),
      _rootpt(MainMemory::get().allocate(PAGE_SIZE)) {
}

AddrSpace::~AddrSpace() {
    MainMemory::get().free(_rootpt);
}

}
