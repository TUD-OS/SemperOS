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

#pragma once

#include <base/Types.h>

namespace kernel {

class RKVPE {
public:
    RKVPE(size_t id, size_t krnlID) : _id(id), _krnlID(krnlID) {
    }
    RKVPE(const RKVPE &) = delete;
    RKVPE &operator=(const RKVPE &) = delete;
    
    size_t id() const {
        return _id;
    }
    size_t krnlID() const {
        return _krnlID;
    }
    void set_id(size_t id) {
        _id = id;
    }
    void set_krnlid(size_t krnlID) {
        _krnlID = krnlID;
    }
    
private:
    size_t _id;
    size_t _krnlID;
    // TODO capabilities
};

}
