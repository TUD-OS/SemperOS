/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
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
#include <base/col/Treap.h>
#include <base/DTU.h>
#include <base/KIF.h>
#include <base/util/Reference.h>
#include <base/col/SList.h>

#include "com/Services.h"
#include "ddl/MHTTypes.h"
#include "mem/SlabCache.h"

namespace m3 {
class OStream;
}

namespace kernel {

class CapTable;
class Capability;

m3::OStream &operator<<(m3::OStream &os, const Capability &cc);

class Capability : public m3::TreapNode<capsel_t>, public m3::RefCounted {
    friend class CapTable;
    friend class MHTItem;

public:
    typedef capsel_t key_t;

    enum {
        SERVICE = 0x01,
        SESSION = 0x02,
        MSG     = 0x04,
        MEM     = 0x08,
        MAP     = 0x10,
        VIRTPE  = 0x20,
    };

    struct Child : public SlabObject<Child>, public m3::SListItem {
        explicit Child(mht_key_t _id) : id(_id) {
        }
        mht_key_t id;
    };

    explicit Capability(CapTable *tbl, capsel_t sel, unsigned type, mht_key_t capid)
        : TreapNode<capsel_t>(sel), _type(type), _id(capid), _tbl(tbl), _parent(), _children() {
    }
    explicit Capability(unsigned type, mht_key_t capid)
        : TreapNode<capsel_t>(HashUtil::hashToObjId(capid)), _type(type), _id(capid) {
    }
    Capability(const Capability &rhs)
        : TreapNode<capsel_t>(rhs.key()), _type(rhs._type), _id(rhs._id), _tbl(rhs._tbl),
        _parent(rhs._parent), _children() {
    }
    virtual ~Capability() {
        for(auto it = _children.begin(); it != _children.end(); ) {
            auto old = it++;
            delete &*old;
        }
    }

    unsigned type() {
        return _type;
    }
    unsigned type() const {
        return _type;
    }
    capsel_t sel() const {
        return key();
    }
    mht_key_t id() const {
        return _id;
    }
    CapTable *table() {
        return _tbl;
    }
    const CapTable *table() const {
        return _tbl;
    }
    mht_key_t parent() {
        return _parent;
    }
    mht_key_t parent() const {
        return _parent;
    }
    m3::SList<Child> &children() {
        return _children;
    }
    const m3::SList<Child> &children() const {
        return _children;
    }
    void addChild(mht_key_t child);
    void removeChild(mht_key_t child);
    void removeChildAllTypes(mht_key_t child);

    static inline ItemType capTypeToItemType(unsigned capType) {
        switch(capType) {
        case SERVICE:
            return ItemType::SRVCAP;
        case SESSION:
            return ItemType::SESSCAP;
        case MSG:
            return ItemType::MSGCAP;
        case MEM:
            return ItemType::MEMCAP;
        case MSG | MEM:
            return ItemType::MEMCAP;
        case MAP:
            return ItemType::MAPCAP;
        case VIRTPE:
            return ItemType::VPECAP;
        default:
            PANIC("Undefined capability type (" << capType << ")");
        }
    }

    void put(CapTable *tbl, capsel_t sel) {
        _tbl = tbl;
        key(sel);
    }

    size_t serializedSizeBase() const {
        return serializedSizeFixed;
    }
    size_t serializedSize() const {
        return serializedSizeBase();
    }
    static size_t serializedSizeTyped(Capability *cap);

    void serialize(GateOStream &ser) {
        ser << _id;
    }
    static void serializeTyped(GateOStream &ser, Capability *cap);

    /**
     * Currently not used.
     * Note that the ID should be set manually beforehand.
     * @param is    The input stream
     */
    template<class T>
    void deserialize(UNUSED T &is) {
    }

    template<class T>
    static Capability *createFromStream(T &is);

    void printChilds(m3::OStream &os, int layer = 0) const;

private:
    virtual m3::Errors::Code revoke() {
        return m3::Errors::NO_ERROR;
    }
    virtual Capability *clone(CapTable *tbl, capsel_t sel) = 0;

public:
    // type, selector and the VPE (hence the CapTable) are encoded in the capID
    static constexpr size_t serializedSizeFixed = m3::ostreamsize<mht_key_t>();

protected:
    unsigned _type;
    mht_key_t _id;

private:
    CapTable *_tbl;
    mht_key_t _parent;
    m3::SList<Child> _children;
};

class MsgObject : public SlabObject<MsgObject>, public m3::RefCounted {
public:
    explicit MsgObject(label_t _label, int _core, int _vpe, int _epid, word_t _credits, mht_key_t _id)
        : RefCounted(), label(_label), core(_core), vpe(_vpe), epid(_epid), credits(_credits),
          derived(false), id(_id) {
    }
    template<class T>
    explicit MsgObject(T &is, mht_key_t objID)
        : id(objID) {
        deserialize(is);
    }
    virtual ~MsgObject() {
    }

