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

#include <base/util/Util.h>
#include <base/util/String.h>
#include <base/util/Math.h>
#include <base/tracing/Tracing.h>
#include <base/com/Marshalling.h>
#include <base/DTU.h>
#include <base/Heap.h>
#include <assert.h>

namespace m3 {

/**
 * The gate stream classes provide an easy abstraction to marshall or unmarshall data when
 * communicating between VPEs. Therefore, if you want to combine multiple values into a single
 * message or extract multiple values from a message, this is the abstraction you might want to use.
 * If you already have the data to send, you should directly use the send-method of SendGate. If
 * you don't want to extract values from a message but directly access the message, use the
 * data-field of the message you received.
 *
 * All classes work with (variadic) templates and are thus type-safe. Of course, that does not
 * relieve you from taking care that sender and receiver agree on the types of the values that are
 * exchanged via messaging.
 */

template<class RGATE, class SGATE>
class BaseGateIStream;

/**
 * The gate stream to marshall values into a message and send it over an endpoint. Thus, it "outputs"
 * values into a message.
 */
template<class RGATE, class SGATE>
class BaseGateOStream : public Marshaller {
public:
    explicit BaseGateOStream(unsigned char *bytes, size_t total) : Marshaller(bytes, total) {
    }
    BaseGateOStream(const BaseGateOStream &) = default;
    BaseGateOStream &operator=(const BaseGateOStream &) = default;

    /**
     * Replies the current content of this GateOStream as a message to the given message.
     *
     * @param gate the gate that hosts the message to reply to
     * @param msg the message to reply to
     * @return the error code or Errors::NO_ERROR
     */
    Errors::Code reply(RGATE &gate, const void *msg) {
        return gate.reply_sync(bytes(), total(), m3::DTU::get().get_msgoff(gate.epid(), msg));
    }

    using Marshaller::put;

    /**
     * Puts all remaining items (the ones that haven't been read yet) of <is> into this GateOStream.
     *
     * @param is the GateIStream
     * @return *this
     */
    void put(const BaseGateIStream<RGATE, SGATE> &is);
};

/**
 * An implementation of GateOStream that hosts the message as a member. E.g. you can put an object
 * of this class on the stack, which would host the message on the stack.
 * In most cases, you don't want to use this class yourself, but the free-standing convenience
 * functions below that automatically determine <SIZE>.
 *
 * @param SIZE the max. size of the message
 */
template<size_t SIZE, class RGATE, class SGATE>
class BaseStaticGateOStream : public BaseGateOStream<RGATE, SGATE> {
public:
    explicit BaseStaticGateOStream() : BaseGateOStream<RGATE, SGATE>(_bytes, SIZE) {
    }
    template<size_t SRCSIZE>
    BaseStaticGateOStream(const BaseStaticGateOStream<SRCSIZE, RGATE, SGATE> &os)
            : BaseGateOStream<RGATE, SGATE>(os) {
        static_assert(SIZE >= SRCSIZE, "Incompatible sizes");
        memcpy(_bytes, os._bytes, sizeof(os._bytes));
    }
    template<size_t SRCSIZE>
    BaseStaticGateOStream &operator=(const BaseStaticGateOStream<SRCSIZE, RGATE, SGATE> &os) {
        static_assert(SIZE >= SRCSIZE, "Incompatible sizes");
        BaseGateOStream<RGATE, SGATE>::operator=(os);
        if(&os != this)
            memcpy(_bytes, os._bytes, sizeof(os._bytes));
        return *this;
    }

private:
    alignas(DTU_PKG_SIZE) unsigned char _bytes[SIZE];
};

/**
 * An implementation of GateOStream that hosts the message on the stack by using alloca.
 */
template<class RGATE, class SGATE>
class BaseAutoGateOStream : public BaseGateOStream<RGATE, SGATE> {
public:
#if defined(__t2__) or defined(__t3__)
    // TODO alloca() uses movsp which causes an exception to be handled appropriately. since this
    // isn't that trivial to implement, we're using malloc instead.
    explicit BaseAutoGateOStream(size_t size)
        : BaseGateOStream<RGATE, SGATE>(static_cast<unsigned char*>(
            Heap::alloc(Math::round_up(size, DTU_PKG_SIZE))), Math::round_up(size, DTU_PKG_SIZE)) {
    }
    ~BaseAutoGateOStream() {
        Heap::free(this->_bytes);
    }
#else
    ALWAYS_INLINE explicit BaseAutoGateOStream(size_t size)
        : BaseGateOStream<RGATE, SGATE>(static_cast<unsigned char*>(
            alloca(Math::round_up(size, DTU_PKG_SIZE))), Math::round_up(size, DTU_PKG_SIZE)) {
    }
#endif

    BaseAutoGateOStream(BaseAutoGateOStream &&os)
        : BaseGateOStream<RGATE, SGATE>(os) {
    }

    bool is_on_heap() const {
#if defined(__t2__) or defined(__t3__)
        return true;
#else
        return false;
#endif
    }

    /**
     * Claim the ownership of the data from this class. Thus, it will not free it.
     */
    void claim() {
        this->_bytes = nullptr;
    }
};

/**
 * The gate stream to unmarshall values from a message. Thus, it "inputs" values from a message
 * into variables.
 *
 * Note: unfortunately, we can't reuse the functionality of Unmarshaller here. It seems to be a
 * compiler-bug when building for Xtensa. The compiler generates wrong code when we initialize the
 * _length field to _msg->length.
 */
template<class RGATE, class SGATE>
class BaseGateIStream {
public:
    /**
     * Creates an object to read the first not acknowledged message from <gate>.
     *
     * @param gate the gate to fetch the message from
     * @param err the error code
     */
    explicit BaseGateIStream(RGATE &gate, const DTU::Message *msg, Errors::Code err)
        : _err(err), _ack(true), _pos(0), _gate(&gate), _msg(msg) {
    }

