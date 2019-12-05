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

#include <m3/session/Pipe.h>

namespace m3 {

void Pipe::attach(bool reading) {
    send_receive_vmsg(_metagate, ATTACH, reading);
}

Errors::Code Pipe::read(size_t *pos, size_t *amount, int *lastid) {
    GateIStream reply = send_receive_vmsg(_rdgate, *amount, *lastid);
    reply >> Errors::last;
    if(Errors::last == Errors::NO_ERROR)
        reply >> *pos >> *amount >> *lastid;
    return Errors::last;
}

Errors::Code Pipe::write(size_t *pos, size_t amount, int *lastid) {
    GateIStream reply = send_receive_vmsg(_wrgate, amount, *lastid);
    reply >> Errors::last;
    if(Errors::last == Errors::NO_ERROR)
        reply >> *pos >> *lastid;
    return Errors::last;
}

void Pipe::close(bool reading, int lastid) {
    send_receive_vmsg(_metagate, CLOSE, reading, lastid);
}

}