    virtual void revokeAction() {
    }

    size_t serializedSize() const {
        return serializedSizeFixed;
    }
    void serialize(GateOStream& ser) {
        ser << label << core << vpe << epid << credits;
    }
    template<class T>
    void deserialize(T &is) {
        is >> label >> core >> vpe >> epid >> credits;
    }

    static constexpr size_t serializedSizeFixed = m3::ostreamsize<label_t, int, int, int, word_t>();

    label_t label;
    int core;
    int vpe;
    int epid;
    word_t credits;
    bool derived;
    mht_key_t id;
};

class MemObject : public MsgObject {
public:
    explicit MemObject(uintptr_t addr, size_t size, uint perms, int core, int vpe,
            int epid, mht_key_t _id)
        : MsgObject(addr | perms, core, vpe, epid, size, _id) {
        assert((addr & m3::KIF::Perm::RWX) == 0);
    }
//    virtual ~MemObject();
    // Note: Freeing has moved to revokeAction() for the same reason as closing did in the SessionObject.
    virtual void revokeAction() override;
};

class SessionObject : public SlabObject<SessionObject>, public m3::RefCounted {
public:
    explicit SessionObject(Service *_srv, word_t _ident, mht_key_t srvid)
        : RefCounted(), servowned(), ident(_ident), srv(_srv), srvID(srvid) {
    }
    // Note: Closing session has moved to explicit close() function. Otherwise temporary SessionObjects
    //       transferred via MHT would cause closing the session on destruction.
    // TODO MIG: check how to use the servowned for serialization/deser.
    ~SessionObject();

    void close();

    bool servowned;
    word_t ident;
    m3::Reference<Service> srv;
    mht_key_t srvID;
};

class MsgCapability : public SlabObject<MsgCapability>, public Capability {
protected:
    explicit MsgCapability(CapTable *tbl, capsel_t sel, unsigned type, MsgObject *_obj,
            mht_key_t _objID, mht_key_t capid)
        : Capability(tbl, sel, type, capid), localepid(-1), obj(_obj), objID(_objID) {
    }

public:
    explicit MsgCapability(CapTable *tbl, capsel_t sel, label_t label, int core, int vpe, int epid,
        word_t credits, mht_key_t objid, mht_key_t capid)
        : MsgCapability(tbl, sel, MSG, new MsgObject(label, core, vpe, epid, credits, objid),
            objid, capid) {
        objID = obj->id;
    }
    template<class T>
    explicit MsgCapability(T &is, mht_key_t capid)
        : Capability((HashUtil::hashToType(capid) == MEMCAP) ? (MEM | MSG) : MSG, capid),
        localepid(-1), obj() {
        deserialize(is);
    }

    size_t serializedSize() const {
        return m3::vostreamsize(serializedSizeBase() + serializedSizeFixed +
                MsgObject::serializedSizeFixed);
    }
    void serialize(GateOStream& ser) {
        Capability::serialize(ser);
        ser << objID;
        obj->serialize(ser);
    }
    template<class T>
    void deserialize(T &is) {
        is >> objID;
        obj = m3::Reference<MsgObject>(new MsgObject(is, objID));
    }

    void print(m3::OStream &os) const override;

protected:
    virtual m3::Errors::Code revoke() override;
    virtual Capability *clone(CapTable *tbl, capsel_t sel) override {
        MsgCapability *c = new MsgCapability(*this);
        c->_id = 0;
        c->put(tbl, sel);
        c->localepid = -1;
        return c;
    }

public:
    static constexpr size_t serializedSizeFixed = m3::ostreamsize<mht_key_t>();
    int localepid;
    m3::Reference<MsgObject> obj;
    mht_key_t objID;
};

class MemCapability : public MsgCapability {
public:
    explicit MemCapability(CapTable *tbl, capsel_t sel, uintptr_t addr, size_t size, uint perms,
            int core, int vpe, int epid, mht_key_t objid, mht_key_t capid)
        : MsgCapability(tbl, sel, MEM | MSG,
            new MemObject(addr, size, perms, core, vpe, epid, objid), objid, capid) {
    }
    template<class T>
    explicit MemCapability(T &is, mht_key_t capid) : MsgCapability(is, capid) {
    }

    void print(m3::OStream &os) const override;

    uintptr_t addr() const {
        return obj->label & ~m3::KIF::Perm::RWX;
    }
    size_t size() const {
        return obj->credits;
    }
    uint perms() const {
        return obj->label & m3::KIF::Perm::RWX;
    }

private:
    virtual Capability *clone(CapTable *tbl, capsel_t sel) override {
        MemCapability *c = new MemCapability(*this);
        c->put(tbl, sel);
        c->localepid = -1;
        c->_id = 0;
        return c;
    }
};

class MapCapability : public SlabObject<MapCapability>, public Capability {
public:
    explicit MapCapability(CapTable *tbl, capsel_t sel, uintptr_t _phys, uint _attr, mht_key_t capid);

