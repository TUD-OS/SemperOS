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

#include <m3/com/GateStream.h>
#include <m3/com/SendQueue.h>
#include <m3/server/Server.h>
#include <m3/server/EventHandler.h>
#include <m3/session/arch/host/Keyboard.h>
#include <m3/session/arch/host/Interrupts.h>
#include <m3/session/Session.h>

using namespace m3;

static Server<EventHandler> *server;

static constexpr size_t DATA_SIZE   = 256;

static char *gendata() {
    char *data = new char[DATA_SIZE];
    for(size_t i = 0; i < DATA_SIZE; ++i)
        data[i] = rand() % 256;
    return data;
}

static void timer_irq(GateIStream &, Subscriber<GateIStream&>*) {
    for(auto &h : server->handler()) {
        // skip clients that have a session but no gate yet
        if(h.gate()) {
            SendQueue::get().send(*static_cast<SendGate*>(h.gate()), gendata(),
                DATA_SIZE, SendQueue::array_deleter<char>);
        }
    }
}

int main() {
    Interrupts timerirqs("interrupts", HWInterrupts::TIMER);
    timerirqs.gate().subscribe(timer_irq);

    // now, register service
    server = new Server<EventHandler>("queuetest", new EventHandler(), nextlog2<4096>::val);

    env()->workloop()->add(&SendQueue::get(), true);
    env()->workloop()->run();
    return 0;
}
