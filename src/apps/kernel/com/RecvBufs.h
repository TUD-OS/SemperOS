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
#include <base/util/Math.h>
#include <base/util/Subscriber.h>
#include <base/col/SList.h>
#include <base/Config.h>
#include <base/DTU.h>
#include <base/Errors.h>

#include "pes/VPE.h"
#include "DTU.h"
#include "Platform.h"

namespace kernel {

class RecvBufs {
    RecvBufs() = delete;

    enum Flags {
        F_ATTACHED  = 1 << (sizeof(int) * 8 - 1),
    };

    struct RBuf {
        explicit RBuf(size_t _coreid, size_t _epid, uintptr_t _addr, int _order, int _msgorder, uint _flags)
            : coreid(_coreid), epid(_epid), addr(_addr), order(_order), msgorder(_msgorder), flags(_flags) {
        }

        size_t coreid;
        size_t epid;
        uintptr_t addr;
        int order;
        int msgorder;
        int flags;
        m3::Subscriptions<bool> waitlist;
    };

public:
    static void init() {
        // Hack:
        // Figure out the highest core ID because we don't see the whole system
        // if we are not the initial kernel
        size_t highCoreID = 0;
        for(size_t i = 0; i < Platform::pe_count(); i++) {
            size_t coreid = Platform::pe_by_index(i).core_id();
            if(coreid > highCoreID)
                highCoreID = coreid;
        }

        _rbufs = static_cast<RBuf**>(m3::Heap::alloc(sizeof(RBuf*) * highCoreID * EP_COUNT));
        if(!_rbufs)
            PANIC("Could not allocate space for receive buffers");

        for(size_t i = 0; i <= highCoreID; i++)
            for(size_t j = 0; j < EP_COUNT; j++) {
                _rbufs[(i * EP_COUNT) + j] = new RBuf(i, j, 0, 0, 0, 0);
                assert(_rbufs[(i * EP_COUNT) + j] != nullptr);
            }
    }

    static bool is_attached(size_t core, size_t epid) {
        RBuf &rbuf = get(core, epid);
        return rbuf.flags & F_ATTACHED;
    }

    static void subscribe(size_t core, size_t epid, const m3::Subscriptions<bool>::callback_type &cb) {
        RBuf &rbuf = get(core, epid);
        assert(~rbuf.flags & F_ATTACHED);
        rbuf.waitlist.subscribe(cb);
    }

    static m3::Errors::Code attach(VPE &vpe, size_t epid, uintptr_t addr, int order, int msgorder, uint flags) {
        RBuf &rbuf = get(vpe.core(), epid);
        if(rbuf.flags & F_ATTACHED)
            return m3::Errors::EXISTS;

        for(size_t i = 0; i < EP_COUNT; ++i) {
            if(i != epid) {
                RBuf &rb = get(vpe.core(), i);
                if((rb.flags & F_ATTACHED) &&
                    m3::Math::overlap(rb.addr, rb.addr + (1UL << rb.order), addr, addr + (1UL << order)))
                    return m3::Errors::INV_ARGS;
            }
        }

        rbuf.addr = addr;
        rbuf.order = order;
        rbuf.msgorder = msgorder;
        rbuf.flags = flags | F_ATTACHED;
        configure(vpe, epid, rbuf);
        notify(rbuf, true);
        return m3::Errors::NO_ERROR;
    }

    static void detach(VPE &vpe, size_t epid) {
        RBuf &rbuf = get(vpe.core(), epid);
        if(rbuf.flags & F_ATTACHED) {
            // TODO we have to make sure here that nobody can send to that EP anymore
            // BEFORE detaching it!
            rbuf.flags = 0;
            configure(vpe, epid, rbuf);
        }
        notify(rbuf, false);
    }

private:
    static void configure(VPE &vpe, size_t epid, RBuf &rbuf) {
        DTU::get().config_recv_remote(vpe.desc(), epid,
            rbuf.addr, rbuf.order, rbuf.msgorder, rbuf.flags & ~F_ATTACHED, rbuf.flags & F_ATTACHED);
    }

    static void notify(RBuf &rbuf, bool success) {
        for(auto sub = rbuf.waitlist.begin(); sub != rbuf.waitlist.end(); ) {
            auto old = sub++;
            old->callback(success, nullptr);
            rbuf.waitlist.unsubscribe(&*old);
        }
    }
    static RBuf &get(size_t coreid, size_t epid) {
        assert(_rbufs[coreid + epid] != nullptr);
        return *_rbufs[(coreid * EP_COUNT) + epid];
    }

    static RBuf **_rbufs;
};

}
