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

#include <base/util/Sync.h>
#include <base/log/Kernel.h>

#include "com/RecvBufs.h"
#include "pes/PEManager.h"
#include "pes/VPE.h"
#include "DTU.h"
#include "Platform.h"
#include "SyscallHandler.h"

namespace kernel {

void VPE::init() {
    // attach default receive endpoint
    UNUSED m3::Errors::Code res = RecvBufs::attach(
        *this, m3::DTU::DEF_RECVEP, Platform::def_recvbuf(core()) -
        /* HACK: load generator PEs get additional space for receive buffers */
        (name().contains(("loadgen")) ? SECONDARY_RECVBUF_SIZE_SPM : 0),
        DEF_RCVBUF_ORDER, DEF_RCVBUF_ORDER, 0);
    assert(res == m3::Errors::NO_ERROR);

    // configure syscall endpoint
    DTU::get().config_send_remote(
        desc(), m3::DTU::SYSC_EP, reinterpret_cast<label_t>(&syscall_gate()),
        Platform::kernel_pe(), Platform::kernelId(),
        _syscEP, 1 << SYSC_CREDIT_ORD, 1 << SYSC_CREDIT_ORD);
}

void VPE::activate_sysc_ep() {
}

void VPE::start(int argc, UNUSED char **argv, int) {
    // when exiting, the program will release one reference
    ref();

#if defined(__gem5__)
    init_memory(argc, argv ? argv[0] : "");
#endif

    DTU::get().wakeup(desc());

    _state = RUNNING;
    KLOG(VPES, "Started VPE '" << _name << "' [id=" << id() << "]");
}

m3::Errors::Code VPE::xchg_ep(size_t epid, MsgCapability *, MsgCapability *n) {
    KLOG(EPS, "Setting ep " << epid << " of VPE " << id() << " to " << (n ? n->sel() : -1));

    if(n) {
        if(n->type() & Capability::MEM) {
            uintptr_t addr = n->obj->label & ~m3::KIF::Perm::RWX;
            int perm = n->obj->label & m3::KIF::Perm::RWX;
            DTU::get().config_mem_remote(desc(), epid,
                n->obj->core, n->obj->vpe, addr, n->obj->credits, perm);
        }
        else {
            // TODO we could use a logical ep id for receiving credits
            // TODO but we still need to make sure that one can't just activate the cap again to
            // immediately regain the credits
            // TODO we need the max-msg size here
            DTU::get().config_send_remote(desc(), epid,
                n->obj->label, n->obj->core, n->obj->vpe, n->obj->epid,
                n->obj->credits, n->obj->credits);
        }
    }
    else
        DTU::get().invalidate_ep(desc(), epid);
    return m3::Errors::NO_ERROR;
}

VPE::~VPE() {
    KLOG(VPES, "Deleting VPE '" << _name << "' [id=" << id() << "]");
    DTU::get().invalidate_eps(desc());
    detach_rbufs();
    free_reqs();
    _objcaps.revoke_all();
    _mapcaps.revoke_all();
    if(_as) {
        DTU::get().suspend(desc());
        delete _as;
    }
}

}
