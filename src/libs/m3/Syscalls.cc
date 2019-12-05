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

#include <base/log/Lib.h>
#include <base/tracing/Tracing.h>
#include <base/Errors.h>
#include <base/Init.h>

#include <m3/com/GateStream.h>
#include <m3/Syscalls.h>

namespace m3 {

INIT_PRIO_SYSC Syscalls Syscalls::_inst;

Errors::Code Syscalls::finish(GateIStream &&reply) {
    if(reply.error())
        return reply.error();
    reply >> Errors::last;
    return Errors::last;
}

void Syscalls::noop() {
    send_receive_vmsg(_gate, KIF::Syscall::NOOP);
}

Errors::Code Syscalls::activate(size_t ep, capsel_t oldcap, capsel_t newcap) {
    LLOG(SYSC, "activate(ep=" << ep << ", oldcap=" << oldcap << ", newcap=" << newcap << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::ACTIVATE, ep, oldcap, newcap));
}

Errors::Code Syscalls::createsrv(capsel_t gate, capsel_t srv, const String &name) {
    LLOG(SYSC, "createsrv(gate=" << gate << ", srv=" << srv << ", name=" << name << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::CREATESRV, gate, srv, name));
}

Errors::Code Syscalls::createsessat(capsel_t srv, capsel_t sess, word_t ident) {
    LLOG(SYSC, "createsessat(srv=" << srv << ", sess=" << sess << ", ident=" << fmt(ident, "0x") << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::CREATESESSAT, srv, sess, ident));
}

Errors::Code Syscalls::createsess(capsel_t vpe, capsel_t cap, const String &name, const GateOStream &args) {
    LLOG(SYSC, "createsess(vpe=" << vpe << ", cap=" << cap << ", name=" << name
        << ", argc=" << args.total() << ")");
    AutoGateOStream msg(vostreamsize(
        ostreamsize<KIF::Syscall::Operation, capsel_t, capsel_t, size_t>(), name.length(), args.total()));
    msg << KIF::Syscall::CREATESESS << vpe << cap << name;
    msg.put(args);
    return finish(send_receive_msg(_gate, msg.bytes(), msg.total()));
}

Errors::Code Syscalls::creategate(capsel_t vpe, capsel_t dst, label_t label, size_t ep, word_t credits) {
    LLOG(SYSC, "creategate(vpe=" << vpe << ", dst=" << dst << ", label=" << fmt(label, "#x")
        << ", ep=" << ep << ", credits=" << credits << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::CREATEGATE, vpe, dst, label, ep, credits));
}

Errors::Code Syscalls::createmap(capsel_t vpe, capsel_t mem, capsel_t first, capsel_t pages, capsel_t dst, int perms) {
    LLOG(SYSC, "createmap(vpe=" << vpe << ", mem=" << mem << ", first=" << first
        << ", pages=" << pages << ", dst=" << dst << ", perms=" << perms << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::CREATEMAP, vpe, mem, first, pages, dst, perms));
}

Errors::Code Syscalls::createvpe(capsel_t vpe, capsel_t mem, const String &name, PEDesc &pe, capsel_t gate, size_t ep) {
    LLOG(SYSC, "createvpe(vpe=" << vpe << ", mem=" << mem << ", name=" << name
        << ", type=" << static_cast<int>(pe.type()) << ", pfgate=" << gate << ", pfep=" << ep << ")");
    GateIStream reply = send_receive_vmsg(_gate, KIF::Syscall::CREATEVPE,
        vpe, mem, name, pe.value(), gate, ep);
    if(reply.error())
        return reply.error();
    reply >> Errors::last;
    if(Errors::last == Errors::NO_ERROR) {
        PEDesc::value_t peval;
        reply >> peval;
        pe = PEDesc(peval);
    }
    return Errors::last;
}

Errors::Code Syscalls::attachrb(capsel_t vpe, size_t ep, uintptr_t addr, int order, int msgorder, uint flags) {
    LLOG(SYSC, "attachrb(vpe=" << vpe << ", ep=" << ep << ", addr=" << fmt(addr, "p")
        << ", size=" << fmt(1UL << order, "x") << ", msgsize=" << fmt(1UL << msgorder, "x")
        << ", flags=" << fmt(flags, "x") << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::ATTACHRB, vpe, ep, addr, order, msgorder, flags));
}

