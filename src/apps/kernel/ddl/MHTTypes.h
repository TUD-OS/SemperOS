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

#include <base/Compiler.h>
#include <base/util/String.h>
#include <base/Heap.h>
#include <base/util/Random.h>
#include <base/util/Reference.h>
#include <base/log/Kernel.h>
#include <base/Panic.h>
#include <base/col/SList.h>
#include <thread/ThreadManager.h>
#include <assert.h>

#include "Gate.h"
#include "Platform.h"

// Bits for the ID space
#define ID_BITS         64
// All bits set
#define KEY_MASK        0xFFFFFFFFFFFFFFFF
// Maximum number of PEs representable by the PE Bits
#define MAX_PES_DDL     kernel::Platform::MAX_PES
// Number of bits representing the PE ID
#define PE_BITS         m3::nextlog2<MAX_PES_DDL>::val
// Number of bits representing the VPE ID
#define VPE_BITS        11
// Number of bis representing different types of data stored in the MHT
#define TYPE_BITS       7
// Number of bits representing hashed key
#define HASH_BITS       (ID_BITS - PE_BITS - VPE_BITS - TYPE_BITS)
#define TYPE_MASK       (((0x1ULL << TYPE_BITS) - 1) << HASH_BITS)
// Number of bits used in TYPE to specify map capability types
#define MCAP_BITS        1
#define TYPE_MASK_MCAP   (((0x1ULL << MCAP_BITS) - 1) << HASH_BITS)
// Number of bits used in TYPE to specify object capability types
#define OCAP_BITS        3
#define TYPE_MASK_OCAP   (((0x1ULL << OCAP_BITS) - 1) << (HASH_BITS + MCAP_BITS))
#define TYPE_MASK_CAP   (((0x1ULL << (OCAP_BITS + MCAP_BITS)) - 1) << HASH_BITS)

#define PRINT_HASH(key) \
        "(" << HashUtil::hashToPeId(key) << " : " << \
        HashUtil::hashToVpeId(key) << " : " << HashUtil::hashToType(key) << \
        " : " << HashUtil::hashToObjId(key) << ")"

namespace kernel {
// Type of the ID space
using mht_key_t = uint64_t;
using vpe_id_t = uint16_t;

/**
 * 7 bits represent the type
 *  0 - 1 - - - 3 - 4 - - - 6
 * |MCap| OCaps |Srv|  KOs  |
*/
enum ItemType : uint8_t {
    NOTYPE = 0,
    MAPCAP = 1,
    MSGCAP = 2,
    SRVCAP = 4,
    SESSCAP = 6,
    VPECAP = 8,
    MEMCAP = 10,
    GENERICOCAP = 14,
    SERVICE = 16,
    MSGOBJ = 32,
    MEMOBJ = 64,
};

enum MembershipFlags : uint8_t {
    NONE = 0,
    MIGRATING = 1,
    UNMANAGED = 2,      // unused
    NOCHANGE = 4,       // when used while implicitly updating membership table, the entries keep their flags
    UNPOPULATED = 0xFF  // No PE at this ID
};

static_assert(HASH_BITS > 0, "DDL ID space too small");

class MHTPartition;
class MHTInstance;
class KPE;
class KernelcallHandler;
class Capability;
class MsgCapability;
class VPECapability;
class ServiceCapability;
class SessionCapability;
class MemCapability;
class Service;

// Membership table entry
struct membership_entry {
    using pe_id_t = uint16_t;
    using krnl_id_t = uint16_t;
    using capacity_t = uint16_t;
    pe_id_t pe_id;
    krnl_id_t krnl_id;
    capacity_t capacity;
    MembershipFlags flags;
} PACKED;

struct DDLCapRngDesc {
    DDLCapRngDesc(mht_key_t _start, uint _count) : start(_start), count(_count) {}

    mht_key_t start;
    uint count;
};

/**
 * Provides methods to create and work with hashes.
 * The ID space is structured as follows:
 * ____________________________________
 * | PE ID | VPE ID | TYPE | OBJECT ID |
 * ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
 *
 */
class HashUtil {
public:
    /**
     * Create the hashed key from the VPE Id, the type and the key for the actual data item.
     * The PE ID is deduced from the VPE ID
     *
     * @param vpe_id    VPE Id to which the key belongs
     * @param type      ItemType of the item
     * @param key       The object identifier
     * @return          The combined key for the structured key space of the DHT
     */
    static inline mht_key_t structured_hash(vpe_id_t vpe_id, ItemType type, uint key) {
        // TODO: doesn't express what we want to have
        mht_key_t pe_id = vpe_id % MAX_PES_DDL;
        return structured_hash(pe_id, vpe_id, type, key);
    }

