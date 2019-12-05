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

#include <base/arch/host/SharedMemory.h>

#include <m3/server/Server.h>
#include <m3/session/arch/host/VGA.h>
#include <m3/stream/Standard.h>
#include <m3/VPE.h>

using namespace m3;

class VGAHandler : public Handler<> {
public:
    explicit VGAHandler(MemGate *vgamem) : _vgamem(vgamem) {
    }

    virtual void handle_obtain(SessionData *, RecvBuf *, GateIStream &args, uint capcount) override {
        if(capcount != 1) {
            reply_vmsg(args, Errors::INV_ARGS);
            return;
        }

        reply_vmsg(args, Errors::NO_ERROR, CapRngDesc(CapRngDesc::OBJ, _vgamem->sel()));
    }

private:
    MemGate *_vgamem;
};

int main() {
    SharedMemory vgamem("vga", VGA::SIZE, SharedMemory::JOIN);
    MemGate memgate = VPE::self().mem().derive(
        reinterpret_cast<uintptr_t>(vgamem.addr()), VGA::SIZE, MemGate::RW);

    Server<VGAHandler> srv("vga", new VGAHandler(&memgate));
    if(Errors::occurred())
        exitmsg("Unable to register service 'vga'");

    env()->workloop()->run();
    return 0;
}
