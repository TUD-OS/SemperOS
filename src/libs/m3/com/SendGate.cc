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

#include <m3/com/SendGate.h>
#include <m3/Syscalls.h>
#include <m3/VPE.h>

#include <assert.h>

namespace m3 {

SendGate SendGate::create(word_t credits, RecvGate *rcvgate, capsel_t sel) {
    rcvgate = rcvgate == nullptr ? &RecvGate::def() : rcvgate;
    return create_for(VPE::self(), rcvgate->epid(), rcvgate->label(), credits, rcvgate, sel);
}

SendGate SendGate::create_for(const VPE &vpe, size_t dstep, label_t label, word_t credits,
        RecvGate *rcvgate, capsel_t sel) {
    uint flags = 0;
    rcvgate = rcvgate == nullptr ? &RecvGate::def() : rcvgate;
    if(sel == INVALID)
        sel = VPE::self().alloc_cap();
    else
        flags |= KEEP_SEL;
    // when we create a send gate for one of our endpoints, it has to be bound to an endpoint (and stay
    // there) because somebody else wants to send us messages to this (or better: to the attached
    // receive gate)
    SendGate gate(sel, flags, rcvgate);
    Syscalls::get().creategate(vpe.sel(), gate.sel(), label, dstep, credits);
    return gate;
}

}
