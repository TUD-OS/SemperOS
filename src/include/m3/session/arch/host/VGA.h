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

namespace m3 {

class VGA : public Session {
public:
    static constexpr int COLS = 80;
    static constexpr int ROWS = 30;
    static constexpr size_t SIZE = ROWS * COLS * 2;

    explicit VGA(const String &service) : Session(service), _gate(MemGate::bind(obtain(1).start())) {
    }

    MemGate &gate() {
        return _gate;
    }

private:
    MemGate _gate;
};

}
