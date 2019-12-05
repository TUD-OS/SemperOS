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
#include <base/Init.h>
#include <base/Errors.h>
#include <base/benchmark/capbench.h>
#include <thread/ThreadManager.h>

#include "Gate.h"
#include "Kernelcalls.h"
#include "KernelcallHandler.h"
#include "Coordinator.h"

namespace kernel {

INIT_PRIO_KRNLC Kernelcalls Kernelcalls::_inst;

m3::Errors::Code Kernelcalls::finish(GateIStream &&reply) {
    reply >> m3::Errors::last;
    return m3::Errors::last;
}

void Kernelcalls::sigvital(KPE* kernel, int creatorThread, m3::Errors::Code err) {
    KLOG(KRNLC, "sigvital(kernel=" << kernel->core() << ", creatorThread=" << creatorThread <<
        ", err=" << (int)err << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, m3::Errors::Code>()> msg;
    msg << SIGVITAL << creatorThread << err;
    kernel->reply(msg.bytes(), msg.total());
    }

m3::Errors::Code Kernelcalls::kcreatevpe(KPE* kernel, OpStage stage, m3::String&& name, const char* core, int tid) {
    KLOG(KRNLC, "kcreatevpe(kernel=" << kernel->core() << ", stage=" << stage << ", name=" << name << ", core=" << core << ")");
    AutoGateOStream msg(m3::vostreamsize(
            m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, int, size_t, size_t>(),
            name.length(), strlen(core)));
    msg << KCREATEVPE << stage << tid << name << core;
    if(stage == Kernelcalls::OpStage::KREPLY)
        kernel->reply(msg.bytes(), msg.total());
    else
        kernel->sendTo(msg.bytes(), msg.total());

    return m3::Errors::NO_ERROR;
}

void Kernelcalls::kexit(KPE* kernel, int exitcode) {
    KLOG(KRNLC, "kexit(code=" << exitcode << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int>()> msg;
    msg << KEXIT << exitcode;
    kernel->sendTo(msg.bytes(), msg.total());
    return;
}

void Kernelcalls::mhtRequest(KPE* kernel, mht_key_t mht_key, Operation op) {
    int tid = m3::ThreadManager::get().current()->id();
    KLOG(KRNLC, "mhtrequest(kernelcore=" << kernel->core() << ", tid=" << tid <<
            ", mht_key=" << PRINT_HASH(mht_key) << ", op=" << op << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, mht_key_t, int>()> msg;
    msg << op << KREQUEST << tid << mht_key;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::mhtForward(KPE* kernel, int tid, membership_entry::krnl_id_t dest, mht_key_t mht_key, Operation op) {
    KLOG(KRNLC, "mhtforward(kernelcore=" << kernel->core() << ", tid=" << tid <<
            ", dest=" << dest << ", mht_key=" << PRINT_HASH(mht_key) << ", op=" << op << ")");
    // forward the request and inform the other kernel about this
    // TODO
    // this message has to arrive at the requesting kernel before
    // the other kernel responds to the actual request. Currently this is not guaranteed
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage,
            membership_entry::krnl_id_t, int, mht_key_t>()> msg;
    msg << op << KFORWARD << tid << dest << mht_key;
    kernel->sendTo(msg.bytes(), msg.total());

    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, mht_key_t, int>()> msgFwd;
    msgFwd << op << KREQUEST << tid << mht_key;
    kernel->forwardTo(msgFwd.bytes(), msgFwd.total(), dest);
}

void Kernelcalls::mhtgetLocking(KPE* kernel, mht_key_t mht_key) {
    int tid = m3::ThreadManager::get().current()->id();
    KLOG(KRNLC, "mhtgetLocking(kernelcore=" << kernel->core() << ", tid=" << tid << ", mht_key=" << PRINT_HASH(mht_key) << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, mht_key_t, int>()> msg;
    msg << MHTGETLOCKING << KREQUEST << tid << mht_key;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::mhtgetReply(KPE* kernel, int tid, const MHTItem& result) {
    KLOG(KRNLC, "mhtgetReply(kernelcore=" << kernel->core() << ", tid=" << tid << ", "
            "result.key=" << PRINT_HASH(result.getKey()) << ", result.length=" << result.getLength() << ")");
    AutoGateOStream msg(m3::vostreamsize(
            m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, int>(),
            result.serializedSize()));
    msg << MHTGET << KREPLY << tid;
    result.serialize(msg);
    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::mhtput(KPE* kernel, MHTItem&& input) {
    KLOG(KRNLC, "mhtput(kernelcore=" << kernel->core() << ", "
            "input.key=" << PRINT_HASH(input.getKey()) << ", input.length=" << input.getLength() << ")");
    AutoGateOStream msg(m3::vostreamsize(
            m3::ostreamsize<Kernelcalls::Operation>(),
            input.serializedSize()));
    msg << MHTPUT;
    input.serialize(msg);
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::mhtputUnlocking(KPE* kernel, MHTItem&& input, uint lockHandle) {
    KLOG(KRNLC, "mhtputUnlocking(kernelcore=" << kernel->core() << ", "
        "input.key=" << PRINT_HASH(input.getKey()) << ", input.length=" << input.getLength() <<
        ", lockHandle=" << lockHandle << ")");
    AutoGateOStream msg(m3::vostreamsize(
            m3::ostreamsize<Kernelcalls::Operation, uint>(),
            input.serializedSize()));
    msg << MHTPUT << lockHandle;
    input.serialize(msg);
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::mhtlockReply(KPE* kernel, int tid, uint lockHndl) {
    KLOG(KRNLC, "mhtlockReply(kernelcore=" << kernel->core() << ", tid=" << tid << ", lockHndl=" << lockHndl << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, int, uint>()> msg;
    msg << MHTLOCK << KREPLY << tid << lockHndl;
    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::mhtunlock(KPE* kernel, mht_key_t mht_key, uint lockHandle) {
    KLOG(KRNLC, "mhtunlock(kernelcore=" << kernel->core() << ", mht_key="
            << PRINT_HASH(mht_key) << ", lockHndl=" << lockHandle << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, mht_key_t, uint>()> msg;
    msg << MHTUNLOCK << mht_key << lockHandle;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::mhtReserveReply(KPE* kernel, int tid, uint reservationNr) {
    KLOG(KRNLC, "mhtReserveReply(kernelcore=" << kernel->core() << ", tid=" << tid <<
            ", reservationNr=" << reservationNr << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, Kernelcalls::OpStage, int, uint>()> msg;
    msg << MHTRESERVE << KREPLY << tid << reservationNr;
    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::mhtRelease(KPE* kernel, mht_key_t mht_key, uint reservation) {
    KLOG(KRNLC, "mhtrelease(kernelcore=" << kernel->core() << ", mht_key="
            << PRINT_HASH(mht_key) << ", reservation=" << reservation << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, mht_key_t, uint>()> msg;
    msg << MHTRELEASE << mht_key << reservation;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::membershipUpdate(KPE *kernel, m3::PEDesc releasedPEs[], uint numPEs,
    membership_entry::krnl_id_t krnl, membership_entry::pe_id_t krnlCore, MembershipFlags flags) {
    KLOG(KRNLC, "membershipUpdate(kernelcore=" << kernel->core() << ", pes=[...], numPEs="
        << numPEs << ", kernel=" << krnl << ", kernelCore=" << krnlCore << ", flags=" << (int)flags << ")");
    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation, uint, size_t, membership_entry::krnl_id_t, MembershipFlags>(),
        numPEs * m3::ostreamsize<m3::PEDesc::value_t>()));
    msg << MEMBERUPDATE << krnl << krnlCore << flags << numPEs;
    for(size_t i = 0; i < numPEs; i++) {
        msg << releasedPEs[i].value();
    }
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::migratePartition(KPE *kernel, AutoGateOStream &payload) {
    KLOG(KRNLC, "migratePartition(kernelcore=" << kernel->core() << ", size=" << payload.total() << ")");
    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation>(),
        payload.total()));
    msg << PARTITIONMIG;
    msg.put(payload);
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::createSessFwd(KPE *kernel, int vpeID, m3::String &srvname, mht_key_t cap, GateOStream args) {
    KLOG(KRNLC, "createSessFwd(kernelcore=" << kernel->core() << ", vpeID=" << vpeID << ", srvname=" <<
        srvname << ", cap=" << PRINT_HASH(cap) << ", argsSize=" << args.total() << ")");
    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation, int, size_t, mht_key_t, int>(),
        srvname.length(), args.total()));
    int tid = m3::ThreadManager::get().current()->id();
    msg << CREATESESSFWD << vpeID << srvname << cap << tid;
    msg.put(args);
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::createSessResp(KPE *kernel, int vpeID, int tid, m3::Errors::Code res, word_t sess,
    mht_key_t srvCap) {
    KLOG(KRNLC, "createSessResp(kernelcore=" << kernel->core() << ", vpeID=" << vpeID << ", tid=" <<
        tid << ", res=" << res << ", sess=" << sess << ", srvCap=" << PRINT_HASH(srvCap) << ")");
    if(res == m3::Errors::NO_ERROR){
        StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, int, m3::Errors::Code,
            word_t, mht_key_t>()> msg;
        msg << CREATESESSRESP << vpeID << tid << res << sess << srvCap;
        kernel->reply(msg.bytes(), msg.total());
    }
    else {
        StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, int, m3::Errors::Code>()> msg;
        msg << CREATESESSRESP << vpeID << tid << res;
        kernel->reply(msg.bytes(), msg.total());
    }
}

void Kernelcalls::createSessFail(KPE *kernel, mht_key_t cap, mht_key_t srvCap) {
    KLOG(KRNLC, "createSessFail(kernelcore=" << kernel->core() << ", cap=" << PRINT_HASH(cap) <<
        ", srvCap=" << PRINT_HASH(srvCap) << ")");
    StaticGateOStream<m3::ostreamsize<mht_key_t, mht_key_t>()> msg;
    msg << cap << srvCap;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::exchangeOverSession(KPE *kernel, bool obtain, mht_key_t vpe, mht_key_t srvID,
    word_t sessID, m3::CapRngDesc caps, GateOStream &args) {
    KLOG(KRNLC, "exchangeOverSession(kernelcore=" << kernel->core() << ", obtain=" <<
        (obtain ? "yes" : "no") << ", vpe=" << PRINT_HASH(vpe) << ", srvID=" <<
        PRINT_HASH(srvID) << ", sessID=" << sessID << ", caps=" << caps << ")");
    CAP_BENCH_TRACE_X_S(KERNEL_OBT_TO_RKERNEL);
    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation, int, bool, mht_key_t, mht_key_t, word_t,
        m3::CapRngDesc::Type, capsel_t, uint>(),
        args.total()));
    int tid = m3::ThreadManager::get().current()->id();
    msg << EXCHANGEOSESS << tid << obtain << vpe << srvID << sessID << caps.type() <<
        caps.start() << caps.count();
    msg.put(args);
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::exchangeOverSessionReply(KPE *kernel, int tid, m3::Errors::Code res,
    m3::CapRngDesc srvcaps, GateOStream &args) {
    KLOG(KRNLC, "exchangeOverSessionReply(kernelcore=" << kernel->core() << ", tid=" << tid <<
        ", res=" << res << ", srvcaps=" << srvcaps << ")");
    CAP_BENCH_TRACE_X_F(KERNEL_OBT_FROM_RKERNEL);
    if(res == m3::Errors::NO_ERROR){
        AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<Kernelcalls::Operation, int, int,
            m3::Errors::Code, m3::CapRngDesc::Type, capsel_t, uint>(), args.total()));
        int mytid = m3::ThreadManager::get().current()->id();
        msg << EXCHANGEOSESSREPLY << tid << res << mytid << srvcaps.type() <<
            srvcaps.start() << srvcaps.count();
        if(args.total())
            msg.put(args);
        kernel->reply(msg.bytes(), msg.total());
    }
    else {
        StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, m3::Errors::Code>()> msg;
        msg << EXCHANGEOSESSREPLY << tid << res;
        kernel->reply(msg.bytes(), msg.total());
    }
}

void Kernelcalls::exchangeOverSessionAck(KPE *kernel, int tid, m3::Errors::Code res) {
    KLOG(KRNLC, "exchangeOverSessionAck(kernelcore=" << kernel->core() << ", tid=" << tid <<
        ", res=" << res << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, m3::Errors::Code>()> msg;
    msg << EXCHANGEOSESSACK << tid << res;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::removeChildCapPtr(KPE *kernel, DDLCapRngDesc parents, DDLCapRngDesc caps) {
    KLOG(KRNLC, "removeChildCaps(kernelcore=" << kernel->core() << ", parentstart=" <<
        PRINT_HASH(parents.start) << ", count=" << parents.count << ", capsstart=" <<
        PRINT_HASH(caps.start) << ", count=" << caps.count << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, mht_key_t, uint, mht_key_t,
        uint>()> msg;
    msg << REMOVECHILDCAPPTR << parents.start << parents.count << caps.start << caps.count;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::recvBufisAttached(KPE *kernel, int core, int epid) {
    KLOG(KRNLC, "recvBufAttached(kernelcore=" << kernel->core() << ", core=" <<
        core << ", epid=" << epid << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, int, int>()> msg;
    int mytid = m3::ThreadManager::get().current()->id();
    msg << RECVBUFISATTACHED << mytid << core << epid;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::recvBufAttached(KPE *kernel, int tid, m3::Errors::Code res) {
    KLOG(KRNLC, "recvBufAttached(kernelcore=" << kernel->core() << ", tid=" << tid <<
        ", res=" << res << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, int, m3::Errors::Code>()> msg;
    msg << RECVBUFATTACHED<< tid << res;
    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::announceSrv(KPE *kernel, mht_key_t id, const m3::String &name) {
    KLOG(KRNLC, "announceSrv(kernelcore=" << kernel->core() << ", id=" << PRINT_HASH(id) <<
        ", name=" << name << ")");
    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation, size_t, mht_key_t>(),
        name.length()));
    msg << ANNOUNCESRV << id << name;
    kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::revoke(KPE *kernel, mht_key_t capID, mht_key_t parent, mht_key_t originCap) {
    KLOG(KRNLC, "revoke(kernelcore=" << kernel->core() << ", capID=" << PRINT_HASH(capID) <<
        ", parent=" << PRINT_HASH(parent) << ", originCap=" << PRINT_HASH(capID) << ")");
    CAP_BENCH_TRACE_X_S(KERNEL_REV_TO_RKERNEL);
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, mht_key_t, mht_key_t, mht_key_t>()> msg;
    msg << REVOKE << capID << parent << originCap;
    kernel->sendRevocationTo(msg.bytes(), msg.total());
}

void Kernelcalls::revokeFinish(KPE *kernel, mht_key_t initiator, int awaits, bool includeReply) {
    KLOG(KRNLC, "revokeFinish(kernelcore=" << kernel->core() << ", initiator=" << PRINT_HASH(initiator) <<
        ", addedAwaits=" << awaits << ", includeReply=" << (includeReply ? "y" : "n") << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, mht_key_t, int, bool>()> msg;
    msg << REVOKEFINISH << initiator << awaits << includeReply;
    kernel->sendRevocationTo(msg.bytes(), msg.total());
}

void Kernelcalls::requestShutdown(KPE *kernel, OpStage stage) {
    KLOG(KRNLC, "requestShutdown(kernelcore=" << kernel->core() << ", stage=" <<
        (stage == OpStage::KREQUEST ? "reque" : "reply") << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, OpStage>()> msg;
    msg << SHUTDOWNREQUEST << stage;
    if(stage == OpStage::KREPLY) {
        Coordinator::get().shutdownRequests--;
        kernel->reply(msg.bytes(), msg.total());
    }
    else
        kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::shutdown(KPE *kernel, OpStage stage) {
    KLOG(KRNLC, "shutdown(kernelcore=" << kernel->core() << ", stage=" <<
        (stage == OpStage::KREQUEST ? "reque" : "reply") << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, OpStage>()> msg;
    msg << SHUTDOWN << stage;
    if(stage == Kernelcalls::OpStage::KREPLY)
        kernel->reply(msg.bytes(), msg.total());
    else
        kernel->sendTo(msg.bytes(), msg.total());
}

void Kernelcalls::announceKrnls(KPE* kernel, membership_entry::krnl_id_t exception) {
    KLOG(KRNLC, "announceKrnls(kernelcore=" << kernel->core() << ", exception=" << exception << ")");
    Coordinator &coord = Coordinator::get();
    uint amount = coord._kpes.size();
    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation, uint>(),
        m3::ostreamsize<membership_entry::krnl_id_t, membership_entry::pe_id_t>() * amount));
    msg << ANNOUNCEKRNLS << amount;
    for(auto it = coord._kpes.begin(); it != coord._kpes.end(); it++)
        if(it->val->id() != exception)
            msg << static_cast<membership_entry::krnl_id_t>(it->val->id()) <<
                static_cast<membership_entry::pe_id_t>( it->val->core());

    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::connect(membership_entry::krnl_id_t kid, membership_entry::pe_id_t core,
    OpStage stage, membership_entry::krnl_id_t myKid, membership_entry::pe_id_t myCore, int epid,
    uint numPEs, MembershipFlags flags, m3::PEDesc releasedPEs[]) {
    KLOG(KRNLC, "connect(kernelcore=" << core << ", stage=" <<
        (stage == OpStage::KREQUEST ? "reque" : "reply") << ", epid=" << epid <<
        ", numPEs=" << numPEs << ")");

    KernelcallHandler::ConnectionRequest *req = new KernelcallHandler::ConnectionRequest(kid, epid);
    KernelcallHandler::get().addConnectionReq(req);

    AutoGateOStream msg(m3::vostreamsize(
        m3::ostreamsize<Kernelcalls::Operation, OpStage, membership_entry::krnl_id_t,
        membership_entry::pe_id_t, int, MembershipFlags, uint>(),
        numPEs * m3::ostreamsize<m3::PEDesc::value_t>()));
    msg << CONNECT << stage << myKid << myCore << epid << flags << numPEs;
    for(size_t i = 0; i < numPEs; i++)
        msg << releasedPEs[i].value();

    KLOG(KPES, "Sending " << msg.total() << "B to kernel #" << kid << " on core #" << core);
    // TODO
    // messages larger than the receive buffer should be split
    assert(msg.total() + m3::DTU::HEADER_SIZE < MSG_SIZE);
    DTU::get().send_to(VPEDesc(core, kid), DTU::SETUP_EP, myKid, msg.bytes(), msg.total(), myKid, epid);
}

void Kernelcalls::connect(KPE *kernel, OpStage stage, membership_entry::krnl_id_t myKid,
    membership_entry::pe_id_t myCore, int epid) {
    KLOG(KRNLC, "connect(kernelcore=" << kernel->core() << ", stage=" <<
        (stage == OpStage::KREQUEST ? "reque" : "reply") << ", epid=" << epid << ")");

    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation, OpStage, membership_entry::krnl_id_t,
        membership_entry::pe_id_t, int>()> msg;
    msg << CONNECT << stage << myKid << myCore << epid;

    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::reply(KPE* kernel) {
    KLOG(KRNLC, "reply(kernel=" << kernel->core() << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation>()> msg;
    msg << REPLYKRNLC;
    kernel->reply(msg.bytes(), msg.total());
}

void Kernelcalls::startApps(KPE* kernel) {
    KLOG(KRNLC, "startApps(kernel=" << kernel->core() << ")");
    StaticGateOStream<m3::ostreamsize<Kernelcalls::Operation>()> msg;
    msg << STARTAPPS;
    kernel->sendTo(msg.bytes(), msg.total());
}

}
