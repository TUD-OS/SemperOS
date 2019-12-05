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
#include <base/tracing/Tracing.h>
#include <base/log/Kernel.h>
#include <base/WorkLoop.h>

#include "KernelcallHandler.h"
#include "SyscallHandler.h"
#include "WorkLoop.h"
#include "thread/ThreadManager.h"

#if defined(__host__)
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>

static int sigchilds = 0;

static void sigchild(int) {
    sigchilds++;
    signal(SIGCHLD, sigchild);
}

static void check_childs() {
    for(; sigchilds > 0; sigchilds--) {
        int status;
        pid_t pid = wait(&status);
        if(WIFEXITED(status)) {
            KLOG(VPES, "Child " << pid << " exited with status " << WEXITSTATUS(status));
        }
        else if(WIFSIGNALED(status)) {
            KLOG(VPES, "Child " << pid << " was killed by signal " << WTERMSIG(status));
            if(WCOREDUMP(status))
                KLOG(VPES, "Child " << pid << " core dumped");
        }
    }
}
#endif

namespace kernel {

void WorkLoop::run() {
#if defined(__host__)
    signal(SIGCHLD, sigchild);
#endif
    EVENT_TRACER_KWorkLoop_run();

    m3::DTU &dtu = m3::DTU::get();
    KernelcallHandler &krnlch = KernelcallHandler::get();
    SyscallHandler &sysch = SyscallHandler::get();
    m3::ThreadManager &tmng = m3::ThreadManager::get();
    int krnlep[DTU::KRNLC_GATES];
    for(int i = 0; i < DTU::KRNLC_GATES; i++) {
        krnlep[i] = krnlch.epid(i);
    }
    int sysep[DTU::SYSC_GATES];
    for(int i = 0; i < DTU::SYSC_GATES; i++)
        sysep[i] = sysch.epid(i);
    int srvep = sysch.srvepid();
    const m3::DTU::Message *msg;
    while(has_items()) {
        m3::DTU::get().wait();

        for(int i = 0; i < DTU::KRNLC_GATES; i++) {
            msg = dtu.fetch_msg(krnlep[i]);
            if(msg) {
                GateIStream is(krnlch.rcvgate(i), msg);
                krnlch.handle_message(is, nullptr);
            }
        }

        for(int i = 0; i < DTU::SYSC_GATES; i++) {
            msg = dtu.fetch_msg(sysep[i]);
            if(msg) {
                // we know the subscriber here, so optimize that a bit
                RecvGate *rgate = reinterpret_cast<RecvGate*>(msg->label);
                GateIStream is(*rgate, msg);
                sysch.handle_message(is, nullptr);
                EVENT_TRACE_FLUSH_LIGHT();
            }
        }

        msg = dtu.fetch_msg(srvep);
        if(msg) {
            RecvGate *gate = reinterpret_cast<RecvGate*>(msg->label);
            GateIStream is(*gate, msg);
            gate->notify_all(is);
        }

        tmng.yield();
#if defined(__host__)
        check_childs();
#endif
    }
}

}
