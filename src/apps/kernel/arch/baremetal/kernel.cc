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

#include <base/stream/Serial.h>
#include <base/tracing/Tracing.h>
#include <base/log/Kernel.h>
#include <base/DTU.h>
#include <base/WorkLoop.h>
#include <thread/ThreadManager.h>

#include "com/RecvBufs.h"
#include "mem/MainMemory.h"
#include "pes/PEManager.h"
#include "SyscallHandler.h"
#include "Platform.h"

using namespace kernel;


void shutdown() {
    EVENT_TRACE_FLUSH();

    KLOG(INFO, "Shutting down kernel #" << Platform::kernelId());

    PEManager::destroy();

    if(Coordinator::get().kid() == 0)
        m3::Machine::shutdown();
    else {
        // TODO
        // when kernels are shutdown we have to go back to idle
        while(true)
            m3::DTU::get().wait();
    }
}

void kernel_thrd_entry(void* ) {
    KLOG(KPES, "Kernel thread #" << m3::ThreadManager::get().current()->id() << " started");

    m3::env()->workloop()->run();

    shutdown();
}


int main(int argc, char *argv[]) {
    EVENT_TRACE_INIT_KERNEL();

    KLOG(MEM, MainMemory::get());

    RecvBufs::init();
    PEManager::create();
    // if we have been created by another kernel, tell him we are alive
    if(Coordinator::get().creator() != nullptr) {
        KLOG(KRNLC, "Kernel #" << Platform::kernelId() << " is contacting its creator (kernel #" << Platform::creatorKernelId() << ")");
        Kernelcalls::get().sigvital(Coordinator::get().creator(), Platform::creatorThread(), m3::Errors::NO_ERROR);
    }
    // 1 thread per PE (maximum PEs = syscall gates * DTU message slots) + threads for k2k communication
    uint krnlThreads = ( (MHTInstance::getInstance().localPEs() < DTU::SYSC_GATES * m3::DTU::MAX_MSG_SLOTS) ?
        MHTInstance::getInstance().localPEs() : (DTU::SYSC_GATES * m3::DTU::MAX_MSG_SLOTS) ) + 1 +
        (DTU::KRNLC_GATES * m3::DTU::MAX_MSG_SLOTS / KernelcallHandler::MAX_MSG_INFLIGHT);
    for(uint i = 0; i < krnlThreads - 1; i++)
        new m3::Thread(kernel_thrd_entry, nullptr);

    // after receiving information about other kernels by announceKrnls kernelcall
    // this thread will be continued
    if(Coordinator::get().creator() != nullptr)
        m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(0));

    PEManager::get().load(argc - 1, argv + 1);

    KLOG(INFO, "Kernel #" << Platform::kernelId() << " (running " << krnlThreads << " kernel threads) is ready");
#ifndef SYNC_APP_START
    // identify for runtime extraction script
    m3::DTU::get().debug_msg(0x11000000);
#endif

    m3::env()->workloop()->run();

    shutdown();
}
