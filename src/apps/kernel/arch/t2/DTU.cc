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

#include <base/Common.h>
#include <base/util/Sync.h>
#include <base/DTU.h>

#include "pes/PEManager.h"
#include "pes/VPE.h"
#include "DTU.h"
#include "Platform.h"

namespace kernel {

void DTU::init() {
    // nothing to do
}

int DTU::log_to_phys(int pe) {
    static int peids[] = {
        /* 0 */ 4,      // PE 0
        /* 1 */ 5,
        /* 2 */ 6,
        /* 3 */ 7,
        /* 4 */ 8,
        /* 5 */ 9,
        /* 6 */ 10,
        /* 7 */ 11,     // PE 7
        /* 8 */ 1,      // CM
        /* 9 */ 2,      // DRAM
    };
    return peids[pe];
}

void DTU::deprivilege(int) {
    // unsupported
}

void DTU::set_vpeid(const VPEDesc &) {
    // unsupported
}

void DTU::unset_vpeid(const VPEDesc &) {
    // unsupported
}

void DTU::wakeup(const VPEDesc &vpe) {
    // first, invalidate all endpoints to start fresh
    invalidate_eps(vpe);

    // write the core id to the PE
    uint64_t id = log_to_phys(vpe.core);
    m3::Sync::compiler_barrier();
    write_mem(vpe, RT_START, &id, sizeof(id));

    // configure syscall endpoint again
    label_t label = reinterpret_cast<label_t>(&PEManager::get().vpe(vpe.id).syscall_gate());
    config_send_remote(vpe, m3::DTU::SYSC_EP, label,
        Platform::kernel_pe(), Platform::kernel_pe(), m3::DTU::SYSC_EP,
        1 << VPE::SYSC_CREDIT_ORD, 1 << VPE::SYSC_CREDIT_ORD);

    injectIRQ(vpe);
}

void DTU::suspend(const VPEDesc &) {
    // nothing to do
}

void DTU::injectIRQ(const VPEDesc &vpe) {
    // inject an IRQ
    uint64_t val = 1;
    m3::Sync::memory_barrier();
    write_mem(vpe, IRQ_ADDR_EXTERN, &val, sizeof(val));
}

void DTU::set_rw_barrier(const VPEDesc &, uintptr_t) {
    // unsupported
}

void DTU::config_pf_remote(const VPEDesc &, uint64_t, int) {
    // unsupported
}

void DTU::map_page(const VPEDesc &, uintptr_t, uintptr_t, int) {
    // unsupported
}

void DTU::unmap_page(const VPEDesc &, uintptr_t) {
    // unsupported
}

void DTU::invalidate_ep(const VPEDesc &vpe, int ep) {
    alignas(DTU_PKG_SIZE) m3::EPConf conf;
    memset(&conf, 0, sizeof(conf));
    m3::Sync::memory_barrier();
    uintptr_t addr = EPS_START + ep * sizeof(m3::EPConf);
    write_mem(vpe, addr, &conf, sizeof(conf));
}

void DTU::invalidate_eps(const VPEDesc &vpe, int first) {
    alignas(DTU_PKG_SIZE) char eps[EPS_SIZE];
    memset(eps, 0, sizeof(eps));
    m3::Sync::memory_barrier();
    size_t start = first * sizeof(m3::EPConf);
    write_mem(vpe, EPS_START, eps + start, sizeof(eps) - start);
}

void DTU::config_recv_local(int, uintptr_t, uint, uint, int) {
    // nothing to do; everything is always ready and fixed on T2 for receiving
}

void DTU::config_recv_remote(const VPEDesc &, int, uintptr_t, uint, uint, int, bool) {
    // nothing to do; everything is always ready and fixed on T2 for receiving
}

void DTU::config_send(void *e, label_t label, int dstcore, int, int dstep, size_t, word_t credits) {
    m3::EPConf *ep = reinterpret_cast<m3::EPConf*>(e);
    ep->valid = 1;
    ep->dstcore = log_to_phys(dstcore);
    ep->dstep = dstep;
    ep->label = label;
    ep->credits = credits;
}

void DTU::config_send_local(int ep, label_t label, int dstcore, int dstvpe, int dstep,
        size_t msgsize, word_t credits) {
    config_send(m3::eps() + ep, label, dstcore, dstvpe, dstep, msgsize, credits);
}

void DTU::config_send_remote(const VPEDesc &vpe, int ep, label_t label, int dstcore, int dstvpe, int dstep,
        size_t msgsize, word_t credits) {
    alignas(DTU_PKG_SIZE) m3::EPConf conf;
    config_send(&conf, label, dstcore, dstvpe, dstep, msgsize, credits);
    m3::Sync::memory_barrier();
    uintptr_t epaddr = EPS_START + ep * sizeof(m3::EPConf);
    write_mem(vpe, epaddr, &conf, sizeof(conf));
}

void DTU::config_mem(void *e, int dstcore, int, uintptr_t addr, size_t size, int perm) {
    m3::EPConf *ep = reinterpret_cast<m3::EPConf*>(e);
    ep->valid = 1;
    ep->dstcore = log_to_phys(dstcore);
    ep->dstep = 0;
    ep->label = addr | perm;
    ep->credits = size;
}

void DTU::config_mem_local(int ep, int dstcore, int dstvpe, uintptr_t addr, size_t size) {
    config_mem(m3::eps() + ep, dstcore, dstvpe, addr, size, m3::KIF::Perm::RW);
}

void DTU::config_mem_remote(const VPEDesc &vpe, int ep, int dstcore, int dstvpe, uintptr_t addr, size_t size, int perm) {
    alignas(DTU_PKG_SIZE) m3::EPConf conf;
    config_mem(&conf, dstcore, dstvpe, addr, size, perm);
    m3::Sync::memory_barrier();
    uintptr_t epaddr = EPS_START + ep * sizeof(m3::EPConf);
    write_mem(vpe, epaddr, &conf, sizeof(conf));
}

void DTU::send_to(const VPEDesc &vpe, int ep, label_t label, const void *msg, size_t size, label_t replylbl, int replyep) {
    config_send_local(_ep, label, vpe.core, vpe.id, ep, size + m3::DTU::HEADER_SIZE,
        size + m3::DTU::HEADER_SIZE);
    m3::DTU::get().send(_ep, msg, size, replylbl, replyep);
    m3::DTU::get().wait_until_ready(_ep);
}

void DTU::reply_to(const VPEDesc &vpe, int ep, int, word_t, label_t label, const void *msg, size_t size) {
    send_to(vpe, ep, label, msg, size, 0, 0);
}

void DTU::write_mem(const VPEDesc &vpe, uintptr_t addr, const void *data, size_t size) {
    m3::DTU::get().set_target(SLOT_NO, log_to_phys(vpe.core), addr);
    m3::DTU::get().fire(SLOT_NO, m3::DTU::WRITE, data, size);
}

void DTU::read_mem(const VPEDesc &vpe, uintptr_t addr, void *data, size_t size) {
    m3::DTU::get().set_target(SLOT_NO, log_to_phys(vpe.core), addr);
    m3::DTU::get().fire(SLOT_NO, m3::DTU::READ, data, size);
}

}
