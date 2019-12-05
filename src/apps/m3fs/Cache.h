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

#include <m3/com/MemGate.h>

#include <fs/internal.h>

class Cache {
    struct BlockInfo {
        bool dirty;
        m3::blockno_t bno;
        unsigned timestamp;
    };

public:
    static const size_t BLOCK_COUNT     = 1024;

    explicit Cache(m3::MemGate &mem, size_t blocksize);
    void *get_block(m3::blockno_t bno, bool write);
    void mark_dirty(m3::blockno_t bno);
    void write_back(m3::inodeno_t ino);
    void flush();

private:
    void flush_block(size_t i);

    m3::MemGate &_mem;
    size_t _blocksize;
    char *_data;
    unsigned _timestamp;
    BlockInfo _blocks[BLOCK_COUNT];
};
