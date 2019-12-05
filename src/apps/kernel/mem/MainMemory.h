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
#include <base/util/Math.h>
#include <base/Config.h>
#include <base/Panic.h>

#include "mem/MemoryModule.h"

namespace m3 {
    class OStream;
}

namespace kernel {

class MainMemory {
    explicit MainMemory();

    static const size_t MAX_MODS    = 4;

public:
    struct Allocation {
        explicit Allocation() : mod(), addr(), size() {
        }
        explicit Allocation(size_t _mod, uintptr_t _addr, size_t _size)
            : mod(_mod), addr(_addr), size(_size) {
        }

        operator bool() const {
            return size > 0;
        }
        size_t pe() const {
            return MainMemory::get().module(mod).pe();
        }

        size_t mod;
        uintptr_t addr;
        size_t size;
    };

    static MainMemory &get() {
        return _inst;
    }

    void add(MemoryModule *mod);

    const MemoryModule &module(size_t id) const;

    Allocation allocate(size_t size);
    Allocation allocate_at(uintptr_t offset, size_t size);

    void free(size_t pe, uintptr_t addr, size_t size);
    void free(const Allocation &alloc);

    /**
     * Detaches a chunk of <size> bytes from the end of the rearmost possible memory module.
     *
     * @param size  Size of the chunk to detach
     * @return The detached memory module, or an empty and unavaibable memory module in case of failure
     */
    MemoryModule detach(size_t size);

    size_t size() const;
    size_t available() const;

    friend m3::OStream &operator<<(m3::OStream &os, const MainMemory &mem);

private:
    size_t _count;
    MemoryModule *_mods[MAX_MODS];
    static MainMemory _inst;
};

}
