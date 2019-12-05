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

#include <base/log/Kernel.h>
#include <base/util/Math.h>

#include <assert.h>

#include "mem/MemoryMap.h"

namespace kernel {

MemoryMap::MemoryMap(uintptr_t addr, size_t size) : list(new Area()) {
    list->addr = addr;
    list->size = size;
    list->next = nullptr;
}

MemoryMap::~MemoryMap() {
    for(Area *a = list; a != nullptr;) {
        Area *n = a->next;
        delete a;
        a = n;
    }
    list = nullptr;
}

uintptr_t MemoryMap::allocate(size_t size) {
    Area *a;
    Area *p = nullptr;
    for(a = list; a != nullptr; p = a, a = a->next) {
        if(a->size >= size)
            break;
    }
    if(a == nullptr)
        return -1;

    /* take it from the front */
    uintptr_t res = a->addr;
    a->size -= size;
    a->addr += size;
    /* if the area is empty now, remove it */
    if(a->size == 0) {
        if(p)
            p->next = a->next;
        else
            list = a->next;
        delete a;
    }
    KLOG(MEM, "Requested " << (size / 1024) << " KiB of memory @ " << m3::fmt(res, "p"));
    return res;
}

void MemoryMap::free(uintptr_t addr, size_t size) {
    KLOG(MEM, "Free'd " << (size / 1024) << " KiB of memory @ " << m3::fmt(addr, "p"));

    /* find the area behind ours */
    Area *n, *p = nullptr;
    for(n = list; n != nullptr && addr > n->addr; p = n, n = n->next)
        ;

    /* merge with prev and next */
    if(p && p->addr + p->size == addr && n && addr + size == n->addr) {
        p->size += size + n->size;
        p->next = n->next;
        delete n;
    }
    /* merge with prev */
    else if(p && p->addr + p->size == addr) {
        p->size += size;
    }
    /* merge with next */
    else if(n && addr + size == n->addr) {
        n->addr -= size;
        n->size += size;
    }
    /* create new area between them */
    else {
        Area *a = new Area();
        a->addr = addr;
        a->size = size;
        if(p)
            p->next = a;
        else
            list = a;
        a->next = n;
    }
}

uintptr_t MemoryMap::detach(size_t size) {
    /* detached are must be multiple of page size */
    assert(!(size & PAGE_MASK));
    Area *a;
    uint count = 0;
    for(a = list; a != nullptr; a = a->next) {
        count++;
    }
    if(count == 0)
        return -1;

    /* find the rearmost fitting area */
    Area *p = a;
    for(int i = count; i >= 0; i--) {
        int k;
        for(a = list, k = 1; k < i; k++, p = a, a = a->next) {
        }
        if( (a->size - (m3::Math::round_up<uintptr_t>(a->addr, PAGE_SIZE) - a->addr) ) >= size)
            break;
        /* no area with sufficient size */
        else if(i == 0)
            return -1;
    }

    /* take it from the end */
    uintptr_t res;
    /* the end is not page aligned */
    if((static_cast<size_t>(a->addr) + a->size) & PAGE_MASK) {
        res = m3::Math::round_dn<uintptr_t>(a->addr + static_cast<uintptr_t>(a->size), PAGE_SIZE) - size;
        /* put the remaining tail in a new area */
        Area *t = new Area();
        t->addr = res + size;
        t->size = (a->addr + a->size) - (res + size);
        t->next = a->next;

        a->next = t;
        a->size -= (size + t->size);
    }
    else {
        res = a->addr + a->size - size;
        a->size -= size;
    }
    /* if the area is empty now, remove it */
    if(a->size == 0) {
        if(p)
            p->next = a->next;
        else
            list = a->next;
        delete a;
    }
    KLOG(MEM, "Detached " << (size / 1024) << " KiB of memory @ " << m3::fmt(res, "p"));
    return res;
}

size_t MemoryMap::get_size(size_t *areas) const {
    size_t total = 0;
    if(areas)
        *areas = 0;
    for(Area *a = list; a != nullptr; a = a->next) {
        total += a->size;
        if(areas)
            (*areas)++;
    }
    return total;
}

}
