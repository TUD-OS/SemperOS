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

#include <base/col/List.h>
#include <assert.h>

namespace m3 {

/**
 * The doubly linked list. Takes an arbitrary class as list-item and expects it to have a prev(),
 * prev(T*), next() and next(*T) method. In most cases, you should inherit from DListItem and
 * specify your class for T.
 */
template<class T>
class DList {
public:
    typedef ListIterator<T> iterator;
    typedef ListConstIterator<T> const_iterator;

    /**
     * Constructor. Creates an empty list
     */
    explicit DList() : _head(nullptr), _tail(nullptr), _len(0) {
    }

    /**
     * Removes all elements from the list
     */
    void clear() {
        _head = _tail = nullptr;
        _len = 0;
    }

    /**
     * @return the number of items in the list
     */
    size_t length() const {
        return _len;
    }

    /**
     * @return beginning of list (you can change list elements)
     */
    iterator begin() {
        return iterator(_head);
    }
    /**
     * @return end of list
     */
    iterator end() {
        return iterator(nullptr);
    }
    /**
     * @return tail of the list, i.e. the last valid item
     */
    iterator tail() {
        return iterator(_tail);
    }

    /**
     * @return beginning of list (you can NOT change list elements)
     */
    const_iterator cbegin() const {
        return const_iterator(_head);
    }
    /**
     * @return end of list (NOT changeable)
     */
    const_iterator cend() const {
        return const_iterator(nullptr);
    }
    /**
     * @return tail of the list, i.e. the last valid item (NOT changeable)
     */
    const_iterator ctail() const {
        return const_iterator(_tail);
    }

    /**
     * Inserts the given item as the first one into the list. This works in constant time.
     *
     * @param e the list item
     */
    void prepend(T *e) {
        assert(e->next() == nullptr);
        assert(e->prev() == nullptr);
        e->prev(nullptr);
        e->next(_head);
        if(_head)
            _head->prev(e);
        if(_tail == nullptr)
            _tail = e;
        _head = e;
        _len++;
    }

    /**
     * Appends the given item to the list. This works in constant time.
     *
     * @param e the list item
     * @return the position where it has been inserted
     */
    iterator append(T *e) {
        assert(e->next() == nullptr);
        assert(e->prev() == nullptr);
        if(_head == nullptr)
            _head = e;
        else
            _tail->next(e);
        e->prev(_tail);
        e->next(nullptr);
        _tail = e;
        _len++;
        return iterator(e);
    }

    /**
     * Removes the first item from the list and returns it.
     *
     * @return the first item or NULL if there is none
     */
    T *removeFirst() {
        T *first = _head;
        if(first) {
            _head = static_cast<T*>(first->next());
            if(_head)
                _head->prev(nullptr);
            if(first == _tail)
                _tail = nullptr;
            _len--;
            first->next(nullptr);
            first->prev(nullptr);
        }
        return first;
    }

    /**
     * Removes the given item from the list. This works in constant time.
     * Expects that the item is in the list!
     *
     * @param e the list item
     */
    void remove(T *e) {
        if(e->prev())
            e->prev()->next(e->next());
        if(e->next())
            e->next()->prev(e->prev());
        if(e == _head)
            _head = static_cast<T*>(e->next());
        if(e == _tail)
            _tail = static_cast<T*>(e->prev());
        e->next(nullptr);
        e->prev(nullptr);
        _len--;
    }

private:
    T *_head;
    T *_tail;
    size_t _len;
};

}
