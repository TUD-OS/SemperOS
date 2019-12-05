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

#include <base/col/SList.h>

#include "ddl/MHTTypes.h"

namespace kernel {

struct Revocation;

struct RevocationSub : m3::SListItem {
    explicit RevocationSub(Revocation *_rev) : rev(_rev) {}

    Revocation *rev;
};

struct Revocation : m3::SListItem {
    explicit Revocation(mht_key_t _capID, mht_key_t _parent, mht_key_t _origin, int _awaitedResp, int _tid)
    : capID(_capID), parent(_parent), origin(_origin), awaitedResp(_awaitedResp), tid(_tid), subscribers() {
#ifndef NDEBUG
        // correctness checks
        // we only use generic cap IDs for revocations
        if(HashUtil::hashToType(_capID) == ItemType::MAPCAP)
            assert((_capID & TYPE_MASK_MCAP) == TYPE_MASK_MCAP);
        else
            assert((_capID & TYPE_MASK_OCAP) == TYPE_MASK_OCAP);
        if(HashUtil::hashToType(_parent) == ItemType::MAPCAP)
            assert((_parent & TYPE_MASK_MCAP) == TYPE_MASK_MCAP);
        else
            assert((_parent & TYPE_MASK_OCAP) == TYPE_MASK_OCAP);
        if(HashUtil::hashToType(_origin) == ItemType::MAPCAP)
            assert((_origin & TYPE_MASK_MCAP) == TYPE_MASK_MCAP);
        else
            assert((_origin & TYPE_MASK_OCAP) == TYPE_MASK_OCAP);
#endif
    }

    void subscribe(Revocation *sub) {
        subscribers.append(new RevocationSub(sub));
    }
    void notifySubscribers();

    mht_key_t capID;
    mht_key_t parent;
    mht_key_t origin; // cap which started revocation
    int awaitedResp; // own awaited resps
    int tid; // tid of origin's thread
    m3::SList<RevocationSub> subscribers; // revocations waiting for this one to finish
};

class RevocationList {
    explicit RevocationList() : _list() {
    }
public:

    using iterator = m3::SList<Revocation>::iterator;

    static RevocationList &get() {
        return _inst;
    }

    iterator begin() {
        return _list.begin();
    }
    iterator end() {
        return _list.end();
    }

    Revocation *add(mht_key_t cap, mht_key_t parent, mht_key_t origin) {
#ifndef NDEBUG
        // make sure there's only one entry per cap ID, otherwise the revoke algorithm fails
        for(auto &it : _list)
            if(it.capID == cap)
                PANIC("Cannot insert second entry for revocation of cap: " << PRINT_HASH(cap));
#endif
        return &*_list.append(new Revocation(cap, parent, origin, 0,
                (origin == cap) ? m3::ThreadManager::get().current()->id() : -1));
    }

    Revocation *find(mht_key_t cap) {
        cap = (HashUtil::hashToType(cap) == ItemType::MAPCAP) ?
            (cap | TYPE_MASK_MCAP) : (cap | TYPE_MASK_OCAP);
        for(auto it = _list.begin(); it != _list.end(); it++) {
            if(it->capID == cap)
                return &*it;
        }
        return nullptr;
    }

    /**
     * Deletes the ongoing revocation of the given cap with the given origin.
     *
     * @param cap       The cap ID to remove
     */
    void remove(mht_key_t cap) {
        for(auto it = _list.begin(); it != _list.end(); it++) {
            if(it->capID == cap) {
                _list.remove(&*it);
                delete &*it;
                return;
            }
        }
    }

private:
    m3::SList<Revocation> _list;
    static RevocationList _inst;
};

}
