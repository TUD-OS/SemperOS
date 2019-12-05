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

#include "MHTTypes.h"

namespace kernel {

struct MHTItem;
struct MHTItemStorable;
class MHTInstance;

class MHTPartition {
    friend MHTInstance;
    friend KPE;
public:
    MHTPartition(membership_entry::pe_id_t id) : _id(id), _storage() {}
    MHTPartition(const MHTPartition &) = delete;
    MHTPartition &operator=(const MHTPartition &) = delete;
    MHTPartition(MHTPartition &&) = delete;
    ~MHTPartition();

    //debug
    void printItems();

private:
    /**
     * Tries to insert or replace the kv_pair. Replacing works only if the item
     * to be replaced is not locked or the correct lockHandle is passed.
     * @param kv_pair       Item to be inserted
     * @param lockHandle    if != 0 this is used to unlock the slot where the item is inserted
     * @return
     */
    m3::Errors::Code put(MHTItem &&kv_pair, uint lockHandle = 0);

    /**
     * Get the item with the given key.
     * @param mht_key
     * @param key       Unhashed key to resolve collisions -- currently not supported
     * @param locking   defaults to false. If set, the get operation locks the key
     * @return
     */
    const MHTItem &get(mht_key_t mht_key, bool locking = false);

    /**
     * Removes a key from the storage.
     * @param mht_key  MHTItem containing the key and _mht_key of the pair to be deleted
     * @return true if item found and removed
     */
    bool remove(mht_key_t mht_key);

    /**
     * Locks the item with the specified key. If the item is already locked the requested
     * item is marked that there are pending requests.
     * @param mht_key   Hashed key of the item to be locked
     * @return  The lockHandle if locking succeeded, otherwise 0
     */
    int lock(mht_key_t mht_key);

    /**
     * Unlocks the item with the specified key.
     * @param mht_key       Key of the item to be unlocked
     * @param lockHandle    Lockhandle which was handed out when the item got locked
     * @return  true, if unlocking succeeded (includes key not found - case); false if the lockHandle was not correct
     */
    bool unlock(mht_key_t mht_key, uint lockHandle);

    /**
     * Reserves the slot for the given key. This succeeds only if there is no item present with this key.
     *
     * @param mht_key   The key to be reserved
     * @return  0 if the reservation failed, otherwise the reservation number that is necessary to use the reservation
     */
    uint reserve(mht_key_t mht_key);

    /**
     * Release a reservation for <mht_key> given you specify the correct reservation number
     *
     * @param mht_key       The key to release
     * @param reservation   The identifier of the reservation you want to reverse
     * @return m3::NO_PERM if incorrect reservation identifier specified, m3::NO_ERROR on success
     */
    m3::Errors::Code release(mht_key_t mht_key, uint reservation);

    /**
     * Creates a ticket for the currently running thread in the item belonging to the <mht_key>.
     * This ticket is enqueued and the thread waits for notification afterwards.
     *
     * @param mht_key   Key of the item to create a ticket for
     */
    void enqueueTicket(mht_key_t mht_key);

    /**
     * Calculate the amount of memory necessary to serialize this partition.
     * @return  Bytes necessary to serialize this partition.
     */
    size_t serializedSize();

    /**
     * Packs each item of this partition in the given stream.
     * The stream object has to have the correct size, which can be obtained with <serializedSize()>
     * @param ser   Stream the content is written to
     */
    void serialize(GateOStream &ser);

    /**
     * Reads content from a stream to fill this partition.
     *
     * We need this templated to enabel an Unmarshaller and a GateIStream implementation,
     * because GateIStream does not inherit from Unmarshaller due to a compiler bug.
     * The GateIStream implementation is used for migrating partitions during runtime,
     * the Unmarshaller implementation is used to read the partition from memory at kernel
     * startup, where we do not have a receive Gate to construct the GateIStream from,
     * but read the data directly from memory.
     *
     * @param ser   Stream to read from
     * @return      m3::Errors::OUT_OF_MEM if allocating memory for items fails.
     */
    template<class T>
    m3::Errors::Code deserialize(T &ser);

    membership_entry::pe_id_t _id;
    m3::SList<MHTItemStorable> _storage;
    static const MHTItem emptyIndicator;
};
}