    /**
     * Use this to create a key if the normal VPE_ID to PE_ID relation does not hold,
     * which is pe_id = (vpe_id / VPE_ID_INTERVAL) % MAX_PES;
     *
     * @param pe_id
     * @param vpe_id
     * @param type
     * @param key
     * @return
     */
    static inline mht_key_t structured_hash(membership_entry::pe_id_t pe_id, vpe_id_t vpe_id, ItemType type, uint key) {
        return static_cast<mht_key_t>(
                (((mht_key_t)pe_id) << (ID_BITS - PE_BITS)) |
                (((mht_key_t)vpe_id) << (ID_BITS - PE_BITS - VPE_BITS)) |
                (((mht_key_t)type) << HASH_BITS) |
                (((mht_key_t)key) & (KEY_MASK >> (ID_BITS - HASH_BITS))) );
    }

    /**
     * Helps to quickly find the partition ID of a key.
     *
     * @param mht_key   The hashed key of the entry
     * @return  PE Id (equals partition ID of hash table) corresponding to the given key
     */
    static inline membership_entry::pe_id_t hashToPeId(mht_key_t mht_key) {
        return static_cast<membership_entry::pe_id_t>(mht_key >> (ID_BITS - PE_BITS));
    }

    /**
     * Extract the VPE ID from the hash.
     *
     * @param mht_key   The hashed key of the entry
     * @return  Vpe Id
     */
    static inline vpe_id_t hashToVpeId(mht_key_t mht_key) {
        return static_cast<vpe_id_t>( (mht_key >> (ID_BITS - PE_BITS - VPE_BITS)) &
                ( (static_cast<vpe_id_t>(1) << VPE_BITS) - 1) );
    }

    /**
     * Identify the type of an Item
     *
     * @param mht_key   The key of the Item
     * @return  The ItemType corresponding to the key
     */
    static inline ItemType hashToType(mht_key_t mht_key) {
        return static_cast<ItemType>((mht_key & TYPE_MASK) >> HASH_BITS);
    }

    /**
     * Extract the object identifier from the hash
     *
     * @param mht_key   The key of the Item
     * @return  The object ID
     */
    static inline uint hashToObjId(mht_key_t mht_key) {
        return static_cast<uint>(mht_key & (~(KEY_MASK << HASH_BITS)));
    }
};

struct DDLTicket : public m3::SListItem {
    explicit DDLTicket(int threadID) : tid(threadID) {};
    int tid;
};

struct MHTItem {
    friend MHTPartition;
    friend MHTInstance;
    friend KPE;
    friend KernelcallHandler;

    /**
     * Empty item containing just a lockHandle for the lock operation
     * @param lockHndl
     */
    MHTItem(uint lockHndl) : data(nullptr), _mht_key(0), length(0),
        lockHandle(lockHndl), reservation(false), _tickets() { }

    MHTItem(void *Data, uint len, mht_key_t mht_key) : data(Data), _mht_key(mht_key),
        length(len), lockHandle(0), reservation(false), _tickets() {
        if(Data && !m3::Heap::is_on_heap(Data))
            PANIC("Data of DDL item is not on the heap!");
    }
    MHTItem(membership_entry::pe_id_t pe_id, vpe_id_t vpe_id, ItemType type, uint Key, void *Data, uint len)
        : MHTItem(Data, len, HashUtil::structured_hash(pe_id, vpe_id, type, Key)) {}
    MHTItem(vpe_id_t vpe_id, ItemType type, uint Key, void *Data, uint len)
        : MHTItem(Data, len, HashUtil::structured_hash(vpe_id, type, Key)) {}

    template<class T>
    MHTItem(T &is) : data(nullptr), _mht_key(0),
        length(0), lockHandle(0), reservation(false), _tickets() {
        deserialize(is);
    }

    MHTItem(const MHTItem &m) = delete;
    MHTItem(MHTItem &&m);

