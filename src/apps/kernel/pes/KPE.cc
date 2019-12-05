/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of M3 (Microkernel for Minimalist Manycores).
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

#include <base/log/Kernel.h>

#include "pes/KPE.h"
#include "KernelcallHandler.h"
#include "DTU.h"
#include "pes/PEManager.h"
#include "Coordinator.h"

namespace kernel {

#ifdef KERNEL_STATISTICS
unsigned long KPE::normalMsgs = 0;
unsigned long KPE::revocationMsgs = 0;
unsigned long KPE::replies = 0;
unsigned long KPE::delayedNormalMsgs = 0;
unsigned long KPE::delayedRevocationMsgs = 0;
unsigned long KPE::delayedReplies = 0;
#endif

bool KPE::_shutdownReplySent = false;

void KernelAllocation::set_max_ptes(uint n) {
    // KEnv is written after root PT and PTEs
    reserve_kenv(ptes + n * PAGE_SIZE);
    // RT is written after KEnv
    rtspace = kenv + KENV_SIZE;
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    size_t pe = m3::DTU::noc_to_pe(ptes);
    uintptr_t addr = m3::DTU::noc_to_virt(ptes);
    for(size_t i = 0; i < (n * PAGE_SIZE) / sizeof(buffer); ++i)
        DTU::get().write_mem(VPEDesc(pe, 0), addr + i * sizeof(buffer), buffer, sizeof(buffer));
}

KPE::~KPE() {
    // TODO: check if there are still callbacks registered
}

void KPE::sendTo(const void* data, size_t size) {
    // Use one slot less for normal sending in order to be able to receive replies
    // and another slot less for revocations to prevent deadlocks
    while(_msgsInflight >= KernelcallHandler::MAX_MSG_INFLIGHT - 2) {
        KLOG(KPES, "Sending to kernel #" << _id << " delayed due to msg slot shortage");
#ifdef KERNEL_STATISTICS
        delayedNormalMsgs++;
#endif
        int tid = m3::ThreadManager::get().current()->id();
        _waitingThrds.append(new WaitingKPE(tid));
        m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(tid));
        checkShutdown();
    }
    KLOG(KPES, "Sending " << size << "B to kernel #" << _id << " on core #" << _core);
#ifdef KERNEL_STATISTICS
    normalMsgs++;
#endif
    // TODO
    // messages larger than the receive buffer should be split
    assert(size + m3::DTU::HEADER_SIZE < Kernelcalls::MSG_SIZE);
    _msgsInflight++;
    DTU::get().send_to(VPEDesc(_core, _id), _remoteEP, Coordinator::get().kid(), data, size, _id, _localEP);
}

void KPE::sendRevocationTo(const void* data, size_t size) {
    // Use one slot less for normal sending in order to be able to receive replies
    while(_msgsInflight >= KernelcallHandler::MAX_MSG_INFLIGHT - 1) {
        KLOG(KPES, "Sending revocation to kernel #" << _id << " delayed due to msg slot shortage");
#ifdef KERNEL_STATISTICS
        delayedRevocationMsgs++;
#endif
        int tid = m3::ThreadManager::get().current()->id();
        // Revocations are prioritized and this inserted at the beginning
        _waitingThrds.insert(nullptr, new WaitingKPE(tid, true));
        m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(tid));
        checkShutdown();
    }
    KLOG(KPES, "Sending revocation of " << size << "B to kernel #" << _id << " on core #" << _core);
#ifdef KERNEL_STATISTICS
    revocationMsgs++;
#endif
    // TODO
    // messages larger than the receive buffer should be split
    assert(size + m3::DTU::HEADER_SIZE < Kernelcalls::MSG_SIZE);
    _msgsInflight++;
    DTU::get().send_to(VPEDesc(_core, _id), _remoteEP, Coordinator::get().kid(), data, size, _id, _localEP);
}

void KPE::reply(const void* data, size_t size) {
    assert(_msgsInflight < KernelcallHandler::MAX_MSG_INFLIGHT);
    while(_msgsInflight >= KernelcallHandler::MAX_MSG_INFLIGHT - 1 && _lastMsgReply) {
        KLOG(KPES, "Replying to kernel #" << _id << " delayed due to msg slot shortage");
#ifdef KERNEL_STATISTICS
        delayedReplies++;
#endif
        int tid = m3::ThreadManager::get().current()->id();
        _waitingThrds.append(new WaitingKPE(tid));
        m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(tid));
        checkShutdown();
    }
    KLOG(KPES, "Sending reply of " << size << "B to kernel #" << _id << " on core #" << _core);
#ifdef KERNEL_STATISTICS
    replies++;
#endif
    assert(size + m3::DTU::HEADER_SIZE < Kernelcalls::MSG_SIZE);
    _lastMsgReply = true;
    DTU::get().send_to(VPEDesc(_core, _id), _remoteEP, Coordinator::get().kid(), data, size, _id, _localEP);
}

void KPE::forwardTo(const void* data, size_t size, label_t label) {
    while(_msgsInflight >= KernelcallHandler::MAX_MSG_INFLIGHT - 2) {
        KLOG(KPES, "Forwarding to kernel #" << _id << " delayed due to msg slot shortage");
#ifdef KERNEL_STATISTICS
        delayedNormalMsgs++;
#endif
        int tid = m3::ThreadManager::get().current()->id();
        _waitingThrds.append(new WaitingKPE(tid));
        m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(tid));
        checkShutdown();
    }
    KLOG(KPES, "Forwarding " << size << "B from kernel #" << label << " to kernel #" << _id);
#ifdef KERNEL_STATISTICS
    normalMsgs++;
#endif
    // TODO
    // messages larger than the receive buffer should be split
    assert(size + m3::DTU::HEADER_SIZE < Kernelcalls::MSG_SIZE);
    _msgsInflight++;
    DTU::get().send_to(VPEDesc(_core, _id), _remoteEP, label, data, size, _id, _localEP);
}

unsigned int KPE::addCallback(std::function<void(GateIStream&, m3::Unmarshaller)> cb, m3::Unmarshaller data) {
    cbData* cbEntry = new cbData(cb, data);
    _callbacks.put(_nextcallbackID, cbEntry);
    return _nextcallbackID++;
}

void KPE::notify(unsigned int cbID, GateIStream& is, bool remove) {
    auto cb = _callbacks.get(cbID);
    cb->func(is, cb->data);
    if(remove)
        removeCallback(cbID);
}

void KPE::removeCallback(unsigned int cbID) {
    auto cb = _callbacks.get(cbID);
    if(cb->data.buffer())
        delete[] cb->data.buffer();
    delete cb;
    _callbacks.remove(cbID);
}

void KPE::checkShutdown() {
    if(Coordinator::get().shutdownIssued && !_shutdownReplySent) {
        if(PEManager::get().terminate()) {
            Kernelcalls::get().shutdown(Coordinator::get().getKPE(0), Kernelcalls::OpStage::KREPLY);
            _shutdownReplySent = true;
        }
    }

}
}
