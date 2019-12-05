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

#include <base/util/Profile.h>

#include <m3/pipe/IndirectPipeReader.h>

namespace m3 {

ssize_t IndirectPipeReader::read(void *buffer, size_t count) {
    size_t pos;
    Errors::Code res = _pipe.read(&pos, &count, &_lastid);
    if(res != Errors::NO_ERROR)
        return -1;
    if(count == 0)
        return 0;

    uint8_t *buf = reinterpret_cast<uint8_t*>(buffer);
    size_t off = pos % DTU_PKG_SIZE;
    if(off) {
        uint8_t tmp[DTU_PKG_SIZE];
        _mem.read_sync(tmp, sizeof(tmp), Math::round_dn(pos, DTU_PKG_SIZE));
        memcpy(buf, tmp + off, DTU_PKG_SIZE - off);
        pos = Math::round_up(pos, DTU_PKG_SIZE);
        count -= DTU_PKG_SIZE - off;
        buf += DTU_PKG_SIZE - off;
    }

    size_t rdamount = Math::round_dn(count, DTU_PKG_SIZE);
    if(rdamount) {
        _mem.read_sync(buf, rdamount, pos);
    }

    size_t rem = count % DTU_PKG_SIZE;
    if(rem) {
        uint8_t tmp[DTU_PKG_SIZE];
        _mem.read_sync(tmp, sizeof(tmp), pos + count - rem);
        memcpy(buf + count - rem, tmp, rem);
    }
    return count;
}

void IndirectPipeReader::delegate(VPE &vpe) {
    IndirectPipeFile::delegate(vpe);
    vpe.delegate(CapRngDesc(CapRngDesc::OBJ, _pipe.read_gate().sel(), 1));
    _pipe.attach(true);
}

File *IndirectPipeReader::unserialize(Unmarshaller &um) {
    capsel_t mem, sess, metagate, rdgate, wrgate;
    um >> mem >> sess >> metagate >> rdgate >> wrgate;
    return new IndirectPipeReader(mem, sess, metagate, rdgate, wrgate);
}

}