    /**
     * Creates an object for given message.
     *
     * @param gate the gate to fetch the message from
     * @param msg the message
     */
    explicit BaseGateIStream(RGATE &gate, const DTU::Message *msg)
        : _err(Errors::NO_ERROR), _ack(true), _pos(0), _gate(&gate), _msg(msg) {
    }

    // don't do the ack twice. thus, copies never ack.
    BaseGateIStream(const BaseGateIStream &is)
        : _err(is._err), _ack(), _pos(is._pos), _gate(is._gate), _msg(is._msg) {
    }
    BaseGateIStream &operator=(const BaseGateIStream &is) {
        if(this != &is) {
            _err = is._err;
            _ack = false;
            _pos = is._pos;
            _gate = is._gate;
            _msg = is._msg;
        }
        return *this;
    }
    BaseGateIStream &operator=(BaseGateIStream &&is) {
        if(this != &is) {
            _err = is._err;
            _ack = is._ack;
            _pos = is._pos;
            _gate = is._gate;
            _msg = is._msg;
            is._ack = 0;
        }
        return *this;
    }
    BaseGateIStream(BaseGateIStream &&is)
        : _err(is._err), _ack(is._ack), _pos(is._pos), _gate(is._gate), _msg(is._msg) {
        is._ack = 0;
    }
    ~BaseGateIStream() {
        finish();
    }

    /**
     * @return the error that occurred (or Errors::NO_ERROR)
     */
    Errors::Code error() const {
        return _err;
    }
    /**
     * @return the receive gate
     */
    RGATE &gate() {
        return *_gate;
    }
    /**
     * @return the message (header + payload)
     */
    const DTU::Message &message() const {
        return *_msg;
    }
    /**
     * @return the label of the message
     */
    label_t label() const {
        return _msg->label;
    }
    /**
     * @return the current position, i.e. the offset of the unread data
     */
    size_t pos() const {
        return _pos;
    }
    /**
     * @return the length of the message in bytes
     */
    size_t length() const {
#if defined(__t3__)
        return _msg->length * DTU_PKG_SIZE;
#else
        return _msg->length;
#endif
    }
    /**
     * @return the remaining bytes to read
     */
    size_t remaining() const {
        return length() - _pos;
    }
    /**
     * @return the message payload
     */
    const unsigned char *buffer() const {
        return _msg->data;
    }

    /**
     * Replies the message constructed by <os> to this message
     *
     * @param os the GateOStream hosting the message to reply
     * @return the error code or Errors::NO_ERROR
     */
    Errors::Code reply(const BaseGateOStream<RGATE, SGATE> &os) {
        return reply(os.bytes(), os.total());
    }
    /**
     * Replies the given message to this one
     *
     * @param data the message data
     * @param len the length of the message
     * @return the error code or Errors::NO_ERROR
     */
    Errors::Code reply(const void *data, size_t len) {
        Errors::Code res = _gate->reply_sync(data, len, DTU::get().get_msgoff(_gate->epid(), _msg));
        // it's already acked
        _ack = false;
        return res;
    }

    /**
     * Pulls the given values out of this stream
     *
     * @param val the value to write to
     * @param args the other values to write to
     */
    template<typename T, typename... Args>
    void vpull(T &val, Args &... args) {
        *this >> val;
        vpull(args...);
    }

    /**
     * Pulls a value into <value>.
     *
     * @param value the value to write to
     * @return *this
     */
    template<typename T>
    BaseGateIStream & operator>>(T &value) {
        assert(_pos + sizeof(T) <= length());
        value = *reinterpret_cast<const T*>(_msg->data + _pos);
        _pos += Math::round_up(sizeof(T), sizeof(ulong));
        return *this;
    }
    BaseGateIStream & operator>>(String &value) {
        assert(_pos + sizeof(size_t) <= length());
        size_t len = *reinterpret_cast<const size_t*>(_msg->data + _pos);
        _pos += sizeof(size_t);
        assert(_pos + len <= length());
        value.reset(reinterpret_cast<const char*>(_msg->data + _pos), len);
        _pos += Math::round_up(len, sizeof(ulong));
        return *this;
    }

    /**
     * Disables acknowledgement of the message. That is, it will be marked as read, but you have
     * to ack the message on your own via DTU::get().mark_acked(<epid>).
     */
    void claim() {
        _ack = false;
    }

    /**
     * Finishes this message, i.e. moves the read-position in the ringbuffer forward. If
     * acknowledgement has not been disabled (see claim), it will be acked.
     */
    void finish() {
        if(_ack) {
            DTU::get().mark_read(_gate->epid(), DTU::get().get_msgoff(_gate->epid(), _msg));
            _ack = false;
        }
    }

private:
    // needed as recursion-end
    void vpull() {
    }

    Errors::Code _err;
    bool _ack;
    size_t _pos;
    RGATE *_gate;
    const DTU::Message *_msg;
};

template<class RGATE, class SGATE>
inline void BaseGateOStream<RGATE, SGATE>::put(const BaseGateIStream<RGATE, SGATE> &is) {
    assert(fits(_bytecount, is.remaining()));
    memcpy(const_cast<unsigned char*>(bytes()) + _bytecount, is.buffer() + is.pos(), is.remaining());
    _bytecount += is.remaining();
}

}
