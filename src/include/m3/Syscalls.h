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

#include <base/util/String.h>
#include <base/util/CapRngDesc.h>
#include <base/Env.h>
#include <base/KIF.h>
#include <base/PEDesc.h>

#include <m3/com/SendGate.h>
#include <m3/com/GateStream.h>

namespace m3 {

class Env;
class RecvBuf;

class Syscalls {
    friend class Env;

    static constexpr size_t BUFSIZE     = 1024;
    static constexpr size_t MSGSIZE     = 256;

public:
    static Syscalls &get() {
        return _inst;
    }

private:
    explicit Syscalls() : _gate(ObjCap::INVALID, 0, nullptr, DTU::SYSC_EP) {
    }

public:
    Errors::Code activate(size_t ep, capsel_t oldcap, capsel_t newcap);
    Errors::Code createsrv(capsel_t gate, capsel_t srv, const String &name);
    Errors::Code createsess(capsel_t vpe, capsel_t cap, const String &name, const GateOStream &args);
    Errors::Code createsessat(capsel_t srv, capsel_t sess, word_t ident);
    Errors::Code creategate(capsel_t vpe, capsel_t dst, label_t label, size_t ep, word_t credits);
    Errors::Code createvpe(capsel_t vpe, capsel_t mem, const String &name, PEDesc &pe, capsel_t gate, size_t ep);
    Errors::Code createmap(capsel_t vpe, capsel_t mem, capsel_t first, capsel_t pages, capsel_t dst, int perms);
    Errors::Code attachrb(capsel_t vpe, size_t ep, uintptr_t addr, int order, int msgorder, uint flags);
    Errors::Code detachrb(capsel_t vpe, size_t ep);
    Errors::Code exchange(capsel_t vpe, const CapRngDesc &own, const CapRngDesc &other, bool obtain);
    // we need the pid only to support the VPE abstraction on the host
    Errors::Code vpectrl(capsel_t vpe, KIF::Syscall::VPECtrl op, int pid, int *exitcode);
    Errors::Code delegate(capsel_t vpe, capsel_t sess, const CapRngDesc &crd);
    GateIStream delegate(capsel_t vpe, capsel_t sess, const CapRngDesc &crd, const GateOStream &args);
    Errors::Code obtain(capsel_t vpe, capsel_t sess, const CapRngDesc &crd);
    GateIStream obtain(capsel_t vpe, capsel_t sess, const CapRngDesc &crd, const GateOStream &args);
    Errors::Code reqmem(capsel_t cap, size_t size, int perms) {
        return reqmemat(cap, -1, size, perms);
    }
    Errors::Code reqmemat(capsel_t cap, uintptr_t addr, size_t size, int perms);
    Errors::Code derivemem(capsel_t src, capsel_t dst, size_t offset, size_t size, int perms);
    Errors::Code revoke(const CapRngDesc &crd, bool own = true);
    void exit(int exitcode);
    void noop();

private:
    Errors::Code finish(GateIStream &&reply);

    SendGate _gate;
    static Syscalls _inst;
};

}
