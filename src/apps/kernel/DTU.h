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

#include <base/Common.h>
#include <base/DTU.h>

namespace kernel {

struct VPEDesc;
struct KernelAllocation;

class DTU {
    explicit DTU() : _next_ep(FIRST_FREE_EP), _ep(alloc_ep()) {
        init();
    }

public:
    static const int SYSC_GATES             = 6;
    static const int KRNLC_EP               = m3::DTU::SYSC_EP + SYSC_GATES;    // kernelcall endpoint, only reserved on kernel PEs
    static const int KRNLC_GATES            = 8;
    static const int SETUP_EP               = KRNLC_EP + KRNLC_GATES - 1;   // EP for the first msg of a newly started kernel goes to
    static const int FIRST_FREE_EP          = SETUP_EP + 1;

    static DTU &get() {
        return _inst;
    }

    int alloc_ep() {
        return _next_ep++;
    }

    void init();

    int log_to_phys(int pe);

    void deprivilege(int core);
    void privilege(int core);

    void set_vpeid(const VPEDesc &vpe);
    void unset_vpeid(const VPEDesc &vpe);

    void wakeup(const VPEDesc &vpe);
    void suspend(const VPEDesc &vpe);
    void injectIRQ(const VPEDesc &vpe);

    void set_rw_barrier(const VPEDesc &vpe, uintptr_t addr);

    void config_pf_remote(const VPEDesc &vpe, uint64_t rootpt, int ep);
    void config_pt_remote(const VPEDesc &vpe, uint64_t rootpt);
    void map_page(const VPEDesc &vpe, uintptr_t virt, uintptr_t phys, int perm);
    void map_kernel_page(const VPEDesc &vpe, uintptr_t virt, uintptr_t phys, int perm,
        KernelAllocation& kernMem);
    void unmap_page(const VPEDesc &vpe, uintptr_t virt);

    void invalidate_ep(const VPEDesc &vpe, int ep);
    void invalidate_eps(const VPEDesc &vpe, int first = 0);

    void config_recv_local(int ep, uintptr_t buf, uint order, uint msgorder, int flags);
    void config_recv_remote(const VPEDesc &vpe, int ep, uintptr_t buf, uint order, uint msgorder,
        int flags, bool valid);

    void config_send_local(int ep, label_t label, int dstcore, int dstvpe,
        int dstep, size_t msgsize, word_t credits);
    void config_send_remote(const VPEDesc &vpe, int ep, label_t label, int dstcore, int dstvpe,
        int dstep, size_t msgsize, word_t credits);

    void config_mem_local(int ep, int dstcore, int dstvpe, uintptr_t addr, size_t size);
    void config_mem_remote(const VPEDesc &vpe, int ep, int dstcore, int dstvpe,
        uintptr_t addr, size_t size, int perm);

    void send_to(const VPEDesc &vpe, int ep, label_t label, const void *msg, size_t size,
        label_t replylbl, int replyep);
    void reply_to(const VPEDesc &vpe, int ep, int crdep, word_t credits, label_t label,
        const void *msg, size_t size);

    void write_mem(const VPEDesc &vpe, uintptr_t addr, const void *data, size_t size);
    void read_mem(const VPEDesc &vpe, uintptr_t addr, void *data, size_t size);

    void cmpxchg_mem(const VPEDesc &vpe, uintptr_t addr, const void *data, size_t datasize,
        size_t off, size_t size);

private:
#if defined(__gem5__)
    void do_set_vpeid(const VPEDesc &vpe, int newVPE);
    void do_ext_cmd(const VPEDesc &vpe, m3::DTU::reg_t cmd);
    void clear_pt(uintptr_t pt);
#endif

    void config_recv(void *e, uintptr_t buf, uint order, uint msgorder, int flags);
    void config_send(void *e, label_t label, int dstcore, int dstvpe, int dstep,
        size_t msgsize, word_t credits);
    void config_mem(void *e, int dstcore, int dstvpe, uintptr_t addr, size_t size, int perm);

    int _next_ep;
    int _ep;
    static DTU _inst;
};

}
