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

#include <base/Panic.h>

#include <m3/com/RecvBuf.h>
#include <m3/com/MemGate.h>
#include <m3/session/Pager.h>
#include <m3/Syscalls.h>

namespace m3 {

uint8_t *RecvBuf::allocate(size_t size) {
    // TODO this assumes that we don't VPE::run between SPM and non-SPM PEs
    static uintptr_t nextbuf = 0;
    static size_t total = 0;
    static uintptr_t begin = 0;
    if(nextbuf == 0) {
        if(env()->pe.has_virtmem()) {
            begin = nextbuf = RECVBUF_SPACE;
            total = RECVBUF_SIZE;
        }
        else {
            begin = nextbuf = env()->pe.mem_size() - RECVBUF_SIZE_SPM - env()->secondaryrecvbuf;
            total = RECVBUF_SIZE_SPM + env()->secondaryrecvbuf;
        }
    }

    // TODO atm, the kernel allocates the complete receive buffer space
    size_t left = total - (nextbuf - begin);
    if(size > left)
        PANIC("Not enough receive buffer space for " << size << "b (" << left << "b left)");

    uint8_t *res = reinterpret_cast<uint8_t*>(nextbuf);
    nextbuf += size;
    return res;
}

void RecvBuf::free(uint8_t *) {
    // TODO implement me
}

}
