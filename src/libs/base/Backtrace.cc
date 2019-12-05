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

#include <base/stream/OStream.h>
#include <base/Backtrace.h>

namespace m3 {

void Backtrace::print(OStream &os) {
    uintptr_t addr[MAX_DEPTH];
    size_t cnt = collect(addr, MAX_DEPTH);

    os << "Backtrace:\n";
    for(size_t i = 0; i < cnt; ++i)
        os << "  " << fmt(addr[i], "p") << "\n";
}

}
