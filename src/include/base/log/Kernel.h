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

#include <base/log/Log.h>
#include <thread/ThreadManager.h>
#define KLOG(lvl, msg)  LOG(KernelLog, lvl, m3::ThreadManager::get().current()->id() << ": " << msg)

namespace m3 {

class KernelLog {
    KernelLog() = delete;

public:
    enum Level {
        INFO        = 1 << 0,
        KENV        = 1 << 1,
        ERR         = 1 << 2,
        MEM         = 1 << 3,
        SYSC        = 1 << 4,
        PTES        = 1 << 5,
        VPES        = 1 << 6,
        EPS         = 1 << 7,
        SERV        = 1 << 8,
        SLAB        = 1 << 9,
        KRNLC       = 1 << 10,
        KPES        = 1 << 11,
        MHT         = 1 << 12,
    };

    static const int level = INFO | ERR;
};

}
