/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of M3 (Microkernel for Minimalist Manycores).
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

#include <base/Errors.h>
#include <base/Panic.h>
#include <base/col/SList.h>

namespace kernel {

/**
 * The key-value store is currently misused in some cases, just to make things
 * work, thus some functions are really untypical for a kv-store.
 */
template<typename KEY, class VALUE>
class KVStore {
private:
    struct Entry;
public:
    using iterator = typename m3::SList<Entry>::iterator;
    using const_iterator = typename m3::SList<Entry>::const_iterator;

    class Updater {
    public:
        Updater(KVStore &store, KEY &key, VALUE val) : _store(store), _key(key), _val(val){
        }

        // Perform put when assignment happens
        Updater & operator=(VALUE const& rhs)
        {
            _val = rhs;
            _store.put(_key, rhs);
            return *this;
        }

        operator VALUE const&()
        {
            return _val;
        }

        VALUE operator->() const {
            return this->_val;
        }

    private:
        KVStore &_store;
        KEY &_key;
        VALUE _val;
    };

    KVStore() : _store() {
    };
    KVStore(const KVStore &) = delete;
    KVStore &operator=(const KVStore &) = delete;

    m3::Errors::Code put(KEY key, VALUE val){
        // check whether the key is already existing. If so, overwrite the existing entry
        for(auto it = _store.begin(); it != _store.end(); it++){
            if(it->id == key){
                it->val = val;
                return m3::Errors::NO_ERROR;
            }
        }
        // Create a new entry if key did not exist before
        Entry *newEntry = new Entry(key, val);
        _store.append(newEntry);
        return m3::Errors::NO_ERROR;
    }

    VALUE get(KEY key) const {
        auto find = _store.end();
        for(auto it = _store.begin(); it != _store.end(); it++){
            if(it->id == key){
                find = it;
                break;
            }
        }
        // TODO: is it right to panic here?
        if(find == _store.end())
            PANIC("Did not find key " << key << " in KV Store!");
        return find->val;
    }

    iterator begin() {
        return _store.begin();
    }

    iterator end() {
        return _store.end();
    }

    const_iterator begin() const {
        return _store.begin();
    }

    const_iterator end() const {
        return _store.end();
    }

    bool remove(KEY key) {
        Entry *find = nullptr;
        for(auto it = _store.begin(); it != _store.end(); it++){
            if(it->id == key){
                find = it.operator ->();
                break;
            }
        }
        if(find == nullptr)
            return false;

        return _store.remove(find);
    }

    bool exists(KEY key) const {
        for(auto it = _store.begin(); it != _store.end(); it++){
            if(it->id == key){
                return true;
            }
        }
        return false;
    }

    unsigned int size() {
        return _store.length();
    }

    constexpr VALUE operator[](KEY pos) const {
        return get(pos);
    }

    Updater operator[](KEY pos){
        return Updater(*this, pos, get(pos));
    }

private:
    struct Entry : public m3::SListItem {
        explicit Entry(KEY key, VALUE value) : id(key), val(value){
        }
        KEY id;
        VALUE val;
    };

    m3::SList<Entry> _store;
};
}