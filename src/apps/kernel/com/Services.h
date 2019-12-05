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
#include <base/col/SList.h>
#include <base/util/String.h>
#include <base/util/Reference.h>

#include "mem/SlabCache.h"
#include "SendQueue.h"
#include "Gate.h"
#include "ddl/MHTTypes.h"

namespace kernel {

class VPE;

class Service : public SlabObject<Service>, public m3::SListItem, public m3::RefCounted {
public:
    static const size_t SRV_MSG_SIZE     = 256;

    explicit Service(VPE &vpe, int sel, const m3::String &name, int ep, label_t label,
            int capacity, mht_key_t id)
        : m3::SListItem(), RefCounted(), closing(), _vpe(vpe), _sel(sel), _name(name),
          _sgate(vpe, ep, label), _queue(capacity), _id(id) {
    }
    ~Service();

    VPE &vpe() const {
        return _vpe;
    }
    int selector() const {
        return _sel;
    }
    const m3::String &name() const {
        return _name;
    }
    SendGate &send_gate() const {
        return const_cast<SendGate&>(_sgate);
    }
    mht_key_t id() const {
        return _id;
    }

    int pending() const {
        return _queue.inflight() + _queue.pending();
    }
    void send(RecvGate *rgate, const void *msg, size_t size, bool free) {
        _queue.send(rgate, &_sgate, msg, size, free);
    }
    void received_reply() {
        _queue.received_reply();
    }

    bool closing;

private:
    VPE &_vpe;
    int _sel;
    m3::String _name;
    SendGate _sgate;
    SendQueue _queue;
    mht_key_t _id;
};

class ServiceList {
    explicit ServiceList() : _list() {
    }

public:
    friend class Service;

    using iterator = m3::SList<Service>::iterator;

    static const size_t MAX_SERVICES = 32;

    static ServiceList &get() {
        return _inst;
    }

    iterator begin() {
        return _list.begin();
    }
    iterator end() {
        return _list.end();
    }

    Service *add(VPE &vpe, int sel, const m3::String &name, int ep, label_t label,
        int capacity, mht_key_t id) {
        Service *inst = new Service(vpe, sel, name, ep, label, capacity, id);
        _list.append(inst);
        return inst;
    }
    Service *find(const m3::String &name) {
        for(auto &s : _list) {
            if(s.name() == name)
                return &s;
        }
        return nullptr;
    }
    void send_and_receive(m3::Reference<Service> serv, const void *msg, size_t size, bool free);

private:
    void remove(Service *inst) {
        _list.remove(inst);
    }

    m3::SList<Service> _list;
    static ServiceList _inst;
};

class RemoteServiceList {
    explicit RemoteServiceList() {
    }

public:
    struct RemoteService : m3::SListItem {
        explicit RemoteService(mht_key_t _id, const m3::String &_name) : id(_id), name(_name) {
        }

        mht_key_t id;
        m3::String name;
    };

    using iterator = m3::SList<RemoteService>::iterator;

    static RemoteServiceList &get() {
        return _inst;
    }

    iterator begin() {
        return _list.begin();
    }
    iterator end() {
        return _list.end();
    }

    RemoteService *add(const m3::String &name, mht_key_t id) {
        RemoteService *inst = new RemoteService(id, name);
        _list.append(inst);
        return inst;
    }
    bool exists(const m3::String &name) {
        for(auto &s : _list) {
            if(s.name == name)
                return true;
        }
        return false;
    }
    RemoteService *find(const m3::String &name) {
        for(auto &s : _list) {
            if(s.name == name)
                return &s;
        }
        return nullptr;
    }

private:
    m3::SList<RemoteService> _list;
    static RemoteServiceList _inst;
};

}
