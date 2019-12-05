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

#pragma once

#include <base/util/String.h>
#include <base/PEDesc.h>

#include "pes/KPE.h"
#include "tests/KTestSuiteContainer.h"
#include "KernelcallHandler.h"

namespace kernel {
class Coordinator {
    friend class Kernelcalls;

public:
    Coordinator(const Coordinator &) = delete;
    Coordinator &operator=(const Coordinator &) = delete;
//    ~Coordinator();

    static void create(size_t kid, m3::String&& creatorBin, size_t creatorId, size_t creatorCore) {
        _inst = new Coordinator(kid, std::move(creatorBin), creatorId, creatorCore);
    }
    static void create(size_t kid) {
        _inst = new Coordinator(kid);
    }

    static Coordinator& get() {
        return *_inst;
    }
    size_t kid() const {
        return _kid;
    }
    KPE* creator() const {
        return _creator;
    }
    KPE* getKPE(size_t id) const {
        return _kpes.get(id);
    }
    KPE* tryGetKPE(size_t id) const {
        if(_kpes.exists(id))
            return _kpes.get(id);
        return nullptr;
    }
    KVStore<size_t, KPE*> &getKPEList() {
        return _kpes;
    }
    bool isKPE(size_t pe_id) const;
    uint numKPEs() {
        return _kpes.size();
    }

    /**
     * Sets up a new kernel and starts it
     * @param binary    Name of the kernel's binary
     * @param id        ID of the new kernel
     * @param core      The core id the new kernel should be run on
     * @param numPEs    Number of PEs to be managed by the new kernel
     */
    void startKrnl(int argc, char* argv[], m3::String&& binary, size_t id, size_t core, unsigned int numPEs);

    /**
     * Moves a kernel from the startingKrnls store to the kpes store, which makes
     * the new kernel an active participant.
     * @param id    The kernel's id
     * @return      Pointer to the KPE of the kernel
     */
    KPE* publishKrnl(size_t id);

    /**
     * Inform all other kernel about the change in the ddl membership
     *
     * @param PEs       Array of PEs to that are affected of the change
     * @param numPEs    Number of affected PEs
     * @param krnl      The new owner's ID
     * @param krnlCore  The new owner's core
     * @param flags     Flags of type ::MembershipFlags
     */
    void broadcastMemberUpdate(m3::PEDesc PEs[], uint numPEs, membership_entry::krnl_id_t krnl,
        membership_entry::pe_id_t krnlCore, MembershipFlags flags);

    void addKPE(m3::String &&prog, size_t id, size_t core, int localEp, int remoteEp) {
        _kpes.put(id, new KPE(m3::Util::move(prog), id, core, localEp, remoteEp));
    }

    void removeKPE(size_t id);

    uint broadcastCreateSess(int vpeID, m3::String &srvname, mht_key_t cap, GateOStream &args);
    uint broadcastAnnounceSrv(m3::String &srvname, mht_key_t id);
    uint broadcastShutdownRequest();
    void broadcastShutdown();
    void broadcastStartApps();

#ifdef KERNEL_TESTS
    void finishStartup() {
        _startup_done = true;
        if(!_startingKernels)
            startTests();
    }
    void startTests() {
        KTestSuiteContainer* testSuites = new KTestSuiteContainer();
        // add test suites here
        testSuites->run();
        delete testSuites;
    }
#endif

    int closingRequests;    ///< Number of outstanding shutdown requests if kernel 0; for others values > -1 indicate that a shutdown was requested before
    bool shutdownIssued;    ///< Indicates whether the KPE should shutdown when it stopped all its VPEs
    int shutdownRequests;
    int startSignsAwaited;  ///< Primary krnl: Number of outstanding startApp calls (signaling that services are set up); secondary: 1 = wait, 0 = go
    bool startSignSent;
private:
    explicit Coordinator(size_t kid, m3::String&& creatorBin, size_t creatorId, size_t creatorCore);
    explicit Coordinator(size_t kid);
    size_t _kid;
    KPE* _creator;
    KVStore<size_t, KPE*> _kpes;
#ifdef KERNEL_TESTS
    uint _startingKernels;
    bool _startup_done;
#endif
    static Coordinator* _inst;
};
}
