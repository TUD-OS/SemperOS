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

#include <base/Common.h>
#include <base/util/Math.h>
#include <base/DTU.h>

#include <algorithm>

class VarRingBuf {
public:
    explicit VarRingBuf(size_t size) : _size(size), _rdpos(), _wrpos(), _last() {
        assert((size & DTU_PKG_SIZE) == 0);
    }

    bool empty() const {
        return _rdpos == _wrpos;
    }

    ssize_t get_write_pos(size_t size) {
        if(_wrpos % DTU_PKG_SIZE) {
            if(!empty())
                return -1;
            _wrpos = m3::Math::round_up(_wrpos, DTU_PKG_SIZE);
            _rdpos = _wrpos;
        }

        if(_wrpos >= _rdpos) {
            if(_size - _wrpos >= size)
                return _wrpos;
            else if(_rdpos > size)
                return 0;
        }
        else if(_rdpos - _wrpos > size)
            return _wrpos;
        return -1;
    }

    ssize_t get_read_pos(size_t *size) {
        if(_wrpos == _rdpos)
            return -1;

        size_t rpos = _rdpos;
        if(rpos == _last)
            rpos = 0;
        if(_wrpos > rpos)
            *size = std::min(_wrpos - rpos,*size);
        else
            *size = std::min(_size - rpos,*size);
        return rpos;
    }

    void push(size_t size) {
        assert((_wrpos % DTU_PKG_SIZE) == 0);

        if(_wrpos >= _rdpos) {
            if(_size - _wrpos >= size)
                _wrpos += size;
            else if(_rdpos > size) {
                _last = _wrpos;
                _wrpos = size;
            }
        }
        else if(_rdpos - _wrpos > size)
            _wrpos += size;
    }

    void pull(size_t size) {
        assert(!empty());
        if(_rdpos == _last) {
            _rdpos = 0;
            _last = 0;
        }
        _rdpos += size;
    }

private:
    size_t _size;
    size_t _rdpos;
    size_t _wrpos;
    size_t _last;
};