Errors::Code Syscalls::detachrb(capsel_t vpe, size_t ep) {
    LLOG(SYSC, "detachrb(vpe=" << vpe << ", ep=" << ep << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::DETACHRB, vpe, ep));
}

Errors::Code Syscalls::exchange(capsel_t vpe, const CapRngDesc &own, const CapRngDesc &other, bool obtain) {
    LLOG(SYSC, "exchange(vpe=" << vpe << ", own=" << own << ", other=" << other
        << ", obtain=" << obtain << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::EXCHANGE, vpe, own, other, obtain));
}

Errors::Code Syscalls::vpectrl(capsel_t vpe, KIF::Syscall::VPECtrl op, int pid, int *exitcode) {
    LLOG(SYSC, "vpectrl(vpe=" << vpe << ", op=" << op << ", pid=" << pid << ")");
    GateIStream &&reply = send_receive_vmsg(_gate, KIF::Syscall::VPECTRL, vpe, op, pid);
    reply >> Errors::last;
    if(op == KIF::Syscall::VCTRL_WAIT && Errors::last == Errors::NO_ERROR)
        reply >> *exitcode;
    return Errors::last;
}

Errors::Code Syscalls::delegate(capsel_t vpe, capsel_t sess, const CapRngDesc &crd) {
    LLOG(SYSC, "delegate(vpe=" << vpe << ", sess=" << sess << ", crd=" << crd << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::DELEGATE, vpe, sess, crd));
}

GateIStream Syscalls::delegate(capsel_t vpe, capsel_t sess, const CapRngDesc &crd, const GateOStream &args) {
    LLOG(SYSC, "delegate(vpe=" << vpe << ", sess=" << sess << ", crd=" << crd << ", argc=" << args.total() << ")");
    AutoGateOStream msg(vostreamsize(ostreamsize<KIF::Syscall::Operation, capsel_t, capsel_t, CapRngDesc>(),
        args.total()));
    msg << KIF::Syscall::DELEGATE << vpe << sess << crd;
    msg.put(args);
    return send_receive_msg(_gate, msg.bytes(), msg.total());
}

Errors::Code Syscalls::obtain(capsel_t vpe, capsel_t sess, const CapRngDesc &crd) {
    LLOG(SYSC, "obtain(vpe=" << vpe << ", sess=" << sess << ", crd=" << crd << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::OBTAIN, vpe, sess, crd));
}

GateIStream Syscalls::obtain(capsel_t vpe, capsel_t sess, const CapRngDesc &crd, const GateOStream &args) {
    LLOG(SYSC, "obtain(vpe=" << vpe << ", sess=" << sess << ", crd=" << crd << ", argc=" << args.total() << ")");
    AutoGateOStream msg(vostreamsize(ostreamsize<KIF::Syscall::Operation, capsel_t, capsel_t, CapRngDesc>(),
        args.total()));
    msg << KIF::Syscall::OBTAIN << vpe << sess << crd;
    msg.put(args);
    return send_receive_msg(_gate, msg.bytes(), msg.total());
}

Errors::Code Syscalls::reqmemat(capsel_t cap, uintptr_t addr, size_t size, int perms) {
    LLOG(SYSC, "reqmem(cap=" << cap << ", addr=" << addr << ", size=" << size
        << ", perms=" << perms << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::REQMEM, cap, addr, size, perms));
}

Errors::Code Syscalls::derivemem(capsel_t src, capsel_t dst, size_t offset, size_t size, int perms) {
    LLOG(SYSC, "derivemem(src=" << src << ", dst=" << dst << ", off=" << offset
            << ", size=" << size << ", perms=" << perms << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::DERIVEMEM, src, dst, offset, size, perms));
}

Errors::Code Syscalls::revoke(const CapRngDesc &crd, bool own) {
    LLOG(SYSC, "revoke(crd=" << crd << ", own=" << own << ")");
    return finish(send_receive_vmsg(_gate, KIF::Syscall::REVOKE, crd, own));
}

// the USED seems to be necessary, because the libc calls it and LTO removes it otherwise
USED void Syscalls::exit(int exitcode) {
    LLOG(SYSC, "exit(code=" << exitcode << ")");
    EVENT_TRACE_FLUSH();
    send_vmsg(_gate, KIF::Syscall::EXIT, exitcode);
}

}
