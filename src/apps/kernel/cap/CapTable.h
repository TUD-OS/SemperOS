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
#include <base/col/Treap.h>
#include <base/col/SList.h>
#include <base/util/CapRngDesc.h>

#include "com/Services.h"
#include "cap/Capability.h"
#include "cap/Revocations.h"

namespace kernel {

class CapTable;

m3::OStream &operator<<(m3::OStream &os, const CapTable &ct);

class CapTable {
    friend m3::OStream &operator<<(m3::OStream &os, const CapTable &ct);

    struct Reservation : public m3::SListItem {
        Reservation(capsel_t _key) : key(_key) {}
        capsel_t key;
    };

public:
    explicit CapTable(uint id, m3::CapRngDesc::Type type) : _id(id), _type(type), _caps(), _reserved() {
    }
    CapTable(const CapTable &ct, uint id) = delete;
    ~CapTable() {
        revoke_all();
    }

    uint id() const {
        return _id;
    }
    m3::CapRngDesc::Type type() const {
        return _type;
    }
    bool unused(capsel_t i) const {
        return get(i) == nullptr;
    }
    bool used(capsel_t i) const {
        return get(i) != nullptr;
    }
    bool range_unused(const m3::CapRngDesc &crd) const {
        if(!range_valid(crd))
            return false;
        for(capsel_t i = crd.start(); i < crd.start() + crd.count(); ++i) {
            if(get(i) != nullptr)
                return false;
        }
        return true;
    }
    bool range_used(const m3::CapRngDesc &crd) const {
        if(!range_valid(crd))
            return false;
        for(capsel_t i = crd.start(); i < crd.start() + crd.count(); ++i) {
            if(get(i) == nullptr)
                return false;
        }
        return true;
    }

    Capability *obtain(capsel_t dst, Capability *c);
    void inherit_and_set(Capability *parent, Capability *child, capsel_t dst);
    void setparent_and_set(mht_key_t parent, Capability *child, capsel_t dst);

    // this function is called by the SyscallHandler when starting a revocation
    m3::Errors::Code revoke(const m3::CapRngDesc &crd, bool own);

    // that's the revoke that is called by both the revoke(CapRngDesc, bool) function
    // and the KernelcallHandler when it receives a revoke request
    int revoke(Capability *c, mht_key_t capID, mht_key_t origin);

    Capability *get(capsel_t i) {
        return _caps.find(i);
    }
    const Capability *get(capsel_t i) const {
        return _caps.find(i);
    }
    Capability *get(capsel_t i, unsigned types) {
        Capability *c = get(i);
        if(c == nullptr || !(c->type() & types))
            return nullptr;
        return c;
    }

    void set(UNUSED capsel_t i, Capability *c) {
        assert(get(i) == nullptr);
        if(c) {
            assert(c->table() == this);
            assert(c->sel() == i);
            _caps.insert(c);
            // TODO
            // this only works as long as VPE ID and PE ID are the same
            c->_id = HashUtil::structured_hash(_id, _id, Capability::capTypeToItemType(c->type()), i);
        }
    }
    void unset(capsel_t i) {
        Capability *c = get(i);
        if(c) {
            _caps.remove(c);
            delete c;
        }
    }

    void reserve(capsel_t i) {
        assert(get(i) == nullptr);
        _reserved.append(new Reservation(i));
    }
    void release(capsel_t i) {
        for(auto it = _reserved.begin(); it != _reserved.end(); it++) {
            if(it->key == i) {
                _reserved.remove(&*it);
                delete &*it;
                return;
            }
        }
    }

    void revoke_all();

private:
    static int revoke_rec(Capability *c, mht_key_t origin, m3::CapRngDesc::Type type);
    bool range_valid(const m3::CapRngDesc &crd) const {
        return crd.count() == 0 || crd.start() + crd.count() > crd.start();
    }

    uint _id;
    m3::CapRngDesc::Type _type;
    m3::Treap<Capability> _caps;
    m3::SList<Reservation> _reserved;
};

}
