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
#include <base/DTU.h>

namespace m3 {

class WorkLoop;

class WorkItem : public SListItem {
    friend class WorkLoop;
public:
    virtual ~WorkItem() {
    }

    virtual void work() = 0;
};

class WorkLoop {
    static const size_t MAX_ITEMS   = 16;

public:
    explicit WorkLoop() : _changed(false), _permanents(0), _count(), _items() {
    }

    bool has_items() const {
        return _count > _permanents;
    }

    void add(WorkItem *item, bool permanent);
    void remove(WorkItem *item);

    virtual void run();
    void stop() {
        _permanents = _count;
    }

private:
    bool _changed;
    uint _permanents;
    size_t _count;
    WorkItem *_items[MAX_ITEMS];
};

}
