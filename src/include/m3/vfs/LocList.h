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

#include <base/stream/OStream.h>

#include <m3/com/GateStream.h>

namespace m3 {

template<size_t N>
class LocList {
public:
    explicit LocList() : _count(), _lengths() {
    }

    void append(size_t length) {
        assert(_count < N);
        _lengths[_count++] = length;
    }
    void clear() {
        _count = 0;
        memset(_lengths, 0, sizeof(_lengths));
    }

    size_t count() const {
        return _count;
    }
    size_t get(size_t i) const {
        return _lengths[i];
    }

    friend OStream &operator <<(OStream &os, const LocList &l) {
        os << "LocList[";
        for(size_t i = 0; i < l.count(); ++i) {
            os << l.get(i);
            if(i != l.count() - 1)
                os << ", ";
        }
        os << "]";
        return os;
    }

private:
    size_t _count;
    size_t _lengths[N];
};

}