    void remap(uint _attr);

    void print(m3::OStream &os) const override;

private:
    virtual m3::Errors::Code revoke() override;
    virtual Capability *clone(CapTable *tbl, capsel_t sel) override {
        return new MapCapability(tbl, sel, phys, attr, 0);
    }

public:
    uintptr_t phys;
    uint attr;
};

class ServiceCapability : public SlabObject<ServiceCapability>, public Capability {
public:
    explicit ServiceCapability(CapTable *tbl, capsel_t sel, Service *_inst, mht_key_t capid)
        : Capability(tbl, sel, SERVICE, capid), inst(_inst) {
    }
    template<class T>
    explicit ServiceCapability(UNUSED T &is, mht_key_t capid)
        : Capability(SERVICE, capid), inst(nullptr) {
    }

    void print(m3::OStream &os) const override;

private:
    virtual m3::Errors::Code revoke() override;
    virtual Capability *clone(CapTable *, capsel_t) override {
        /* not cloneable */
        return nullptr;
    }

public:
    // TODO
    // serialize the service
    m3::Reference<Service> inst;
};

class SessionCapability : public SlabObject<SessionCapability>, public Capability {
public:
    explicit SessionCapability(CapTable *tbl, capsel_t sel, Service *srv, mht_key_t srvid, word_t ident, mht_key_t capid)
        : Capability(tbl, sel, SESSION, capid), obj(new SessionObject(srv, ident, srvid)) {
    }
    template<class T>
    explicit SessionCapability(T &is, mht_key_t capid)
        : Capability(SESSION, capid), obj(new SessionObject(nullptr, 0, 0)) {
        deserialize(is);
    }

    size_t serializedSize() const {
        return serializedSizeBase() + serializedSizeFixed;
    }
    void serialize(GateOStream& ser) {
        Capability::serialize(ser);
        ser << obj->ident << obj->srvID;
    }
    template<class T>
    void deserialize(T &is) {
        is >> obj->ident >> obj->srvID;
    }

    void print(m3::OStream &os) const override;

private:
    virtual m3::Errors::Code revoke() override;
    virtual Capability *clone(CapTable *tbl, capsel_t sel) override {
        SessionCapability *s = new SessionCapability(*this);
        s->put(tbl, sel);
        s->_id = 0;
        return s;
    }

public:
    static constexpr size_t serializedSizeFixed = m3::ostreamsize<word_t, mht_key_t>();
    m3::Reference<SessionObject> obj;
};

class VPECapability : public SlabObject<VPECapability>, public Capability {
public:
    explicit VPECapability(CapTable *tbl, capsel_t sel, VPE *p, mht_key_t capid);
    VPECapability(const VPECapability &t);
    template<class T>
    explicit VPECapability(T &is, mht_key_t capid)
        : Capability(VIRTPE, capid) {
        deserialize(is);
    }

    size_t serializedSize() const {
        return m3::vostreamsize(serializedSizeBase() + serializedSizeFixed);
    }
    void serialize(GateOStream& ser) {
        Capability::serialize(ser);
        ser << vpeId;
    }
    template<class T>
    void deserialize(T &is) {
        is >> vpeId;
    }

    void print(m3::OStream &os) const override;

private:
    virtual m3::Errors::Code revoke() override;
    virtual Capability *clone(CapTable *tbl, capsel_t sel) override {
        VPECapability *v = new VPECapability(*this);
        v->put(tbl, sel);
        v->_id = 0;
        return v;
    }

public:
    VPE *vpe;
    static constexpr size_t serializedSizeFixed = m3::ostreamsize<mht_key_t>();
    mht_key_t vpeId; // TODO: if cap is put into table the selector changes, consequence: key changes
};

template<class T>
Capability *Capability::createFromStream(T &is) {
    mht_key_t id;
    is >> id;
    switch(HashUtil::hashToType(id)) {
    case ItemType::MSGCAP:
        return new MsgCapability(is, id);
    case ItemType::SRVCAP:
        return new ServiceCapability(is, id);
    case ItemType::SESSCAP:
        return new SessionCapability(is, id);
    case ItemType::VPECAP:
        return new VPECapability(is, id);
    case ItemType::MEMCAP:
        return new MemCapability(is, id);
    case ItemType::SERVICE:
    case ItemType::MSGOBJ:
    case ItemType::MEMOBJ:
    case ItemType::NOTYPE:
    default:
        PANIC("Stream does not contain capability");
    }
}

}
