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

#include <m3/pipe/IndirectPipeWriter.h>

namespace m3 {

ssize_t IndirectPipeWriter::write(const void *buffer, size_t count) {
    size_t pos = 0;
    Errors::Code res = _pipe.write(&pos, count, &_lastid);
    assert((pos % DTU_PKG_SIZE) == 0);
    if(res != Errors::NO_ERROR)
        return -1;

    _mem.write_sync(buffer, Math::round_up(count, DTU_PKG_SIZE), pos);
    return count;
}

void IndirectPipeWriter::delegate(VPE &vpe) {
    IndirectPipeFile::delegate(vpe);
    vpe.delegate(CapRngDesc(CapRngDesc::OBJ, _pipe.write_gate().sel(), 1));
    _pipe.attach(false);
}

File *IndirectPipeWriter::unserialize(Unmarshaller &um) {
    capsel_t mem, sess, metagate, rdgate, wrgate;
    um >> mem >> sess >> metagate >> rdgate >> wrgate;
    return new IndirectPipeWriter(mem, sess, metagate, rdgate, wrgate);
}

}
