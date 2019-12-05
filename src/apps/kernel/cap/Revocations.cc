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

#include "cap/Revocations.h"
#include "ddl/MHTInstance.h"

namespace kernel {

RevocationList RevocationList::_inst;

void Revocation::notifySubscribers() {
    for (auto sub = subscribers.begin(); sub != subscribers.end();) {
        auto curSub = sub++;
        for (auto it = RevocationList::get().begin(); it != RevocationList::get().end(); it++) {
            if (it->capID == curSub->rev->capID) {
                it->awaitedResp--;

                if (it->awaitedResp == 0) {
                    // notify our subscribers too
                    // Note: this will inform local parents
                    it->notifySubscribers();

                    // If the parent is local
                    membership_entry::krnl_id_t parentAuthority =
                        MHTInstance::getInstance().responsibleKrnl(HashUtil::hashToPeId(it->parent));

                    // if there is a thread ID this is the entry of a revocation root
                    if(it->tid != -1) {
                        assert(it->capID == it->origin);
                        m3::ThreadManager::get().notify(reinterpret_cast<void*> (it->tid));
                    }
                    else if(parentAuthority != Coordinator::get().kid()) {
                        // we only need to inform remote parents here
                        // if we have a local parent it subscribed to this
                        // revocation before and is informed by notifySubscribers()
                        assert(it->parent != 0);
                        Kernelcalls::get().revokeFinish(Coordinator::get().getKPE(parentAuthority),
                            it->parent, -1, false);
                    }
                }

                subscribers.remove(&*curSub);
                delete &*curSub;
                break;
            }
        }
    }
}


}
