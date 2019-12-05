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

#include "pes/VPE.h"
#include "Gate.h"

namespace kernel {

m3::Errors::Code SendGate::send(const void *data, size_t len, RecvGate *rgate) {
    DTU::get().send_to(_vpe.desc(), _ep, _label, data, len,
        reinterpret_cast<uintptr_t>(rgate), rgate->epid());
    return m3::Errors::NO_ERROR;
}

}
