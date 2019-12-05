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

#include "Allocator.h"
#include "FSHandle.h"

using namespace m3;

Allocator::Allocator(uint32_t first, uint32_t *first_free, uint32_t *free, uint32_t total, uint32_t blocks)
    : _first(first), _first_free(first_free), _free(free), _total(total), _blocks(blocks) {
    static_assert(sizeof(blockno_t) == sizeof(uint32_t), "Wrong type");
    static_assert(sizeof(inodeno_t) == sizeof(uint32_t), "Wrong type");
}

uint32_t Allocator::alloc(FSHandle &h, size_t *count) {
    const size_t perblock = h.sb().blocksize * 8;
    const uint32_t lastno = _first + _blocks - 1;
    const size_t icount = *count;
    uint32_t no = _first + *_first_free / perblock;
    size_t total = 0;
    uint32_t i = *_first_free % perblock;

    while(total == 0 && no <= lastno) {
        Bitmap::word_t *bytes = reinterpret_cast<Bitmap::word_t*>(h.cache().get_block(no, true));
        // take care that total_blocks might not be a multiple of perblock
        size_t max = perblock;
        if(no == lastno) {
            max = _total % perblock;
            max = max == 0 ? perblock : max;
        }
        Bitmap bm(bytes);

        // first, search quickly until we've found a word that has free bits
        if(i < max && bm.is_word_set(i)) {
            // within the first word with free bits, the first free bit is not necessarily
            // i % Bitmap::WORD_BITS. thus, start from 0 within each word
            i = (i + Bitmap::WORD_BITS) & ~(Bitmap::WORD_BITS - 1);
            for(; i < max && bm.is_word_set(i); i += Bitmap::WORD_BITS)
                ;
            if(i < max) {
                // now walk to the actual first free bit
                for(; bm.is_set(i); ++i)
                    ;
            }
        }

        // now walk until its aligned (i < max is not required here since a block is always a multiple
        // of Bitmap::WORD_BITS and we run only until i % Bitmap::WORD_BITS == 0)
        for(; (i % Bitmap::WORD_BITS) != 0 && total < icount; ++i) {
            if(!bm.is_set(i)) {
                bm.set(i);
                total++;
            }
            else if(total > 0)
                break;
        }

        // now allocate in words
        for(; (icount - total) >= Bitmap::WORD_BITS && (max - i) >= Bitmap::WORD_BITS; i += Bitmap::WORD_BITS) {
            if(bm.is_word_free(i)) {
                bm.set_word(i);
                total += Bitmap::WORD_BITS;
            }
            else if(total > 0)
                break;
        }

        if(total == 0) {
            for(; i < max && total < icount; ++i) {
                if(!bm.is_set(i)) {
                    bm.set(i);
                    total++;
                }
            }
        }
        else {
            // there might be something left
            for(; i < max && total < icount && !bm.is_set(i); ++i) {
                bm.set(i);
                total++;
            }
        }

        if(total == 0) {
            no++;
            i = 0;
        }
    }

    assert(*_free >= total);
    *_free -= total;
    *count = total;
    if(total == 0)
        return 0;
    uint32_t off = (no - _first) * perblock + i;
    *_first_free = off;
    uint32_t start = off - total;
    return start;
}

void Allocator::free(FSHandle &h, uint32_t start, size_t count) {
    size_t perblock = h.sb().blocksize * 8;
    uint32_t no = _first + start / perblock;
    if(start < *_first_free)
        *_first_free = start;
    *_free += count;
    while(count > 0) {
        Bitmap::word_t *bytes = reinterpret_cast<Bitmap::word_t*>(h.cache().get_block(no, true));
        Bitmap bm(bytes);

        // first, align it to word-size
        uint32_t i = start & (perblock - 1);
        uint32_t begin = i;
        uint32_t end = Math::min(i + count, perblock);
        for(; (i % Bitmap::WORD_BITS) != 0 && i < end; ++i) {
            assert(bm.is_set(i));
            bm.unset(i);
        }

        // now clear in word-steps
        uint32_t wend = end & ~(Bitmap::WORD_BITS - 1);
        for(; i < wend; i += Bitmap::WORD_BITS) {
            assert(bm.is_word_set(i));
            bm.clear_word(i);
        }

        // maybe, there is something left
        for(; i < end; ++i) {
            assert(bm.is_set(i));
            bm.unset(i);
        }

        // to next bitmap block
        count -= i - begin;
        start = (start + perblock - 1) & ~(perblock - 1);
        no++;
    }
}
