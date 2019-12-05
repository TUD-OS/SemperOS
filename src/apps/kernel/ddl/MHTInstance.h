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

#include <base/col/SList.h>
#include <base/util/Util.h>
#include <functional>

#include "pes/KPE.h"
#include "ddl/MHTTypes.h"
#include "ddl/MHTPartition.h"
#include "KernelcallHandler.h"
#include "Coordinator.h"
#include "Platform.h"

namespace kernel {

struct PartitionEntry : public m3::SListItem {
    explicit PartitionEntry(MHTPartition *part) : partition(part) {}
    MHTPartition* partition;
};

class MHTInstance {
    friend KernelcallHandler;
    friend KPE;
public:
    MHTInstance(const MHTInstance &) = delete;
    MHTInstance &operator=(const MHTInstance &) = delete;

    static void create(){
        _inst = new MHTInstance();
    }
    static void create(uint64_t memberTab, uint64_t parts, size_t partsSize){
        _inst = new MHTInstance(memberTab, parts, partsSize);
    }

    static MHTInstance& getInstance(){
        return *_inst;
    }

    m3::Errors::Code put(MHTItem &&kv_pair);
    m3::Errors::Code putUnlocking(MHTItem &&item, uint lockHandle);

    const MHTItem &get(mht_key_t mht_key, bool locking = false);
    const MHTItem &localGet(mht_key_t key, bool locking);
    void remoteGet(mht_key_t key, bool locking);

    /**
     * Locks the item with the given key, if existent
     *
     * @param mht_key   The key of the item that should be locked
     * @return  the lockHandle locking succeeds, 0 if the key is not found
     */
    uint lock(mht_key_t mht_key);
    uint lockLocal(mht_key_t mht_key);

    bool unlock(mht_key_t mht_key, uint lockHandle);
    inline bool unlockLocal(mht_key_t mht_key, uint lockHandle) {
        MHTPartition *part = findPartition(mht_key);
        assert(part);
        return part->unlock(mht_key, lockHandle);
    }

    /**
     * Reserve the slot for <mht_key>
     * @param mht_key   The key to be reserved
     * @return  0 if reservation fails, otherwise the identifier of the reservation
     */
    uint reserve(mht_key_t mht_key);

    /**
     * Release a reservation for <mht_key> given you specify the correct reservation number.
     * Releasing a non-existent key does not return an error.
     *
     * @param mht_key       The key to release
     * @param reservation   The identifier of the reservation you want to reverse
     * @return m3::NO_PERM if incorrect reservation identifier specified, m3::NO_ERROR on success
     */
    m3::Errors::Code release(mht_key_t mht_key, uint reservation);

    /**
     * Migrates partitions which are owned by the local kernel to another kernel.
     *
     * @param pes       Array of PEs for which the partitions should be migrated
     * @param numPEs    Number of PEs to be migrated
     * @param receiver  The target kernel
     */
    void migratePartitions(m3::PEDesc pes[], uint numPEs, membership_entry::krnl_id_t receiver);

    void receivePartitions(GateIStream & is);

    void finishMigration(membership_entry::krnl_id_t krnlId);

    /**
     * Set the given kernel as responsible for a number of PEs (\p capacity) starting at
     * \p start. \p update decides whether the (local) hardware information is updated accordingly.
     *
     * @param start         The PE ID to start from
     * @param krnl          The new owner's ID
     * @param krnlCore      The new owner's core
     * @param capacity      Number of PEs affected
     * @param flags         Flags of type ::MembershipFlags
     * @param propagate     Indicates whether this update should be propagated to other kernels
     */
    void updateMembership(membership_entry::pe_id_t start, membership_entry::krnl_id_t krnl,
        membership_entry::pe_id_t krnlCore, membership_entry::capacity_t capacity,
        MembershipFlags flags, bool propagate);

    /**
     * Set the given kernel as responsible for the PEs given in \p releasedPEs.
     * \p update decides whether the (local) hardware information is updated accordingly.
     *
     * @param releasedPEs   Array of PEs to that are affected of the change
     * @param numPEs        Number of affected PEs
     * @param krnl          The new owner's ID
     * @param krnlCore      The new owner's core
     * @param flags         Flags of type ::MembershipFlags
     * @param propagate     Indicates whether this update should be propagated to other kernels
     */
    void updateMembership(m3::PEDesc releasedPEs[], uint numPEs, membership_entry::krnl_id_t krnl,
    membership_entry::pe_id_t krnlCore, MembershipFlags flags, bool propagate);

    bool keyLocality(mht_key_t key);

    membership_entry::krnl_id_t inline responsibleKrnl(membership_entry::pe_id_t peid) {
        return memberTable[peid].krnl_id;
    }
    membership_entry::krnl_id_t inline responsibleMember(mht_key_t key) {
        // membership table lookup
        return responsibleKrnl(HashUtil::hashToPeId(key));
    }

    uint localPEs() const;

    MHTPartition* findPartition(mht_key_t key);

    KPE* getMigrationDestination(membership_entry::pe_id_t partID) {
        for(auto it = _migratingPartitions.begin(); it != _migratingPartitions.end(); it++) {
            if(it->partitionID == partID)
                return &(it->destKrnl);
        }
        return nullptr;
    }

    /* Debugging functions */
    void printMembership();
    void printContents();

private:
    explicit MHTInstance();
    explicit MHTInstance(uint64_t memberTab, uint64_t parts, size_t partsSize);

    // list of MHTPartitions
    m3::SList<PartitionEntry> partitions;
    // membership table
    membership_entry *memberTable;
    m3::SList<MigratingPartitionEntry> _migratingPartitions;
    static MHTInstance *_inst;

    // TODO
    // dirty workaround, the interface needs to change
    struct nextResItem {
        nextResItem() : idx(0) {};
        size_t getnext() {
            if(idx >= 3)
                idx = 0;
            return idx++;
        }
        size_t idx;
    };
    static nextResItem nextIdx;
    static MHTItem resItems[3];
};
}
