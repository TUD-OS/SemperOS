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

#include <base/Common.h>
#include <base/stream/Serial.h>
#include <base/Backtrace.h>
#include <base/Env.h>

#include <m3/com/RecvGate.h>
#include <m3/com/RecvBuf.h>
#include <m3/stream/Standard.h>
#include <m3/Syscalls.h>
#include <m3/VPE.h>

namespace m3 {

class EnvUserBackend : public BaremetalEnvBackend {
public:
    explicit EnvUserBackend() {
        _workloop = new WorkLoop();
    }

    virtual void init() override {
        // wait until the kernel has initialized our core
        volatile Env *senv = env();
        while(senv->coreid == 0)
            ;

        // TODO argv is always present, isn't it?
        Serial::init(env()->argv ? env()->argv[0] : "Unknown", env()->coreid);
    }

    virtual void reinit() override {
        // wait until the kernel has initialized our core
        volatile Env *senv = env();
        while(senv->coreid == 0)
            ;

#if defined(__t3__)
        // set default receive buffer again
        RecvBuf &def = RecvBuf::def();
        DTU::get().configure_recv(def.epid(), reinterpret_cast<word_t>(def.addr()),
            def.order(), def.msgorder(), def.flags());
#endif

        Serial::init(env()->argv ? env()->argv[0] : "Unknown", senv->coreid);
        EPMux::get().reset();
#if defined(__t2__)
        DTU::get().reset();
#endif

        VPE::self().init_state();
        VPE::self().init_fs();

        EVENT_TRACE_REINIT();
    }

    void exit(int code) override {
        Syscalls::get().exit(code);
    }
};

EXTERN_C void init_env(Env *e) {
    e->backend = new EnvUserBackend();
}

}
