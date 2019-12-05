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

#include <base/util/Util.h>
#include <limits.h>
#include <base/log/Kernel.h>
#include <base/util/Random.h>
#include <base/Panic.h>
#include <thread/ThreadManager.h>

#include "ddl/MHTInstance.h"
#include "Coordinator.h"
#include "Kernelcalls.h"
#include "Platform.h"
#include "pes/PEManager.h"

namespace kernel {

MHTInstance *MHTInstance::_inst;
// TODO
MHTInstance::nextResItem MHTInstance::nextIdx;
MHTItem MHTInstance::resItems[3];

MHTInstance::MHTInstance() {
    KLOG(MHT, "Initializing DDL.\n\tID bits=" << ID_BITS << ", max PEs=" << MAX_PES_DDL <<
        ", PE bits=" << PE_BITS << ",\n\tVPE bits=" << VPE_BITS << ", type bits=" <<
        TYPE_BITS << ", hash bits=" << HASH_BITS << ",\n\ttype mask=" << m3::fmt(TYPE_MASK, "0x#", ID_BITS/4));

    memberTable = new membership_entry[Platform::MAX_PES];
    if(!memberTable)
        PANIC("Not enought memory to create membership table");
    // create partitions and fill membership table
    m3::Random::init(1);
    size_t kid = Coordinator::get().kid();
    for(membership_entry::pe_id_t i = 0; i < Platform::pe_count(); i++) {
        memberTable[i] = {i, static_cast<membership_entry::krnl_id_t>(kid), 1, MembershipFlags::NONE};
        PartitionEntry *p = new PartitionEntry(new MHTPartition(i));
        partitions.append(p);
    }

    // handle unused slots in the ID space
    if(Platform::pe_count() < MAX_PES_DDL) {
        // create partitions
        for(membership_entry::pe_id_t i = Platform::pe_count(); i < MAX_PES_DDL; i++) {
            memberTable[i] = {i, static_cast<membership_entry::krnl_id_t>(kid), 1, MembershipFlags::UNPOPULATED};
            PartitionEntry *p = new PartitionEntry(new MHTPartition(i));
            partitions.append(p);
        }
    }
}

m3::Errors::Code MHTInstance::put(MHTItem &&kv_pair) {
    KLOG(MHT, "Put mht_key: " << PRINT_HASH(kv_pair._mht_key));

    MHTPartition *part = findPartition(kv_pair._mht_key);
    if(part) {
        // the partition is local, get it and insert the kv_pair
        return part->put(m3::Util::move(kv_pair));
    } else {
        // the partition is remote, transfer data to the remote node
        membership_entry::krnl_id_t krnlID = responsibleMember(kv_pair._mht_key);
        KPE *dest = Coordinator::get().tryGetKPE(krnlID);
        // check if the partition currently migrates and forward the request if so
        if(dest == nullptr)
            dest = MHTInstance::getInstance().getMigrationDestination(HashUtil::hashToPeId(kv_pair._mht_key));
        if(dest != nullptr)
            Kernelcalls::get().mhtput(dest, m3::Util::move(kv_pair));
        else {
            KLOG(ERR, "Ignoring put request to unknown kernel #" << krnlID);
            return m3::Errors::INV_ARGS;
        }
        return m3::Errors::NO_ERROR;
    }
}

m3::Errors::Code MHTInstance::putUnlocking(MHTItem &&item, uint lockHandle) {
    KLOG(MHT, "PutUnlocking mht_key: " << PRINT_HASH(item._mht_key));

    MHTPartition *part = findPartition(item._mht_key);
    if(part) {
        // the partition is local, get it and insert the kv_pair
        return part->put(m3::Util::move(item), lockHandle);
    } else {
        // the partition is remote, transfer data to the remote node
        membership_entry::krnl_id_t krnlID = responsibleMember(item._mht_key);
        Kernelcalls::get().mhtputUnlocking(Coordinator::get().getKPE(krnlID), m3::Util::move(item), lockHandle);
        return m3::Errors::NO_ERROR;
    }
}

const MHTItem &MHTInstance::localGet(mht_key_t mht_key, bool locking) {
    KLOG(MHT, "Requesting mht_key: " << PRINT_HASH(mht_key));

    MHTPartition *part = findPartition(mht_key);
    assert(part);
    return part->get(mht_key, locking);
}

void MHTInstance::remoteGet(mht_key_t mht_key, bool locking) {
    KLOG(MHT, "Request mht_key: " << PRINT_HASH(mht_key) << " from remote kernel");

    KPE *remoteKrnl = Coordinator::get().getKPE(MHTInstance::getInstance().responsibleMember(mht_key));
    if(!locking)
        Kernelcalls::get().mhtget(remoteKrnl, mht_key);
    else
        Kernelcalls::get().mhtgetLocking(remoteKrnl, mht_key);
}

const MHTItem &MHTInstance::get(mht_key_t mht_key, bool locking) {
    KLOG(MHT, "Requesting mht_key: " << PRINT_HASH(mht_key));

    MHTPartition *part = findPartition(mht_key);
    if(part) { // local partition
        KLOG(MHT,"Request is local");
        // capabilities are stored in CapTables
        // TODO
        // Memory responsibility is to be determined. Due to the interface we have to return
        // a MHTItem reference, but for capabilities there is no persistent MHTItem if we
        // channel accesses to capabilities to the CapTable
        ItemType type = HashUtil::hashToType(mht_key);
        // TODO
        // when restructuring the interface, we also need to incorporate the check if the VPE exists
//        if(!PEManager::get().exists(HashUtil::hashToPeId(mht_key)))
//            return MHTPartition::emptyIndicator;
        if(type & ItemType::GENERICOCAP) {
            // TODO
            // dirty workaround, the interface needs to change
            size_t idx = nextIdx.getnext();
            resItems[idx]._mht_key = mht_key;
            resItems[idx].data = PEManager::get().vpe(HashUtil::hashToPeId(mht_key)).objcaps().get(HashUtil::hashToObjId(mht_key));
            resItems[idx].length = 1;
            return resItems[idx];
        }
        else if(type == MAPCAP) {
            // TODO
            size_t idx = nextIdx.getnext();
            resItems[idx]._mht_key = mht_key;
            resItems[idx].data = PEManager::get().vpe(HashUtil::hashToPeId(mht_key)).mapcaps().get(HashUtil::hashToObjId(mht_key));
            resItems[idx].length = 1;
            return resItems[idx];
        }
        else if(type == SERVICE) {
            ServiceList &list = ServiceList::get();
            for(auto it = list.begin(); it != list.end(); it++) {
                if (it->id() == mht_key) {
                    // TODO
                    size_t idx = nextIdx.getnext();
                    resItems[idx]._mht_key = mht_key;
                    resItems[idx].data = &*it;
                    resItems[idx].length = sizeof(Service*);
                    return resItems[idx];
                }
            }
            return MHTPartition::emptyIndicator;
        }
        else {
            return part->get(mht_key, locking);
        }
    } else {
        KLOG(MHT, "Request is remote");
        // request has to be served by another kernel
        // the request is identified by the current thread's ID
        KPE *remoteKrnl = Coordinator::get().getKPE(responsibleMember(mht_key));
        if(locking)
            Kernelcalls::get().mhtgetLocking(remoteKrnl, mht_key);
        else
            Kernelcalls::get().mhtget(remoteKrnl, mht_key);
        // wait for response
        m3::ThreadManager::get().wait_for(
                reinterpret_cast<void*>(m3::ThreadManager::get().current()->id()));
        // when the thread is resumed the thread's message buffer contains the result
        assert(m3::ThreadManager::get().get_current_msg() != nullptr);
        const MHTItem *res = reinterpret_cast<const MHTItem*>(m3::ThreadManager::get().get_current_msg());
        return *res;
    }
    }

uint MHTInstance::lockLocal(mht_key_t mht_key) {
    MHTPartition *part = findPartition(mht_key);
    int lockHandle = part->lock(mht_key);
    // let this thread wait until the item is unlocked
    if(lockHandle == -1) {
        part->enqueueTicket(mht_key);
        // when we resume the thread, the partition could have been migrated in the meantime
        MHTPartition *part = findPartition(mht_key);
        if(part == nullptr)
            PANIC("Locking key " << PRINT_HASH(mht_key) << " failed! Partition not found. "
                    "Migrating partitions not implemented yet.");
        lockHandle = part->lock(mht_key);
    }
    return lockHandle;
}

uint MHTInstance::lock(mht_key_t mht_key) {
    KLOG(MHT, "Locking key " << PRINT_HASH(mht_key));

    MHTPartition *part = findPartition(mht_key);
    if(part) { // local partition
        return lockLocal(mht_key);
    } else {
        // the partition is remote, forward the request
        KPE *remoteKrnl = Coordinator::get().getKPE(responsibleMember(mht_key));
        Kernelcalls::get().mhtlock(remoteKrnl, mht_key);
        // wait for response
        m3::ThreadManager &tmng = m3::ThreadManager::get();
        tmng.wait_for(reinterpret_cast<void*>(tmng.current()->id()));
        // when resumed the thread msg buffer contains the lockHandle
        assert(tmng.get_current_msg() != nullptr);
        return *reinterpret_cast<uint*>(const_cast<unsigned char*>(tmng.get_current_msg()));
    }
}

bool MHTInstance::unlock(mht_key_t mht_key, uint lockHandle) {
    KLOG(MHT, "Unlocking key " << PRINT_HASH(mht_key));

    if(responsibleMember(mht_key) == Coordinator::get().kid()) {
        return unlockLocal(mht_key, lockHandle);
    } else {
        Kernelcalls::get().mhtunlock(Coordinator::get().getKPE(responsibleMember(mht_key)), mht_key, lockHandle);
        // Note: we assume this to succeed, hence return true
        return true;
    }
}

uint MHTInstance::reserve(mht_key_t mht_key) {
    KLOG(MHT, "Reserving key " << PRINT_HASH(mht_key));

    MHTPartition *part = findPartition(mht_key);
    if(part) {
        return part->reserve(mht_key);
    } else {
        Kernelcalls::get().mhtReserve(Coordinator::get().getKPE(responsibleMember(mht_key)), mht_key);
        // wait for response
        m3::ThreadManager::get().wait_for(
                reinterpret_cast<void*>(m3::ThreadManager::get().current()->id()));
        // thread's message buffer contains the reservation number
        assert(m3::ThreadManager::get().get_current_msg() != nullptr);
        return *reinterpret_cast<const uint*>(m3::ThreadManager::get().get_current_msg());
    }
}

m3::Errors::Code MHTInstance::release(mht_key_t mht_key, uint reservation) {
    KLOG(MHT, "Releasing key " << PRINT_HASH(mht_key));

    MHTPartition *part = findPartition(mht_key);
    if(part) {
        return part->release(mht_key, reservation);
    } else {
        Kernelcalls::get().mhtRelease(Coordinator::get().getKPE(responsibleMember(mht_key)), mht_key, reservation);
        // we do not acknowledge this operation
        return m3::Errors::NO_ERROR;
    }
}

void MHTInstance::migratePartitions(m3::PEDesc pes[], uint numPEs, membership_entry::krnl_id_t receiver) {
    KLOG(MHT, "Migrating " << numPEs << " DDL partitions to kernel #" << (uint)receiver);
    // calculate memory necessary to transmit data
    MHTPartition *parts[numPEs];
    size_t size = 0;
    for(size_t i = 0; i < numPEs; i++) {
        parts[i] = findPartition(HashUtil::structured_hash(pes[i].core_id(), 0, NOTYPE, 0));
        assert(parts[i]);
        size = parts[i]->serializedSize();
    }
    // stream content: number of partitions [<< partition number << number of items << items]
    AutoGateOStream ser(m3::vostreamsize(
        m3::ostreamsize<uint>(), size));
    ser << numPEs;
    for(size_t i = 0; i < numPEs; i++)
        parts[i]->serialize(ser);

    Kernelcalls::get().migratePartition(Coordinator::get().getKPE(receiver), ser);
}

void MHTInstance::receivePartitions(GateIStream& is) {
    uint numPEs;
    is >> numPEs;
    KLOG(MHT, "Receiving " << numPEs << " DDL partitions");
    for(size_t i = 0; i < numPEs; i++) {
        MHTPartition *part = new MHTPartition(0);
        part->deserialize(is);
        assert(findPartition(HashUtil::structured_hash(part->_id, 0, NOTYPE, 0)) == nullptr);
        partitions.append(new PartitionEntry(part));
    }
}

void MHTInstance::updateMembership(membership_entry::pe_id_t start, membership_entry::krnl_id_t krnl,
    membership_entry::pe_id_t krnlCore, membership_entry::capacity_t capacity, MembershipFlags flags,
    bool propagate) {
    assert(capacity <= MAX_PES_DDL);
    // update membership array
    m3::PEDesc releasedPEs[capacity];
    size_t count = 0;
    for(membership_entry::pe_id_t id = start; id < start + capacity; id++, count++) {
        releasedPEs[count] = Platform::pe_by_core(id);
    }
    updateMembership(releasedPEs, start + capacity, krnl, krnlCore, flags, propagate);
}

void MHTInstance::updateMembership(m3::PEDesc releasedPEs[], uint numPEs, membership_entry::krnl_id_t krnl,
    membership_entry::pe_id_t krnlCore, MembershipFlags flags, bool propagate) {
    assert(numPEs <= MAX_PES_DDL);

    // inform other kernels if this is a completed update
    if(propagate && !(flags & MembershipFlags::MIGRATING))
        Coordinator::get().broadcastMemberUpdate(releasedPEs, numPEs, krnl, krnlCore, flags);

    size_t idx = 0;
    for(size_t id = 0; id < numPEs; id++) {
        idx = releasedPEs[id].core_id();
        memberTable[idx].krnl_id = krnl;
        if(flags != NOCHANGE) {
            // if we modify a partition that was migrated, it finished the migration
            // hence, remove it from the migration list
            if(memberTable[idx].flags & MIGRATING){
                for(auto it = _migratingPartitions.begin(); it != _migratingPartitions.end(); ) {
                    auto old = it++;
                    if(old->destKrnl.id() == idx) {
                        _migratingPartitions.remove(&(*old));
                        delete &(*old);
                    }
                }
            }
            if(flags & MIGRATING)
                _migratingPartitions.append(new MigratingPartitionEntry(idx, krnl, krnlCore));
            memberTable[idx].flags = flags;
        }
    }
}

bool MHTInstance::keyLocality(mht_key_t key) {
    if(!(memberTable[HashUtil::hashToPeId(key)].flags & MembershipFlags::MIGRATING))
        return (responsibleMember(key) == Coordinator::get().kid()) ? true : false;
    else
        return false;
}

uint MHTInstance::localPEs() const {
    membership_entry::krnl_id_t kid = Coordinator::get().kid();
    uint count = 0;
    for(uint i = 0; i < Platform::MAX_PES; i++)
        if(memberTable[i].krnl_id == kid && memberTable[i].flags != MembershipFlags::UNPOPULATED &&
            memberTable[i].flags != MembershipFlags::UNMANAGED)
            count++;
    return count;
}

void MHTInstance::printMembership() {
    KLOG(MHT, "--- Membership Table ---\n# | PE Id | kernel | capacity | flags");
    for(uint i = 0; i < (1U << PE_BITS); i++) {
        if(memberTable[i].flags != UNPOPULATED)
            KLOG(MHT, "#" << i << " | " << memberTable[i].pe_id << " | " << memberTable[i].krnl_id <<
                " | " << memberTable[i].capacity << " | " << memberTable[i].flags);
    }
}

MHTPartition* MHTInstance::findPartition(mht_key_t key) {
    if(responsibleMember(key) != Coordinator::get().kid()) {
        KLOG(MHT, "Partition is not stored locally");
        return nullptr;
    }
    membership_entry::pe_id_t peId = HashUtil::hashToPeId(key);
    for(auto it = partitions.begin(); it != partitions.end(); it++) {
        if(it->partition->_id == peId)
            return it->partition;
    }
    KLOG(ERR, "MHT Partition not found! Key: " << PRINT_HASH(key));
    return nullptr;
}

void MHTInstance::printContents() {
    KLOG(MHT, "- Printing content of MHT -");
    for(auto it = partitions.begin(); it != partitions.end(); it++) {
        if(it->partition->_storage.length())
            it->partition->printItems();
    }
}

}
