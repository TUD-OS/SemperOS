/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of M3 (Microkernel for Minimalist Manycores).
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

#include <base/util/Sync.h>
#include <base/Config.h>

#include "DTU.h"
#include "pes/KPE.h"
#include "pes/VPE.h"

namespace kernel {

void KPE::start(int argc, char** argv, size_t pe_count, m3::PEDesc PEs[]) {
#if defined(__gem5__)
    init_memory(argc, argv, pe_count, PEs);
#endif

    DTU::get().wakeup(VPEDesc(core(), VPE::INVALID_ID));

    KLOG(KPES, "Started KPE '" << _name << "' [id=" << id() << "]");
}

}
