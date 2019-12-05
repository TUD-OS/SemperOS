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

#define SLOG(lvl, msg)  LOG(ServiceLog, lvl, msg)

namespace m3 {

class ServiceLog {
    ServiceLog() = delete;

public:
    enum Level {
        KEYB        = 1 << 0,
        FS          = 1 << 1,
        FS_DBG      = 1 << 2,
        PAGER       = 1 << 3,
        PIPE        = 1 << 4,
    };

    static const int level = 0;
};

}
