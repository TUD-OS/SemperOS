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

namespace m3 {

/**
 * The different types of PEs
 */
enum class PEType {
    // Compute PE with internal memory
    COMP_IMEM   = 0,
    // Compute PE with external memory, i.e., with cache
    COMP_EMEM   = 1,
    // memory PE
    MEM         = 2,
};

/**
 * Describes a PE
 */
struct PEDesc {
    typedef uint64_t value_t;

    /**
     * Default constructor
     */
    explicit PEDesc() : _value() {
    }
    /**
     * Creates a PE description from the given descriptor word
     */
    explicit PEDesc(value_t value) : _value(value) {
    }
    /**
     * Creates a PE description of given type and with given memory size
     */
    explicit PEDesc(PEType type, size_t memsize = 0)
        : _value(static_cast<value_t>(type) | memsize) {
    }

    /**
     * @return the raw descriptor word
     */
    value_t value() const {
        return _value;
    }

    /**
     * @return the type of PE
     */
    PEType type() const {
        return static_cast<PEType>(_value & 0x0000000000000007);
    }
    /**
     * @return the memory size (for type() == COMP_IMEM | MEM)
     */
    size_t mem_size() const {
        return _value & ~0xFFC0000000000007;
    }
    /**
     * @return true if the PE has internal memory
     */
    bool has_memory() const {
        return type() != PEType::COMP_EMEM;
    }
    /**
     * @return true if the PE has a cache, i.e., external memory
     */
    bool has_cache() const {
        return type() == PEType::COMP_EMEM;
    }
    /**
     * @return true if the PE has virtual memory support
     */
    bool has_virtmem() const {
        return type() == PEType::COMP_EMEM;
    }
    size_t core_id() const {
        return (_value & 0xFFC0000000000000) >> 54;
    }

private:
    value_t _value;
} PACKED;

}
