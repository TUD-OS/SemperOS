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

#include <base/Env.h>

#include <m3/com/GateStream.h>
#include <m3/session/Session.h>
#include <m3/session/arch/host/Plasma.h>
#include <m3/session/arch/host/Keyboard.h>

using namespace m3;

static void kb_event(Plasma *plasma, GateIStream &is, Subscriber<GateIStream&> *) {
    Keyboard::Event ev;
    is >> ev;
    if(ev.isbreak)
        return;
    switch(ev.keycode) {
        case Keyboard::VK_LEFT:
            plasma->left();
            break;
        case Keyboard::VK_RIGHT:
            plasma->right();
            break;
        case Keyboard::VK_UP:
            plasma->colup();
            break;
        case Keyboard::VK_DOWN:
            plasma->coldown();
            break;
    }
}

int main() {
    // create event gate
    Keyboard kb("keyb");
    Plasma plasma("plasma");
    kb.gate().subscribe(std::bind(kb_event, &plasma, std::placeholders::_1, std::placeholders::_2));

    env()->workloop()->run();
    return 0;
}
