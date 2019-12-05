/*
 * Copyright (C) 2013, Nils Asmussen <nils@os.inf.tu-dresden.de>
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
#include <base/util/Util.h>

#include "Coordinator.h"
#include "pes/KPE.h"
#include "DTU.h"
#include "Kernelcalls.h"
#include "pes/PEManager.h"

namespace kernel {

Coordinator* Coordinator::_inst;

Coordinator::Coordinator(size_t kid, m3::String&& creatorBin, size_t creatorId, size_t creatorCore)
    : closingRequests(-1), shutdownIssued(false), shutdownRequests(0), startSignsAwaited(1),
    startSignSent(false), _kid(kid), _creator(new KPE(m3::Util::move(creatorBin), creatorId,
    creatorCore, DTU::KRNLC_EP, Platform::creatorEp())) {
    _kpes.put(creatorId, _creator);
#ifdef KERNEL_TESTS
    _startup_done = true;
#endif
#ifndef SYNC_APP_START
    startSignsAwaited = 0;
    startSignSent = true;
#endif
}

Coordinator::Coordinator(size_t kid)
    : closingRequests(-1), shutdownIssued(false), shutdownRequests(0), startSignsAwaited(0),
    startSignSent(false), _kid(kid), _creator(nullptr) {
#ifdef KERNEL_TESTS
    _startup_done = false;
#endif
#ifndef SYNC_APP_START
    startSignSent = true;
#endif
}

bool Coordinator::isKPE(size_t pe_id) const {
    for(auto it = _kpes.begin(); it != _kpes.end(); it++) {
        if(it->val->core() == pe_id)
            return true;
    }
    return false;
}

void Coordinator::startKrnl(int argc, char* argv[], m3::String&& binary, size_t id,
    size_t core, unsigned int numPEs) {
    KLOG(KPES, "Starting kernel (binary=" << binary << ", argc=" << argc << ", id=" <<
        id << ", core=" << core << ", numPEs=" << numPEs << ")");
    DTU::get().set_vpeid(VPEDesc(core, VPE::INVALID_ID));
    DTU::get().suspend(VPEDesc(core, VPE::INVALID_ID));
    DTU::get().privilege(core);

    int localEp = KernelcallHandler::get().reserve_ep(id);
    if(localEp == -1)
        PANIC("No msg slots for kernel #" << id << " left");
    KLOG(KPES, "Reserved EP " << localEp);

    KPE* newkrnl = new KPE(m3::Util::move(binary), id, core, localEp, DTU::KRNLC_EP);
    m3::PEDesc releasedPEs[numPEs];
    releasedPEs[0] = Platform::pe_by_core(core);
    PEManager::get()._vpes[core] = reinterpret_cast<VPE*>(1);
    for(unsigned int i = 1; i < numPEs; i++) {
        // take the cores from the back of the free list
        size_t freeCore = PEManager::get().free_compute_core_from_back(numPEs - i - 1);
        if(freeCore == Platform::MAX_PES)
            PANIC("Not enough free compute cores to start kernel #" << id << " with " << numPEs << " PEs");
        // mark the PE as "used"
        PEManager::get()._vpes[freeCore] = reinterpret_cast<VPE*>(1);
        releasedPEs[i] = Platform::pe_by_core(freeCore);
    }

    // update the DDL's membership table to indicate that these PEs are migrating
    MHTInstance::getInstance().updateMembership(releasedPEs, numPEs, id, core, MembershipFlags::MIGRATING, false);
    newkrnl->start(argc, argv, numPEs, releasedPEs);
#ifdef SYNC_APP_START
    startSignsAwaited++;
#endif
#ifdef KERNEL_TESTS
    _startingKernels++;
#endif
    // yield and update status of PEs when resumed
    // we are resumed by the SIGVITAL of the remote kernel
    m3::ThreadManager &tmng = m3::ThreadManager::get();
    int tid = tmng.current()->id();
    tmng.wait_for(reinterpret_cast<void*>(tid));

    if(*reinterpret_cast<const m3::Errors::Code*>(tmng.get_current_msg()) != m3::Errors::NO_ERROR)
        PANIC("Error starting remote kernel #" << id << "! Error: " <<
            m3::Errors::to_string(*reinterpret_cast<const m3::Errors::Code*>(tmng.get_current_msg())));

    // send information about existing kernels
    Kernelcalls::get().announceKrnls(newkrnl, _kid);

    tmng.wait_for(reinterpret_cast<void*>(tid));

    if(*reinterpret_cast<const m3::Errors::Code*>(tmng.get_current_msg()) != m3::Errors::NO_ERROR)
        PANIC("Error starting remote kernel #" << id << "! Error: " <<
            m3::Errors::to_string(*reinterpret_cast<const m3::Errors::Code*>(tmng.get_current_msg())));

    // finish migration by updating the member table
    MHTInstance::getInstance().updateMembership(releasedPEs, numPEs, id, core, MembershipFlags::NONE, false);
    // unmark PEs
    for(unsigned int i = 1; i < numPEs; i++)
        PEManager::get()._vpes[releasedPEs[i].core_id()] = nullptr;

    _kpes.put(id, newkrnl);

    // send information about already existent services
    ServiceList &srv = ServiceList::get();
    for(auto s = srv.begin(); s != srv.end(); s++)
        Kernelcalls::get().announceSrv(newkrnl, s->id(), s->name());


#ifdef KERNEL_TESTS
    _startingKernels--;
    if(_startup_done && !_startingKernels) {
        startTests();
    }
#endif
}

void Coordinator::broadcastMemberUpdate(m3::PEDesc PEs[], uint numPEs, membership_entry::krnl_id_t krnl,
    membership_entry::pe_id_t krnlCore, MembershipFlags flags) {
    for(auto it = _kpes.begin(); it != _kpes.end(); it++)
        Kernelcalls::get().membershipUpdate(it->val, PEs, numPEs, krnl, krnlCore, flags);
}

void Coordinator::removeKPE(size_t id) {
    delete _kpes.get(id);
    _kpes.remove(id);

    // TODO
    // Currently the distributed shutdown mechanism is just to let the system stop.
    // In order to reuse the cores and resouces of a closed kernel we would need to
    // give them back and update the membership tables
}

uint Coordinator::broadcastCreateSess(int vpeID, m3::String& srvname, mht_key_t cap, GateOStream &args) {
    for(auto it = _kpes.begin(); it != _kpes.end(); it++)
        Kernelcalls::get().createSessFwd(it->val, vpeID, srvname, cap, args);
    return _kpes.size();
}

uint Coordinator::broadcastAnnounceSrv(m3::String& srvname, mht_key_t id) {
    for(auto it = _kpes.begin(); it != _kpes.end(); it++)
        Kernelcalls::get().announceSrv(it->val, id, srvname);
    return _kpes.size();
}

uint Coordinator::broadcastShutdownRequest() {
    uint requests = 0;
    for(auto it = _kpes.begin(); it != _kpes.end(); it++)
        if(it->val->getShutdownState() == KPE::ShutdownState::NONE) {
            Kernelcalls::get().requestShutdown(it->val, Kernelcalls::OpStage::KREQUEST);
            it->val->setShutdownState(KPE::ShutdownState::INFLIGHT);
            requests++;
        }
    return requests;
}

void Coordinator::broadcastShutdown() {
    for(auto it = _kpes.begin(); it != _kpes.end(); it++)
        Kernelcalls::get().shutdown(it->val, Kernelcalls::OpStage::KREQUEST);
}

void Coordinator::broadcastStartApps() {
    for(auto it = _kpes.begin(); it != _kpes.end(); it++)
        Kernelcalls::get().startApps(it->val);

#ifdef SYNC_APP_START
    // identify for runtime extraction script
    if (kid() == 0)
        m3::DTU::get().debug_msg(0x11000000);
#endif
}

}
