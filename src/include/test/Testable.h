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

#include <base/Common.h>
#include <base/util/String.h>

namespace test {

class Testable {
public:
    explicit Testable(const m3::String& name)
        : _name(name), _succeeded(), _failed() {
    }
    virtual ~Testable() {
    }

    const m3::String& get_name() const {
        return _name;
    }
    size_t get_succeeded() const {
        return _succeeded;
    }
    size_t get_failed() const {
        return _failed;
    }

    virtual void run() = 0;

protected:
    void success() {
        _succeeded++;
    }
    void failed() {
        _failed++;
    }

private:
    m3::String _name;
    size_t _succeeded;
    size_t _failed;
};

}