    MHTItem operator=(MHTItem &m) = delete;
    MHTItem &operator=(MHTItem &&m);

    ~MHTItem() {
        for(auto it = _tickets.begin(); it != _tickets.end();) {
            auto old = it++;
            delete &(*old);
        }
    }

    bool isEmpty() const {
        return !(_mht_key | length);
    }
    bool validData() const {
        return (data != nullptr);
    }
    mht_key_t getKey() const {
        return _mht_key;
    }
    uint getLength() const {
        return length;
    }
    uint getLockHandle() const {
        return lockHandle;
    }
    bool islocked() const {
        return (lockHandle);
    }

    template<class T>
    inline bool checkType() const {
        return false;
    }

    template<class T>
    T *getData() const {
        if(!checkType<T>()) {
            KLOG(ERR, "Warning: Type (" << HashUtil::hashToType(_mht_key) << ") "
                    "of MHTItem did not match, returning nullptr");
            return nullptr;
        }
        return static_cast<T*>(data);
    }

    /**
     * Transfer the data from <src> to this item.
     *
     * The data ptr of <src> is reset to transfer data ownership to this item.
     *
     * @param src   The item of which the data will be taken from.
     */
    void transferData(MHTItem &src);

    uint lock() {
        if(lockHandle) {
            PANIC("Double locking MHTItem! mhtKey: " << PRINT_HASH(_mht_key) << ")");
        }
        // generate a random lockHandle here, to prevent unauthorized unlocking
        do {
            lockHandle = m3::Random::get();
        } while(!lockHandle);
        return lockHandle;
    }

    bool unlock(uint lockHdl) {
        // TODO
        // notify the threads waiting for this item to be unlocked
        if(!lockHandle) // item was not locked
            return true;
        if(lockHdl != lockHandle) {
            KLOG(ERR, "Unlocking MHTItem failed! mhtKey: " << PRINT_HASH(_mht_key) << ")");
            return false;
        }
        lockHandle = 0;
        for(auto it = _tickets.begin(); it != _tickets.end(); ){
            auto old = it++;
            m3::ThreadManager::get().notify(reinterpret_cast<void*>(old->tid), nullptr, 0);
            delete &(*old);
        }
        return true;
    }

    static size_t defaultSize(ItemType type);
    size_t serializedSize() const;

    /**
     * Serialize the data item depending on its type.
     *
     * @param ser   The GateOStream to serialize the item into
     */
    void serialize(GateOStream &ser) const;

    template<class T>
    void deserialize(T &is);

    /**
     * Creates a ticket for the currently running thread and lets the thread wait for this event.
     */
    void enqueueTicket();

    // debug
    void printState() const;

private:
    /**
     * Private default constructor creates an empty item.
     */
    MHTItem() {
        MHTItem(0);
    };

    void *data;
    mht_key_t _mht_key;
    uint length;
    uint lockHandle;
    bool reservation; // >true if the item is a placeholder for a reservation
    m3::SList<DDLTicket> _tickets;
};

template <>
inline bool MHTItem::checkType<MsgCapability>() const {
    return (HashUtil::hashToType(_mht_key) == MSGCAP || HashUtil::hashToType(_mht_key) == MEMCAP);
}
template <>
inline bool MHTItem::checkType<ServiceCapability>() const {
    return HashUtil::hashToType(_mht_key) == SRVCAP;
}
template <>
inline bool MHTItem::checkType<SessionCapability>() const {
    return HashUtil::hashToType(_mht_key) == SESSCAP;
}
template <>
inline bool MHTItem::checkType<VPECapability>() const {
    return HashUtil::hashToType(_mht_key) == VPECAP;
}
template <>
inline bool MHTItem::checkType<MemCapability>() const {
    return (HashUtil::hashToType(_mht_key) == MEMCAP || HashUtil::hashToType(_mht_key) == MSGCAP);
}
template <>
inline bool MHTItem::checkType<Capability>() const {
    ItemType type = HashUtil::hashToType(_mht_key);
    return  (type == GENERICOCAP || type == MSGCAP || type == SRVCAP || type == SESSCAP ||
                type == VPECAP || type == MEMCAP);
}
template <>
inline bool MHTItem::checkType<Service>() const {
    ItemType type = HashUtil::hashToType(_mht_key);
    return  (type == SERVICE);
}

}
