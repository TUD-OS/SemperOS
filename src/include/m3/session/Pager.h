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

#include <m3/session/Session.h>
#include <m3/com/MemGate.h>
#include <m3/com/SendGate.h>

namespace m3 {

class Pager : public Session {
private:
    explicit Pager(capsel_t sess)
        : Session(sess, 0), _gate(SendGate::bind(obtain(1).start())) {
    }

public:
    enum Operation {
        PAGEFAULT,
        CLONE,
        MAP_ANON,
        UNMAP,
        COUNT,
    };

    enum Prot {
        READ    = MemGate::R,
        WRITE   = MemGate::W,
        EXEC    = MemGate::X,
    };

    explicit Pager(capsel_t sess, capsel_t gate)
        : Session(sess), _gate(SendGate::bind(gate)) {
    }
    explicit Pager(const String &service)
        : Session(service), _gate(SendGate::bind(obtain(1).start())) {
    }

    const SendGate &gate() const {
        return _gate;
    }

    Pager *create_clone();
    Errors::Code clone();
    Errors::Code pagefault(uintptr_t addr, uint access);
    Errors::Code map_anon(uintptr_t *virt, size_t len, int prot, int flags);
    Errors::Code map_ds(uintptr_t *virt, size_t len, int prot, int flags, const Session &sess,
        int fd, size_t offset);
    Errors::Code unmap(uintptr_t virt);

private:
    SendGate _gate;
};

}
