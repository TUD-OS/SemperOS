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

#if defined(__host__)

#include <base/log/Lib.h>
#include <base/Init.h>
#include <base/DTU.h>

#include <m3/com/SendQueue.h>

namespace m3 {

INIT_PRIO_SENDQUEUE SendQueue SendQueue::_inst;

void SendQueue::work() {
    if(_queue.length() > 0) {
        if(DTU::get().is_ready()) {
            SendItem *it = _queue.remove_first();
            LLOG(IPC, "Removing " << it << " from queue");
            delete it;
            if(_queue.length() > 0) {
                SendItem &first = *_queue.begin();
                first.gate.send(first.data, first.len);
                LLOG(IPC, "Sending " << &first << " from queue");
            }
        }
    }
}

void SendQueue::send_async(SendItem &it) {
    it.gate.send(it.data, it.len);
    LLOG(IPC, "Sending " << &it << " from queue");
}

}

#endif
