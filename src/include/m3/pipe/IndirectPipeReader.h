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

#include <m3/pipe/IndirectPipe.h>

namespace m3 {

class IndirectPipeReader : public IndirectPipeFile {
public:
    explicit IndirectPipeReader(capsel_t mem, capsel_t sess,
        capsel_t metagate, capsel_t rdgate, capsel_t wrgate)
        : IndirectPipeFile(mem, sess, metagate, rdgate, wrgate) {
    }
    ~IndirectPipeReader() {
        _pipe.close(true, _lastid);
    }

    virtual ssize_t read(void *, size_t) override;
    virtual ssize_t write(const void *, size_t) override {
        // not supported
        return 0;
    }

    virtual char type() const override {
        return 'I';
    }

    virtual void delegate(VPE &vpe) override;
    static File *unserialize(Unmarshaller &um);
};

}
