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

#include <base/arch/host/HWInterrupts.h>
#include <base/col/SList.h>

#include <m3/com/GateStream.h>
#include <m3/server/Server.h>
#include <m3/server/EventHandler.h>
#include <m3/Syscalls.h>

#include <sys/time.h>
#include <cstdlib>

using namespace m3;

class IntSessionData : public EventSessionData {
public:
    explicit IntSessionData(HWInterrupts::IRQ irq) : EventSessionData(), irq(irq) {
    }

    HWInterrupts::IRQ irq;
};

class IntEventHandler : public EventHandler {
public:
    virtual IntSessionData *handle_open(GateIStream &args) override {
        int irq;
        args >> irq;
        auto *sess = new IntSessionData(static_cast<HWInterrupts::IRQ>(irq));
        reply_vmsg(args, Errors::NO_ERROR, add_session(sess));
        return sess;
    }
};

static IntEventHandler *evhandler;

class HWIrqs : public WorkItem {
public:
    explicit HWIrqs() : _total_pending(), _pending() {
    }

    void add_irq(HWInterrupts::IRQ irq) {
        _pending[irq]++;
        _total_pending++;
    }

    virtual void work() override {
        if(_total_pending > 0) {
            HWInterrupts::Guard noints;
            for(size_t i = 0; i < sizeof(_pending) / sizeof(_pending[0]); ++i) {
                for(; _pending[i] > 0; _pending[i]--) {
                    for(auto &s : *evhandler) {
                        IntSessionData *sess = static_cast<IntSessionData*>(&s);
                        if(sess->gate() && sess->irq == i)
                            send_vmsg(*static_cast<SendGate*>(sess->gate()), sess->irq);
                    }
                }
            }
            _total_pending = 0;
        }
    }

private:
    int _total_pending;
    int _pending[HWInterrupts::IRQ::COUNT];
};

static HWIrqs hwirqs;

static void irq_handler(HWInterrupts::IRQ irq) {
    hwirqs.add_irq(irq);
}

int main() {
    HWInterrupts::set_handler(HWInterrupts::TIMER, irq_handler);
    HWInterrupts::set_handler(HWInterrupts::KEYB, irq_handler);

    evhandler = new IntEventHandler();
    Server<IntEventHandler> srv("interrupts", evhandler);

    env()->workloop()->add(&hwirqs, true);
    env()->workloop()->run();
    return 0;
}
