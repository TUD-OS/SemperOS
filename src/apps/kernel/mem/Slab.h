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
#include <base/col/SList.h>
#include <base/col/DList.h>
#include <base/util/Util.h>

namespace kernel {

class Slab : public m3::SListItem {
    struct Pool : public m3::DListItem {
        explicit Pool(size_t objsize, size_t count);
        ~Pool();

        size_t total;
        size_t free;
        void *mem;
    };

public:
    static const size_t STEP_SIZE   = 64;

    static Slab *get(size_t objsize);

    explicit Slab(size_t objsize) : _freelist(), _objsize(objsize) {
    }

    void *alloc();
    void free(void *ptr);

private:
    void **_freelist;
    size_t _objsize;
    m3::DList<Pool> _pools;
    static m3::SList<Slab> _slabs;
};

}
