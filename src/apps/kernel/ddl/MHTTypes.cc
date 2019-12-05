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

#include <base/Heap.h>
#include <base/log/Kernel.h>
#include <thread/ThreadManager.h>
#include <base/util/Util.h>

#include "ddl/MHTTypes.h"
#include "ddl/MHTInstance.h"
#include "pes/VPE.h"

//#define DDL_PRINT_ITEMS

namespace kernel {

MHTItem::MHTItem(MHTItem &&m) : data(m.data), _mht_key(m._mht_key), length(m.length),
        lockHandle(m.lockHandle), reservation(m.reservation), _tickets(m3::Util::move(m._tickets)) {
    m.data = nullptr;
    m._mht_key = 0;
    m.length = 0;
    m.lockHandle = 0;
}

MHTItem &MHTItem::operator=(MHTItem &&m) {
    data = m.data;
    _mht_key = m._mht_key;
    length = m.length;
    lockHandle = m.lockHandle;
    reservation = m.reservation;
    _tickets = m3::Util::move(m._tickets);
    m.data = nullptr;
    m._mht_key = 0;
    m.length = 0;
    m.lockHandle = 0;
    return *this;
}

void MHTItem::transferData(MHTItem &src) {
    if(src.data && !m3::Heap::is_on_heap(src.data))
        PANIC("Data of DDL item is not on the heap!");
    if(data)
        m3::Heap::free(data);
    data = src.data;
    src.data = nullptr;
}

size_t MHTItem::serializedSize() const {
    size_t size = m3::ostreamsize<mht_key_t, uint>();
    switch(HashUtil::hashToType(_mht_key)) {
        case MSGCAP:
            return size + static_cast<MsgCapability*>(data)->serializedSize();
        case SRVCAP:
            return size + static_cast<ServiceCapability*>(data)->serializedSize();
        case SESSCAP:
            return size + static_cast<SessionCapability*>(data)->serializedSize();
        case VPECAP:
            return size + static_cast<VPECapability*>(data)->serializedSize();
        case MEMCAP:
            return size + static_cast<MemCapability*>(data)->serializedSize();
        case NOTYPE:
        default:
            return size + m3::vostreamsize(m3::ostreamsize<size_t>() + length);
    }
}

void MHTItem::serialize(GateOStream &ser) const {
    ser << _mht_key << length;
    // TODO
    // cases SERVICE, ACTIVE
    // TODO
    // do we need to serialize the capID too? actually its stored in the mht_key of the mhtitem
    switch(HashUtil::hashToType(_mht_key)) {
        case MSGCAP:
            static_cast<MsgCapability*>(data)->serialize(ser);
            return;
        case SRVCAP:
            static_cast<ServiceCapability*>(data)->serialize(ser);
            return;
        case SESSCAP:
            static_cast<SessionCapability*>(data)->serialize(ser);
            return;
        case VPECAP:
            static_cast<VPECapability*>(data)->serialize(ser);
            return;
        case MEMCAP:
            static_cast<MemCapability*>(data)->serialize(ser);
            return;
        case SERVICE:
        case MSGOBJ:
            PANIC("Not implemented");
        case MEMOBJ:
            PANIC("Not implemented");
        case NOTYPE:
        default:
        {
            ser.put_str(static_cast<const char*>(data), length);
            return;
        }
    }
}

template<class T>
void MHTItem::deserialize(T &is) {
    is >> _mht_key >> length;
    // TODO
    // cases SERVICE, ACTIVE
    switch(HashUtil::hashToType(_mht_key)) {
        case MSGCAP:
            data = new MsgCapability(is, _mht_key);
            return;
        case SRVCAP:
            data = new ServiceCapability(is, _mht_key);
            return;
        case SESSCAP:
            data = new SessionCapability(is, _mht_key);
            return;
        case VPECAP:
            data = new VPECapability(is, _mht_key);
            return;
        case MEMCAP:
            data = new MemCapability(is, _mht_key);
            return;
        case SERVICE:
        case MSGOBJ:
            PANIC("Not implemented");
        case MEMOBJ:
            PANIC("Not implemented");
        case NOTYPE:
        default:
            PANIC("Deserializing item of type notype not implemented");
    }
}
template void MHTItem::deserialize<m3::Unmarshaller>(m3::Unmarshaller &is);
template void MHTItem::deserialize<GateIStream>(GateIStream &is);

void MHTItem::enqueueTicket() {
    int tid = m3::ThreadManager::get().current()->id();
    _tickets.append(new DDLTicket(tid));
    m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(tid));
}

size_t MHTItem::defaultSize(ItemType type) {
    switch(type) {
        case MSGCAP:
            return sizeof(MsgCapability);
        case SRVCAP:
            // includes service name, hence no static size;
            PANIC("No default size for service capability");
        case SESSCAP:
            // includes service name, hence no static size;
            PANIC("No default size for session capability");
        case VPECAP:
            return sizeof(VPECapability);
        case MEMCAP:
            return sizeof(MemCapability);
        case SERVICE:
            PANIC("No default size for services");
        case MSGOBJ:
            PANIC("Default size of MSGOBJ not defined");
        case MEMOBJ:
            PANIC("Default size of MEMOBJ not defined");
        case NOTYPE:
        default:
            PANIC("Untyped items dont have a default size");
    }
    UNREACHED;
};

void MHTItem::printState() const {
    KLOG(MHT, "---- Printing MHTItem. " << " mhtKey: " << PRINT_HASH(_mht_key) <<
            " len: " << length << " data ptr: " << m3::fmt(data, "0x#"));
    #ifdef DDL_PRINT_ITEMS
    if(m3::KernelLog::level & m3::KernelLog::MHT) {
        LOCK();
        switch(HashUtil::hashToType(_mht_key)){
            case MSGCAP:
                (static_cast<MsgCapability*>(data))->print(m3::Serial::get());
                break;
            case SRVCAP:
                (static_cast<ServiceCapability*>(data))->print(m3::Serial::get());
                break;
            case SESSCAP:
                (static_cast<SessionCapability*>(data))->print(m3::Serial::get());
                break;
            case VPECAP:
                (static_cast<VPECapability*>(data))->print(m3::Serial::get());
                break;
            case MEMCAP:
                (static_cast<MemCapability*>(data))->print(m3::Serial::get());
                break;
            case NOTYPE:
            case MSGOBJ:
            case MEMOBJ:
            case SERVICE:
                break;
        }
        m3::Serial::get() << '\n';
        UNLOCK();
    }
    #endif
}

}
