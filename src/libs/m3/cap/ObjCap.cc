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

#include <m3/ObjCap.h>
#include <m3/Syscalls.h>
#include <m3/VPE.h>

namespace m3 {

void ObjCap::release() {
    if(_sel != INVALID) {
        if(~_flags & KEEP_SEL)
            VPE::self().free_cap(sel());
        if(~_flags & KEEP_CAP)
            Syscalls::get().revoke(CapRngDesc(CapRngDesc::OBJ, sel()));
    }
}

}
