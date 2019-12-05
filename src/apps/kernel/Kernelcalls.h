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

#include <m3/com/SendGate.h>
#include <base/util/String.h>
#include <base/util/CapRngDesc.h>
#include <base/Errors.h>
#include <base/PEDesc.h>

#include "Gate.h"
#include "ddl/MHTTypes.h"

namespace kernel {

class KPE;

class Kernelcalls {
public:
    static constexpr size_t MSG_SIZE         = 2048;
    static constexpr size_t MSG_ORD   = m3::nextlog2<MSG_SIZE>::val;

    enum Operation {
        SIGVITAL,
        KCREATEVPE,
        KEXIT,
        MHTGET,
        MHTGETTRY, // TODO - necessary?
        MHTGETLOCKING, // TODO
        MHTPUT,
        MHTPUTUNLOCKING, // TODO
        MHTLOCK, // Not used yet
        MHTUNLOCK,
        MHTREMOVE, // TODO
        MHTRESERVE,
        MHTRELEASE,
        MEMBERUPDATE,
        PARTITIONMIG,
        CREATESESSFWD,
        CREATESESSRESP,
        CREATESESSFAIL,
        EXCHANGEOSESS,
        EXCHANGEOSESSREPLY,
        EXCHANGEOSESSACK,
        REMOVECHILDCAPPTR,
        RECVBUFISATTACHED,
        RECVBUFATTACHED,
        ANNOUNCESRV,
        REVOKE,
        REVOKEFINISH,
        SHUTDOWNREQUEST,
        SHUTDOWN,
        ANNOUNCEKRNLS,
        CONNECT,
        REPLYKRNLC,
        STARTAPPS,
        COUNT
    };

    enum OpStage : uint8_t {
        KREQUEST,
        KREPLY,
        KFORWARD
    };

    static Kernelcalls &get() {
        return _inst;
    }

private:
    explicit Kernelcalls() {
#if defined(__host__)
        if(!Config::get().is_kernel())
            init(DTU::get().ep_regs());
#endif
    }

public:
    void sigvital(KPE* kernel, int creatorThread, m3::Errors::Code err);
    m3::Errors::Code kcreatevpe(KPE* kernel, OpStage stage, m3::String&& name, const char* core, int tid);
    void kexit(KPE* kernel, int exitcode);

    void mhtRequest(KPE* kernel, mht_key_t mht_key, Operation op);
    void mhtForward(KPE* kernel, int tid, membership_entry::krnl_id_t dest, mht_key_t mht_key, Operation op);
    inline void mhtget(KPE* kernel, mht_key_t mht_key) {
        mhtRequest(kernel, mht_key, MHTGET);
    }
    void mhtgetLocking(KPE* kernel, mht_key_t mht_key);
    void mhtgetReply(KPE* kernel, int tid, const MHTItem& result);
    // Note: the normal put has no guarantees, that the operation succeeds
    void mhtput(KPE* kernel, MHTItem&& input);
    void mhtputUnlocking(KPE* kernel, MHTItem&& input, uint lockHandle);

    inline void mhtlock(KPE* kernel, mht_key_t mht_key) {
        mhtRequest(kernel, mht_key, MHTLOCK);
    }
    void mhtlockReply(KPE* kernel, int tid, uint lockHndl);

    // Note: unlocking is not acknowledged
    void mhtunlock(KPE* kernel, mht_key_t mht_key, uint lockHandle);

    inline void mhtReserve(KPE* kernel, mht_key_t mht_key) {
        mhtRequest(kernel, mht_key, MHTRESERVE);
    }
    void mhtReserveReply(KPE* kernel, int tid, uint reservationNr);

    // Note: releasing is not acknowledged
    void mhtRelease(KPE* kernel, mht_key_t mht_key, uint reservation);

    void membershipUpdate(KPE *kernel, m3::PEDesc releasedPEs[], uint numPEs,
        membership_entry::krnl_id_t krnl, membership_entry::pe_id_t krnlCore, MembershipFlags flags);

    void migratePartition(KPE *kernel, AutoGateOStream &payload);

    void createSessFwd(KPE *kernel, int vpeID, m3::String &srvname, mht_key_t cap, GateOStream args);
    void createSessResp(KPE *kernel, int vpeID, int tid, m3::Errors::Code res, word_t sess, mht_key_t srvCap);
    void createSessFail(KPE *kernel, mht_key_t cap, mht_key_t srvCap);

    void exchangeOverSession(KPE *kernel, bool obtain, mht_key_t vpe, mht_key_t srvID,
        word_t sessID, m3::CapRngDesc caps, GateOStream &args);
    void exchangeOverSessionReply(KPE *kernel, int tid, m3::Errors::Code res,
        m3::CapRngDesc srvcaps, GateOStream &args);
    void exchangeOverSessionAck(KPE *kernel, int tid, m3::Errors::Code res);

    void removeChildCapPtr(KPE *kernel, DDLCapRngDesc parents, DDLCapRngDesc caps);

    void recvBufisAttached(KPE *kernel, int core, int epid);
    void recvBufAttached(KPE *kernel, int tid, m3::Errors::Code res);

    void announceSrv(KPE *kernel, mht_key_t id, const m3::String &name);

    void revoke(KPE *kernel, mht_key_t capID, mht_key_t parent, mht_key_t originCap);
    void revokeFinish(KPE *kernel, mht_key_t initiator, int awaits, bool includeReply);

    void requestShutdown(KPE *kernel, OpStage stage);
    void shutdown(KPE *kernel, OpStage stage);

    void announceKrnls(KPE *kernel, membership_entry::krnl_id_t exception);
    void connect(membership_entry::krnl_id_t kid, membership_entry::pe_id_t core, OpStage stage,
        membership_entry::krnl_id_t myKid, membership_entry::pe_id_t myCore, int epid,
        uint numPEs, MembershipFlags flags, m3::PEDesc releasedPEs[]);
    void connect(KPE *kernel, OpStage stage, membership_entry::krnl_id_t myKid,
        membership_entry::pe_id_t myCore, int epid);

    void reply(KPE *krnl);

    void startApps(KPE *kernel);


private:
    m3::Errors::Code finish(GateIStream &&reply);

    static Kernelcalls _inst;
};

}
