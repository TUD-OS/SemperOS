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

#include <base/arch/host/HWInterrupts.h>

#include <m3/com/Gate.h>
#include <m3/com/RecvBuf.h>
#include <m3/session/Session.h>
#include <m3/VPE.h>

namespace m3 {

class Interrupts : public Session {
public:
    explicit Interrupts(const String &service, HWInterrupts::IRQ irq, int buford = nextlog2<256>::val,
            int msgord = nextlog2<64>::val)
        : Session(service, create_vmsg(irq)),
          _rcvbuf(RecvBuf::create(VPE::self().alloc_ep(), buford, msgord, 0)),
          _rgate(RecvGate::create(&_rcvbuf)), _sgate(SendGate::create(SendGate::UNLIMITED, &_rgate)) {
        if(!Errors::occurred())
            delegate_obj(_sgate.sel());
    }

    RecvGate &gate() {
        return _rgate;
    }

private:
    RecvBuf _rcvbuf;
    RecvGate _rgate;
    SendGate _sgate;
};

}
