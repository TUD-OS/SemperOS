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
#include <m3/com/SendGate.h>
#include <m3/com/RecvGate.h>
#include <m3/com/GateStream.h>

class CapServerSession : public m3::Session {
public:
    CapServerSession(const m3::String &name) : m3::Session(name),
            _sgate(m3::SendGate::bind(obtain(1).start())) {
    }

enum CapOperation {
    SIGNAL,
    COUNT
};

void signal() {
    m3::send_vmsg(_sgate, CapOperation::SIGNAL);
}

private:
    m3::SendGate _sgate;

};
