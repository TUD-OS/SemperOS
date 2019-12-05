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

#include <base/log/Kernel.h>

#include "pes/PEManager.h"
#include "cap/Capability.h"
#include "cap/CapTable.h"

namespace kernel {

m3::OStream &operator<<(m3::OStream &os, const Capability &cc) {
    cc.print(os);
    return os;
}

void Capability::addChild(mht_key_t child) {
    // sort list w.r.t locality
    // local items are put first
    if (MHTInstance::getInstance().keyLocality(child) || _children.length() == 0) {
        _children.insert(nullptr, new Child(child));
        return;
    }

    // sort other kernels' capabilities
    membership_entry::pe_id_t peID = HashUtil::hashToPeId(child);
    Child *prev = &*_children.begin();
    membership_entry::krnl_id_t current_respKrnl = 0;
    membership_entry::krnl_id_t respKrnl = 0;
    respKrnl = MHTInstance::getInstance().responsibleKrnl(peID);
    if(respKrnl <= MHTInstance::getInstance().responsibleMember(prev->id)) {
        _children.insert(nullptr, new Child(child));
        return;
    }
    for(auto &it : _children) {
        current_respKrnl = MHTInstance::getInstance().responsibleMember(it.id);
        if(respKrnl >= current_respKrnl) {
            _children.insert(prev, new Child(child));
            return;
        }
        prev = &it;
    }
}

void Capability::removeChild(mht_key_t child) {
    for(auto it = _children.begin(); it != _children.end(); it++) {
        if(it->id == child) {
            _children.remove(&*it);
            delete &*it;
            return;
        }
    }
}

void Capability::removeChildAllTypes(mht_key_t child) {
    for(auto it = _children.begin(); it != _children.end(); it++) {
        if((it->id & ~TYPE_MASK_CAP) == (child & ~TYPE_MASK_CAP)) {
            _children.remove(&*it);
            delete &*it;
            return;
        }
    }
}

size_t Capability::serializedSizeTyped(Capability *cap) {
    switch (HashUtil::hashToType(cap->_id)) {
        case ItemType::MSGCAP:
            return reinterpret_cast<MsgCapability*>(cap)->serializedSize();
        case ItemType::SRVCAP:
            return reinterpret_cast<ServiceCapability*>(cap)->serializedSize();
        case ItemType::SESSCAP:
            return reinterpret_cast<SessionCapability*>(cap)->serializedSize();
        case ItemType::VPECAP:
            return reinterpret_cast<VPECapability*>(cap)->serializedSize();
        case ItemType::MEMCAP:
            return reinterpret_cast<MemCapability*>(cap)->serializedSize();
        case ItemType::SERVICE:
        case ItemType::MSGOBJ:
        case ItemType::MEMOBJ:
        case ItemType::NOTYPE:
        default:
            PANIC("Serialized size not specified");
    }
}

void Capability::serializeTyped(GateOStream& ser, Capability* cap) {
    switch (HashUtil::hashToType(cap->_id)) {
        case ItemType::MSGCAP:
            reinterpret_cast<MsgCapability*>(cap)->serialize(ser);
            break;
        case ItemType::SRVCAP:
            reinterpret_cast<ServiceCapability*>(cap)->serialize(ser);
            break;
        case ItemType::SESSCAP:
            reinterpret_cast<SessionCapability*>(cap)->serialize(ser);
            break;
        case ItemType::VPECAP:
            reinterpret_cast<VPECapability*>(cap)->serialize(ser);
            break;
        case ItemType::MEMCAP:
            reinterpret_cast<MemCapability*>(cap)->serialize(ser);
            break;
        case ItemType::SERVICE:
        case ItemType::MSGOBJ:
        case ItemType::MEMOBJ:
        case ItemType::NOTYPE:
        default:
            PANIC("Serialization not implemented");
    }
}

void MemObject::revokeAction() {
    // if it's not derived, it's always memory from mem-PEs
    if(!derived) {
        uintptr_t addr = label & ~m3::KIF::Perm::RWX;
        MainMemory::get().free(core, addr, credits);
    }
}

void SessionObject::close() {
    // only send the close message, if the service has not exited yet
    if(srv->vpe().state() == VPE::RUNNING) {
        AutoGateOStream msg(m3::ostreamsize<m3::KIF::Service::Command, word_t>());
        msg << m3::KIF::Service::CLOSE << ident;
        KLOG(SERV, "Sending CLOSE message for ident " << m3::fmt(ident, "#x", 8)
            << " to " << srv->name());
        ServiceList::get().send_and_receive(srv, msg.bytes(), msg.total(), msg.is_on_heap());
        msg.claim();
    }
}
SessionObject::~SessionObject() {
    if(!servowned)
        close();
}

m3::Errors::Code MsgCapability::revoke() {
    if(localepid != -1) {
        VPE &vpe = PEManager::get().vpe(table()->id());
        vpe.xchg_ep(localepid, nullptr, nullptr);
        // wakeup the core to give him the chance to notice that the endpoint was invalidated
        // TODO
        // we don't want to use this wake up since it reset the PC at the PE
//        if(vpe.state() != VPE::DEAD)
//            DTU::get().wakeup(vpe.desc());
    }
    // only the KO owner takes action
    if(MHTInstance::getInstance().keyLocality(obj->id) && obj->refcount() == 1)
        obj->revokeAction();
    obj.unref();
    return m3::Errors::NO_ERROR;
}

MapCapability::MapCapability(CapTable *tbl, capsel_t sel, uintptr_t _phys, uint _attr, mht_key_t capid)
    : Capability(tbl, sel, MAP, capid), phys(_phys), attr(_attr) {
    VPE &vpe = PEManager::get().vpe(tbl->id());
    DTU::get().map_page(vpe.desc(), sel << PAGE_BITS, phys, attr);
}

void MapCapability::remap(uint _attr) {
    attr = _attr;
    VPE &vpe = PEManager::get().vpe(table()->id());
    DTU::get().map_page(vpe.desc(), sel() << PAGE_BITS, phys, attr);
}

m3::Errors::Code MapCapability::revoke() {
    VPE &vpe = PEManager::get().vpe(table()->id());
    DTU::get().unmap_page(vpe.desc(), sel() << PAGE_BITS);
    return m3::Errors::NO_ERROR;
}

m3::Errors::Code SessionCapability::revoke() {
    // if the server created that, we want to close it as soon as there are no clients using it anymore
    if(obj->servowned && obj->refcount() == 2)
        obj->close();
    obj.unref();
    return m3::Errors::NO_ERROR;
}

m3::Errors::Code ServiceCapability::revoke() {
    bool closing = inst->closing;
    inst->closing = true;
    // if we have childs, i.e. sessions, we need to send the close-message to the service first,
    // in which case there will be pending requests, which need to be handled first.
    if(inst->pending() > 0 || (children().length() > 0 && !closing))
        return m3::Errors::MSGS_WAITING;
    return m3::Errors::NO_ERROR;
}

VPECapability::VPECapability(CapTable *tbl, capsel_t sel, VPE *p, mht_key_t capid)
    : Capability(tbl, sel, VIRTPE, capid), vpe(p) {
    p->ref();
    vpeId = p->mht_key(VPECAP, sel);
}

VPECapability::VPECapability(const VPECapability &t) : Capability(t), vpe(t.vpe) {
    vpe->ref();
}

m3::Errors::Code VPECapability::revoke() {
    vpe->unref();
    // TODO reset core and release it (make it free to use for others)
    return m3::Errors::NO_ERROR;
}

void MsgCapability::print(m3::OStream &os) const {
    os << m3::fmt(table()->id(), 2) << " @ " << m3::fmt(sel(), 6);
    os << ": mesg[refs=" << obj->refcount()
       << ", curep=" << localepid
       << ", dst=" << obj->core << ":" << obj->epid
       << ", lbl=" << m3::fmt(obj->label, "#0x", sizeof(label_t) * 2)
       << ", crd=#" << m3::fmt(obj->credits, "x") << "], parent=["
       << PRINT_HASH(parent()) << "], chld=[ ";
    for(auto it : children())
        os << PRINT_HASH(it.id) << " ";
    os << "]";
}

void MemCapability::print(m3::OStream &os) const {
    os << m3::fmt(table()->id(), 2) << " @ " << m3::fmt(sel(), 6);
    os << ": mem [refs=" << obj->refcount()
       << ", curep=" << localepid
       << ", dst=" << obj->core << ":" << obj->epid << ", lbl=" << m3::fmt(obj->label, "#x")
       << ", crd=#" << m3::fmt(obj->credits, "x") << "], parent=["
       << PRINT_HASH(parent()) << "], chld=[ ";
    for(auto it : children())
        os << PRINT_HASH(it.id) << " ";
    os << "]";
}

void MapCapability::print(m3::OStream &os) const {
    os << m3::fmt(table()->id(), 2) << " @ " << m3::fmt(sel(), 6);
    os << ": map [virt=#" << m3::fmt(sel() << PAGE_BITS, "x")
       << ", phys=#" << m3::fmt(phys, "x")
       << ", attr=#" << m3::fmt(attr, "x") << "], parent=["
       << PRINT_HASH(parent()) << "], chld=[ ";
    for(auto it : children())
        os << PRINT_HASH(it.id) << " ";
    os << "]";
}

void ServiceCapability::print(m3::OStream &os) const {
    os << m3::fmt(table()->id(), 2) << " @ " << m3::fmt(sel(), 6);
    os << ": serv[name=" << inst->name() << "], parent=["
       << PRINT_HASH(parent()) << "], chld=[ ";
    for(auto it : children())
        os << PRINT_HASH(it.id) << " ";
    os << "]";
}

void SessionCapability::print(m3::OStream &os) const {
    os << m3::fmt(table()->id(), 2) << " @ " << m3::fmt(sel(), 6);
    os << ": sess[refs=" << obj->refcount()
        << ", serv=" << obj->srv->name()
        << ", ident=#" << m3::fmt(obj->ident, "x")
        << ", servowned=" << obj->servowned << "], parent=["
        << PRINT_HASH(parent()) << "], chld=[ ";
    for(auto it : children())
        os << PRINT_HASH(it.id) << " ";
    os << "]";
}

void VPECapability::print(m3::OStream &os) const {
    os << m3::fmt(table()->id(), 2) << " @ " << m3::fmt(sel(), 6);
    os << ": vpe [refs=" << vpe->refcount()
       << ", name=" << vpe->name() << "], vpeId=" << m3::fmt(vpeId, "0x#");
    os << ", parent=["
       << PRINT_HASH(parent()) << "], chld=[ ";
    for(auto it : children())
        os << PRINT_HASH(it.id) << " ";
    os << "]";
}

void Capability::printChilds(m3::OStream &os, int layer) const {
    os << "\n";
    os << m3::fmt("", layer * 2) << " \\-";
    print(os);
    for(auto it : _children)
        os << PRINT_HASH(it.id);
}

}
