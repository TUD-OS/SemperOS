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
#include <base/Heap.h>
#include <base/benchmark/capbench.h>
#include <thread/ThreadManager.h>

#include "KernelcallHandler.h"
#include "Kernelcalls.h"
#include "pes/PEManager.h"
#include "Coordinator.h"
#include "ddl/MHTInstance.h"
#include "pes/VPE.h"
#include "pes/KPE.h"
#include "com/RecvBufs.h"

namespace kernel {

INIT_PRIO_USER(3) KernelcallHandler KernelcallHandler::_inst;

#define LOG_KRNL(kpe, expr) \
    KLOG(KRNLC, (kpe)->name() << " #" << (kpe)->id() << "@" << m3::fmt((kpe)->core(), "X") << " issued: " << expr)

#define LOG_ANONYM(id, expr) \
    KLOG(KRNLC, "Kernel #" << id << " issued: " << expr)

KernelcallHandler::KernelcallHandler()
    : _rcvgate{RecvGate(DTU::KRNLC_EP, nullptr), RecvGate(DTU::KRNLC_EP + 1, nullptr),
        RecvGate(DTU::KRNLC_EP + 2, nullptr), RecvGate(DTU::KRNLC_EP + 3, nullptr),
        RecvGate(DTU::KRNLC_EP + 4, nullptr), RecvGate(DTU::KRNLC_EP + 5, nullptr),
        RecvGate(DTU::KRNLC_EP + 6, nullptr), RecvGate(DTU::KRNLC_EP + 7, nullptr)} {
    for(uint i = 0; i < KRNLC_SLOTS; i++)
        _epOccup[i] = -1;

    #if !defined(__t2__)
    // configure receive buffer (we need to do that manually in the kernel)
    int buford = m3::getnextlog2(m3::DTU::MAX_MSG_SLOTS) + Kernelcalls::MSG_ORD;
    size_t bufsize = static_cast<size_t>(1) << buford;
    for(uint i = 0; i < DTU::KRNLC_GATES + 1; i++)
        DTU::get().config_recv_local(epid(i),reinterpret_cast<uintptr_t>(
            new uint8_t[bufsize]), buford, Kernelcalls::MSG_ORD, 0);
#endif

//    VPE::self()._eps->set(DTU::KRNLC_EP);

    // add a dummy item to workloop; we handle everything manually anyway
    // but one item is needed to not stop immediately
    m3::env()->workloop()->add(nullptr, false);

    add_operation(Kernelcalls::SIGVITAL, &KernelcallHandler::sigvital);
    add_operation(Kernelcalls::KCREATEVPE, &KernelcallHandler::kcreatevpe);
    add_operation(Kernelcalls::KEXIT, &KernelcallHandler::kexit);
    add_operation(Kernelcalls::MHTGET, &KernelcallHandler::mhtget);
    add_operation(Kernelcalls::MHTPUT, &KernelcallHandler::mhtput);
    add_operation(Kernelcalls::MHTPUTUNLOCKING, &KernelcallHandler::mhtputUnlocking);
    add_operation(Kernelcalls::MHTLOCK, &KernelcallHandler::mhtlock);
    add_operation(Kernelcalls::MHTUNLOCK, &KernelcallHandler::mhtunlock);
    add_operation(Kernelcalls::MHTRESERVE, &KernelcallHandler::mhtreserve);
    add_operation(Kernelcalls::MHTRELEASE, &KernelcallHandler::mhtrelease);
    add_operation(Kernelcalls::MEMBERUPDATE, &KernelcallHandler::membershipUpdate);
    add_operation(Kernelcalls::PARTITIONMIG, &KernelcallHandler::migratePartition);
    add_operation(Kernelcalls::CREATESESSFWD, &KernelcallHandler::createSessFwd);
    add_operation(Kernelcalls::CREATESESSRESP, &KernelcallHandler::createSessResp);
    add_operation(Kernelcalls::CREATESESSFAIL, &KernelcallHandler::createSessFail);
    add_operation(Kernelcalls::EXCHANGEOSESS, &KernelcallHandler::exchangeOverSession);
    add_operation(Kernelcalls::EXCHANGEOSESSREPLY, &KernelcallHandler::exchangeOverSessionReply);
    add_operation(Kernelcalls::REMOVECHILDCAPPTR, &KernelcallHandler::removeChildCapPtr);
    add_operation(Kernelcalls::RECVBUFISATTACHED, &KernelcallHandler::recvBufisAttached);
    add_operation(Kernelcalls::RECVBUFATTACHED, &KernelcallHandler::recvBufAttached);
    add_operation(Kernelcalls::ANNOUNCESRV, &KernelcallHandler::announceSrv);
    add_operation(Kernelcalls::REVOKE, &KernelcallHandler::revoke);
    add_operation(Kernelcalls::REVOKEFINISH, &KernelcallHandler::revokeFinish);
    add_operation(Kernelcalls::SHUTDOWNREQUEST, &KernelcallHandler::requestShutdown);
    add_operation(Kernelcalls::SHUTDOWN, &KernelcallHandler::shutdown);
    add_operation(Kernelcalls::ANNOUNCEKRNLS, &KernelcallHandler::announceKrnls);
    add_operation(Kernelcalls::CONNECT, &KernelcallHandler::connect);
    add_operation(Kernelcalls::REPLYKRNLC, &KernelcallHandler::reply);
    add_operation(Kernelcalls::STARTAPPS, &KernelcallHandler::startApps);
}

void KernelcallHandler::sigvital(GateIStream& is) {
    // Marks the kernel as active
    int tid;
    m3::Errors::Code err;
    is >> tid >> err;
    LOG_ANONYM(is.label(), "kernelcall::sigvital(creatorThread=" << tid << ", err=" << err << ")");
    m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid), reinterpret_cast<void*>(&err),
        sizeof(m3::Errors::Code));
}

void KernelcallHandler::kexit(GateIStream &is) {
    // Here we should shut down ourself -- not ready yet
    KPE *kpe = Coordinator::get().getKPE(is.label());
    int exitcode;
    is >> exitcode;
    LOG_KRNL(kpe, "kernelcall::kexit(" << exitcode << ")");

    // for test purposes just destroy
    PEManager::destroy();

    // if there are no VPEs left, we can stop everything
//    if(PEManager::get().used() == 0) {
//        PEManager::destroy();
//        // ensure that the workloop stops
//        _rcvbuf.detach();
//        _srvrcvbuf.detach();
//    }
//    // if there are only daemons left, start the shutdown-procedure
//    else if(PEManager::get().used() == PEManager::get().daemons())
//        PEManager::shutdown();
    }

void KernelcallHandler::kcreatevpe(GateIStream& is) {
    Kernelcalls::OpStage stage;
    is >> stage;
    switch(stage) {
        case Kernelcalls::KREQUEST:
        {
            // try to create a VPE and reply to the requester
            m3::String name, core;
            int tid;
            is >> tid >> name >> core;
            LOG_ANONYM(is.label(), "Kernelcall::kcreatevpe(KREQUEST, name=" << name << ", core=" << core << ", tid=" << tid << ")");
            // TODO: adapt args to new interface
            VPE* nkvpe = nullptr;//PEManager::get().create(m3::Util::move(name), nullptr);
            if(nkvpe != nullptr){
                reply_vmsg(is, Kernelcalls::KCREATEVPE, Kernelcalls::KREPLY, tid, true, (size_t)nkvpe->id());
                KLOG(KRNLC, "Created KVPE");
                // TODO
                // send args accordingly and start the VPE
            }
            else{
                reply_vmsg(is, Kernelcalls::KCREATEVPE, Kernelcalls::KREPLY, tid, false);
                KLOG(KRNLC, "Creating KVPE failed");
            }
            break;
        }
        case Kernelcalls::KREPLY:
        {
            // handle a reply
            int tid;
            is >> tid;
            KPE* kpe = Coordinator::get().getKPE(is.label());
            LOG_KRNL(kpe, "Kernelcall::kcreatevpe(KREPLY, tid=" << tid << ")");
            // callback is KPE::cbCreateVPE()
            kpe->notify(tid, is);
            Coordinator::get().getKPE(is.label())->msg_received();
            break;
        }
        default:
            KLOG(ERR, "Unhandled kcreateVPE response! stage=" << stage);
            Coordinator::get().getKPE(is.label())->msg_received();
            break;
    }
}

void KernelcallHandler::mhtget(GateIStream& is) {
    Kernelcalls::OpStage stage;
    is >> stage;
    if(stage == Kernelcalls::KREQUEST){
        int tid;
        mht_key_t mht_key;
        is >> tid >> mht_key ;
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtget(KREQUEST, tid=" << tid << ", mht_key=" << PRINT_HASH(mht_key) << ")");
        const MHTItem& result = MHTInstance::getInstance().localGet(mht_key, false);
        Kernelcalls::get().mhtgetReply(Coordinator::get().getKPE(is.label()), tid, result);
    }
    else { // KREPLY
        int tid;
        is >> tid;
        MHTItem result(is);
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtget(KREPLY, mht_key=" << PRINT_HASH(result.getKey()) << ", tid=" << tid << ", len=" << result.getLength() << ")");
        Coordinator::get().getKPE(is.label())->msg_received();
        m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid), &result, sizeof(MHTItem));
    }
}

void KernelcallHandler::mhtgetLocking(GateIStream&) {
    KLOG(ERR, "mhtgetlocking NOT implemented");
    //TODO
}

void KernelcallHandler::mhtput(GateIStream& is) {
    // Deserializing constructor
    MHTItem input(is);
    LOG_KRNL(Coordinator::get().getKPE(is.label()),
            "kernelcall::mhtput(KREQUEST, input.mht_key=" << PRINT_HASH(input.getKey()) << ", input.len=" << input.getLength() << ")");
    MHTInstance::getInstance().put(m3::Util::move(input));
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::mhtputUnlocking(GateIStream& is) {
    uint lockHandle;
    is >> lockHandle;
    // Deserializing constructor
    MHTItem input(is);
    LOG_KRNL(Coordinator::get().getKPE(is.label()),
        "kernelcall::mhtputUnlocking(KREQUEST, input.mht_key=" << PRINT_HASH(input.getKey()) <<
        ", input.len=" << input.getLength() << ", lockHandle=" << lockHandle << ")");
    MHTInstance::getInstance().putUnlocking(m3::Util::move(input), lockHandle);
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::mhtlock(GateIStream& is) {
    Kernelcalls::OpStage stage;
    int tid;
    is >> stage >> tid;
    switch(stage) {
    case Kernelcalls::KREQUEST:
    {
        mht_key_t mht_key;
        is >> mht_key;
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtlock(KREQUEST, tid=" << tid << ", mht_key=" << PRINT_HASH(mht_key) << ")");

        // check if this kernel really maintains the partition containing the key
        if(MHTInstance::getInstance().responsibleMember(mht_key) == Coordinator::get().kid()) {
            uint lockHandle = MHTInstance::getInstance().lockLocal(mht_key);
            Kernelcalls::get().mhtlockReply(Coordinator::get().getKPE(is.label()), tid, lockHandle);
            // TODO
            // if the key has been migrated return the new maintainer of the partition to the requestor (Li's algorithm)
//            if(MHTInstance::getInstance().responsibleMember(mht_key) != Coordinator::get().kid()) {
//                Kernelcalls::get().mhtForward(Coordinator::get().getKPE(is.label), tid,
//                        MHTInstance::getInstance().responsibleMember(mht_key),
//                        mht_key, Kernelcalls::MHTLOCK);
//                return;
//            }
        } else {
            // return the new maintainer of the partition to the requestor and forward the request to the new maintainer
            Kernelcalls::get().mhtForward(Coordinator::get().getKPE(is.label()),
                    tid, MHTInstance::getInstance().responsibleMember(mht_key), mht_key, Kernelcalls::MHTLOCK);
        }
        break;
    }
    case Kernelcalls::KREPLY:
    {
        uint lockHandle;
        is >> lockHandle;
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtlock(KREPLY, tid=" << tid << ", lockHandle=" << lockHandle << ")");
        m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid), &lockHandle, sizeof(uint));
        Coordinator::get().getKPE(is.label())->msg_received();
        break;
    }
    case Kernelcalls::KFORWARD:
    {
        // The requested key is not maintained by the kernel we asked.
        // Update membership table accordingly
        membership_entry::krnl_id_t krnlId;
        mht_key_t mht_key;
        is >> krnlId >> mht_key;
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtlock(KFORWARD, tid=" << tid << ", krnlId=" << krnlId << ", "
                "mht_key=" << PRINT_HASH(mht_key) << ")");

        Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
        // only update the membership table if we already know the responsible kernel
        KPE *destKrnl = Coordinator::get().tryGetKPE(krnlId);
        if(destKrnl == nullptr)
            destKrnl = MHTInstance::getInstance().getMigrationDestination(HashUtil::hashToPeId(mht_key));
        if(destKrnl != nullptr)
            MHTInstance::getInstance().updateMembership(HashUtil::hashToPeId(mht_key), krnlId,
                destKrnl->core(), 1, NOCHANGE, true);
        break;
    }
    }
}

void KernelcallHandler::mhtunlock(GateIStream& is) {
    mht_key_t mht_key;
    uint lockHandle;
    is >> mht_key >> lockHandle;
    LOG_KRNL(Coordinator::get().getKPE(is.label()),
            "kernelcall::mhtunlock(mht_key=" << PRINT_HASH(mht_key) << ", lockHandle=" << lockHandle << ")");
    MHTInstance::getInstance().unlock(mht_key, lockHandle);
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::mhtreserve(GateIStream& is) {
    Kernelcalls::OpStage stage;
    int tid;
    is >> stage >> tid;
    switch(stage) {
    case Kernelcalls::KREQUEST:
    {
        mht_key_t mht_key;
        is >> mht_key;
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtreserve(KREQUEST, tid = " << tid << ", mht_key=" << PRINT_HASH(mht_key) << ")");
        // Forwarding is not yet allowed
        assert(MHTInstance::getInstance().keyLocality(mht_key));
        Kernelcalls::get().mhtReserveReply(Coordinator::get().getKPE(is.label()), tid,
                MHTInstance::getInstance().reserve(mht_key));
        break;
    }
    case Kernelcalls::KREPLY:
    {
        uint reservationNr;
        is >> reservationNr;
        LOG_KRNL(Coordinator::get().getKPE(is.label()),
                "kernelcall::mhtreserve(KREPLY, tid = " << tid << ", reservationNr=" << reservationNr << ")");
        m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid), &reservationNr, sizeof(uint));
        Coordinator::get().getKPE(is.label())->msg_received();
        break;
    }
    case Kernelcalls::KFORWARD:
        PANIC("Not implemented");
        break;
    }
}

void KernelcallHandler::mhtrelease(GateIStream& is) {
    mht_key_t mht_key;
    uint reservation;
    is >> mht_key >> reservation;
    LOG_KRNL(Coordinator::get().getKPE(is.label()),
            "kernelcall::mhtrelease(mht_key=" << PRINT_HASH(mht_key) << ", reservation=" << reservation << ")");
    MHTInstance::getInstance().release(mht_key, reservation);
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::membershipUpdate(GateIStream &is) {
    membership_entry::krnl_id_t krnlId;
    membership_entry::pe_id_t krnlCore;
    MembershipFlags flags;
    uint numPEs;
    is >> krnlId >> krnlCore >> flags >> numPEs;
    LOG_KRNL(Coordinator::get().getKPE(is.label()),
        "kernelcall::membershipUpdate(pes=[...], numPEs=" << numPEs << ", kernel=" << krnlId <<
        ", krnlCore=" << krnlCore << ", flags=" << (int)flags << ")");

    m3::PEDesc* newPEs = new m3::PEDesc[numPEs];
    for(size_t i = 0; i < numPEs; i++) {
        m3::PEDesc::value_t desc;
        is >> desc;
        newPEs[i] = m3::PEDesc(desc);
    }

    // New kernels can only be added when we know which EP we should connect it to, so we wait
    // for the new kernel to connect to us and provide this information.
    if(!Coordinator::get().tryGetKPE(krnlId)) {
        KLOG(KRNLC, "Ignoring membership update for unknown kernel. Waiting for connection request.");
        return;
    }

    MHTInstance::getInstance().updateMembership(newPEs, numPEs, krnlId, krnlCore, flags, false);
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::migratePartition(GateIStream &is) {
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::migratePartition()");
    MHTInstance::getInstance().receivePartitions(is);
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::createSessFwd(GateIStream &is) {
    m3::String srvname;
    mht_key_t cap;
    int tid, vpeID;
    is >> vpeID >> srvname >> cap >> tid;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::createSessFwd(vpeID=" << vpeID <<
        ", srvname=" << srvname << ", cap=" << PRINT_HASH(cap) << ")");
    Service *srv = ServiceList::get().find(srvname);
    if(srv == nullptr || srv->closing) {
        Kernelcalls::get().createSessResp(Coordinator::get().getKPE(is.label()), vpeID, tid,
            m3::Errors::INV_ARGS, 0, 0);
        return;
    }
    else {
        // send request to service
        label_t sender = is.label();
        m3::Reference<Service> rsrv(srv);

        RecvGate *rgate = new RecvGate(SyscallHandler::get().srvepid(), nullptr);
        rgate->subscribe([this, rgate, rsrv, cap, tid, vpeID, sender] (GateIStream &reply, m3::Subscriber<GateIStream&> *s) {
            m3::Reference<Service> srvcpy = rsrv;
            srvcpy->received_reply();

            m3::Errors::Code res;
            reply >> res;
            LOG_KRNL(Coordinator::get().getKPE(sender),"createSessFwd-cb(res=" << res << ")");
            if(res != m3::Errors::NO_ERROR) {
                Kernelcalls::get().createSessResp(Coordinator::get().getKPE(sender), vpeID, tid,
                    m3::Errors::INV_ARGS, 0, 0);
            }
            else {
                word_t sess;
                reply >> sess;
                Capability *srvcap = rsrv->vpe().objcaps().get(rsrv->selector(), Capability::SERVICE);
                assert(srvcap != nullptr);
                // add child representing the session with the remote VPE
                static_cast<ServiceCapability*>(srvcap)->addChild(cap);
                // add a reference to service for the remote session
                srvcpy->add_ref();

                Kernelcalls::get().createSessResp(Coordinator::get().getKPE(sender), vpeID, tid,
                    m3::Errors::NO_ERROR, sess, srvcap->id());
            }

            // unsubscribe will delete the lambda
            RecvGate *rgatecpy = rgate;
            rgate->unsubscribe(s);
            delete rgatecpy;
        });

        AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<m3::KIF::Service::Command>(), is.remaining()));
        msg << m3::KIF::Service::OPEN;
        msg.put(is);
        srv->send(rgate, msg.bytes(), msg.total(), msg.is_on_heap());
        msg.claim();
    }
}

void KernelcallHandler::createSessResp(GateIStream &is) {
    // Note: We have a race condition here if kernel threads are not scheduled
    //       round-robin and it becomes possible to have two message handling
    //       threads running after another since the second would override the
    //       message of the first.
    int vpeID, tid, res;
    is >> vpeID >> tid >> res;
    label_t sender = is.label();
    LOG_KRNL(Coordinator::get().getKPE(sender), "kernelcall::createSessResp(vpeID=" << vpeID <<
        ", tid=" << tid << ", res=" << res << ")");

    if(res != m3::Errors::INV_ARGS) {
        size_t sizeResult = m3::vostreamsize(m3::ostreamsize<label_t, m3::Errors::Code>(), is.remaining());
        void *resultBuf = m3::Heap::alloc(sizeResult);
        if(!resultBuf)
            PANIC("Not enough memory for result of service lookup");
        GateOStream *data = new GateOStream(static_cast<unsigned char*>(resultBuf), sizeResult);
        *data << sender << res;
        data->put(is);
        PEManager::get().vpe(vpeID).srvLookupResult(data);
    }

    VPE &vpe = PEManager::get().vpe(vpeID);
    if(!vpe.sessRespArrived()) {
        m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid));
    }
    Coordinator::get().getKPE(is.label())->msg_received();
}

void KernelcallHandler::createSessFail(GateIStream &is) {
    mht_key_t cap, srvCap;
    is >> cap >> srvCap;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::createSessFail(cap=" <<
        PRINT_HASH(cap) << ", srvCap=" << PRINT_HASH(srvCap) << ")");
    ServiceCapability *srv = MHTInstance::getInstance().get(srvCap).getData<ServiceCapability>();
    srv->removeChild(cap);
    srv->inst->rem_ref();
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::exchangeOverSession(GateIStream &is) {
    int tid;
    bool obtain;
    mht_key_t vpe, srvID;
    word_t sessID;
    m3::CapRngDesc::Type capsType;
    capsel_t capsStart;
    uint capsCount;

    is >> tid >> obtain >> vpe >> srvID >> sessID >> capsType >> capsStart >> capsCount;
    m3::CapRngDesc caps(capsType, capsStart, capsCount);
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::exchangeOverSession(obtain=" <<
        (obtain ? "yes" : "no") << ", vpe=" << PRINT_HASH(vpe) << ", srvID=" << PRINT_HASH(srvID) << ", sessID=" << sessID <<
        ", caps=" << caps << ")");

    const MHTItem &srvItem = MHTInstance::getInstance().get(srvID);
    if(srvItem.data == nullptr) {
        KLOG(ERR, "Service not found");
        StaticGateOStream<0> empty;
        Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(is.label()),
            tid, m3::Errors::INV_ARGS, m3::CapRngDesc(), empty);
        return;
    }

    Service *srv = srvItem.getData<Service>();
    // send request to service
    label_t sender = is.label();
    m3::Reference<Service> rsrv(srv);
    RecvGate *rgate = new RecvGate(SyscallHandler::get().srvepid(), nullptr);

    rgate->subscribe([this, rgate, rsrv, tid, sender, obtain, vpe, caps]
        (GateIStream &reply, m3::Subscriber<GateIStream&> *s) {
        CAP_BENCH_TRACE_X_F(RKERNEL_OBT_FROM_SRV);
        m3::Reference<Service> srvcpy = rsrv;
        srvcpy->received_reply();

        m3::CapRngDesc srvcaps;
        m3::Errors::Code res;
        reply >> res;
        KLOG(KRNLC, "exchangeOverSession-cb: res=" << res);

        if(res == m3::Errors::NO_ERROR) {
            reply >> srvcaps;
            if(srvcaps.type() != caps.type()) {
                KLOG(ERR,"Descriptor types don't match (" <<
                    m3::Errors::INV_ARGS << ")");
                // send error back
                StaticGateOStream<0> empty;
                Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(sender),
                    tid, m3::Errors::INV_ARGS, m3::CapRngDesc(), empty);
                goto finish;
            }
            if((obtain && srvcaps.count() > caps.count()) || (!obtain && caps.count() != srvcaps.count())) {
                KLOG(ERR, "Server gave me invalid CRD (" <<
                    m3::Errors::INV_ARGS << ")");
                StaticGateOStream<0> empty;
                Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(sender),
                    tid, m3::Errors::INV_ARGS, m3::CapRngDesc(), empty);
                goto finish;
            }
            CapTable &srvTbl = srvcaps.type() == m3::CapRngDesc::OBJ ?
                        srvcpy->vpe().objcaps() : srvcpy->vpe().mapcaps();
            if(!obtain && srvTbl.range_unused(srvcaps)) {
                KLOG(ERR, "Invalid destination caps (" << m3::Errors::INV_ARGS << ")");
                StaticGateOStream<0> empty;
                Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(sender),
                    tid, m3::Errors::INV_ARGS, m3::CapRngDesc(), empty);
                goto finish;
            }

            Capability *srvcapsPtr[srvcaps.count()];
            // TODO
            // currently we just ignore obtains for capability slots that are not filled.
            // Not sure wether these are good semantics for obtain

//            // gather parent caps
//            for(capsel_t i = 0; i < srvcaps.count(); i++) {
//                srvcapsPtr[i] = srvTbl.get(srvcaps.start() + i);
//                if(srvcapsPtr[i] == nullptr) {
//                    KLOG(ERR, "Server gave me invalid caps (" << m3::Errors::INV_ARGS <<
//                        "). Caps=" << srvcaps);
//                    StaticGateOStream<0> empty;
//                    Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(sender),
//                        tid, m3::Errors::INV_ARGS, m3::CapRngDesc(), empty);
//                    goto finish;
//                }
//            }
            size_t serializedSize = 0;
            for(capsel_t i = 0; i < srvcaps.count(); i++) {
                srvcapsPtr[i] = srvTbl.get(srvcaps.start() + i);
                if(obtain) {
                    if(srvcapsPtr[i] != nullptr) {
                        // add children to parent caps
                        srvcapsPtr[i]->addChild(
                            HashUtil::structured_hash(HashUtil::hashToPeId(vpe), HashUtil::hashToVpeId(vpe),
                            Capability::capTypeToItemType(srvcapsPtr[i]->type()), i + caps.start()));
                        serializedSize += Capability::serializedSizeTyped(srvcapsPtr[i]);
                    }
                    else {
                        KLOG(KRNLC, "Ignoring obtain for non-existent cap (sel="
                            << (srvcaps.start() + i) << ")");
                        srvcaps = m3::CapRngDesc(srvcaps.type(), srvcaps.start(), i);
                        continue;
                    }
                }
                else {
                    // reserve cap slots so no other operation overtakes them
                    srvTbl.reserve(srvcaps.start() + i);
                }
            }

            // send reply to krnl
            AutoGateOStream remain(m3::vostreamsize(serializedSize + reply.remaining()));
            for(capsel_t i = 0; i < srvcaps.count(); i++) {
                Capability::serializeTyped(remain, srvcapsPtr[i]);
            }
            remain.put(reply);
            Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(sender),
                tid, res, srvcaps, remain);

            // put thread to sleep
            // TODO
            // do we really need to spend a thread on this? for delegate we insert caps into table,
            // for obtain we could send the cap IDs again that have to be removed in failure case,
            // that might be cheaper
            if(!obtain) {
                int mytid = m3::ThreadManager::get().current()->id();
                m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(mytid));
                const unsigned char *ack = m3::ThreadManager::get().get_current_msg();
                m3::Errors::Code res;
                res = *reinterpret_cast<m3::Errors::Code*>(const_cast<unsigned char*>(ack));
                if(res != m3::Errors::NO_ERROR) {
                    for(capsel_t i = 0; i < srvcaps.count(); i++)
                        srvTbl.release(srvcaps.start() + i);
                }
                else {
                    // TODO
                    // delegate: read capability information from stream and insert them into cap table
                    PANIC("Delegate not implemented");
                }
            }
        }
        else {
            KLOG(KRNLC, " Server denied cap-transfer (" << res << ")");
            StaticGateOStream<0> empty;
            Kernelcalls::get().exchangeOverSessionReply(Coordinator::get().getKPE(sender),
                tid, res, m3::CapRngDesc(), empty);
        }

        finish:
            // unsubscribe will delete the lambda
            RecvGate *rgatecpy = rgate;
            rgate->unsubscribe(s);
            delete rgatecpy;
    });

    AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<m3::KIF::Service::Command, word_t,
        m3::CapRngDesc>(), is.remaining()));
    msg << (obtain ? m3::KIF::Service::OBTAIN : m3::KIF::Service::DELEGATE) << sessID << caps.count();
    msg.put(is);
    CAP_BENCH_TRACE_X_S(RKERNEL_OBT_TO_SRV);
    srv->send(rgate, msg.bytes(), msg.total(), msg.is_on_heap());
    msg.claim();
}

void KernelcallHandler::exchangeOverSessionReply(GateIStream &is) {
    int tid;
    m3::Errors::Code res;
    is >> tid >> res;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::exchangeOverSessionReply(tid=" <<
        tid << ", res=" << res << ")");
    if(res == m3::Errors::NO_ERROR) {
        AutoGateOStream reply(is.remaining() + m3::ostreamsize<m3::Errors::Code, unsigned long int>());
        reply << res << is.remaining();
        if(is.remaining())
            reply.put(is);
        m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid),
            const_cast<unsigned char*>(reply.bytes()), reply.total());
    }
    else {
        StaticGateOStream<m3::ostreamsize<unsigned long int, m3::Errors::Code>()> reply;
        reply << res;
        m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid),
            const_cast<unsigned char*>(reply.bytes()), reply.total());
    }
    Coordinator::get().getKPE(is.label())->msg_received();
}

void exchangeOverSessionAck(GateIStream &is) {
    int tid;
    m3::Errors::Code res;
    is >> tid >> res;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::exchangeOverSessionAck(tid=" <<
        tid << ", res=" << res << ")");
    m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid),
        reinterpret_cast<void*>(&res), sizeof(m3::Errors::Code));
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::removeChildCapPtr(GateIStream &is) {
    mht_key_t parentStart;
    mht_key_t childStart;
    uint parentCount, childCount;
    is >> parentStart >> parentCount >> childStart >> childCount;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::removeChildPtr(parentstart=" <<
        PRINT_HASH(parentStart) << ", count= " << parentCount << ", capsstart=" <<
        PRINT_HASH(childStart) << ", count=" << childCount << ")");
    if(parentCount != childCount) {
        KLOG(ERR, "Amount of caps unequal while removing child ptr");
        Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
        return;
    }

    for(uint i = 0; i < parentCount; i ++) {
        const MHTItem &parentItem = MHTInstance::getInstance().get(parentStart + i);
        if(parentItem.data == nullptr) {
            KLOG(ERR, "Can't remove children, parent cap" << PRINT_HASH(parentStart) << " not existent.");
            continue;
        }
        parentItem.getData<Capability>()->removeChildAllTypes(childStart + i);
    }
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::recvBufisAttached(GateIStream &is) {
    int tid, core, epid;
    is >> tid >> core >> epid;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::recvBufisAttached(tid=" <<
        tid << ", core=" << core << ", epid=" << epid << ")");
    if(RecvBufs::is_attached(core, epid)) {
        Kernelcalls::get().recvBufAttached(Coordinator::get().getKPE(is.label()),
            tid, m3::Errors::NO_ERROR);
    }
    else {
        size_t sender = is.label();
        auto callback = [sender, tid, core, epid](bool success, m3::Subscriber<bool> *) {
            EVENT_TRACER_Syscall_activate();
            m3::Errors::Code res = success ? m3::Errors::NO_ERROR : m3::Errors::RECV_GONE;

            LOG_KRNL(Coordinator::get().getKPE(sender), "kernelcall::recvBufisAttached-cb(res=" << res << ")");

            Kernelcalls::get().recvBufAttached(Coordinator::get().getKPE(sender), tid, res);
            if(res != m3::Errors::NO_ERROR)
                KLOG(ERR, (Coordinator::get().getKPE(sender))->name() << "@" <<
                    m3::fmt(Coordinator::get().getKPE(sender)->core(), "X") <<
                    ": activate failed (" << res << ")");

        };
        RecvBufs::subscribe(core, epid, callback);
    }
}

void KernelcallHandler::recvBufAttached(GateIStream &is) {
    int tid;
    m3::Errors::Code res;
    is >> tid >> res;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::recvBufAttached(tid=" <<
        tid << ", res=" << res << ")");
    m3::ThreadManager::get().notify(reinterpret_cast<void*>(tid),
        reinterpret_cast<void*>(&res), sizeof(m3::Errors::Code));
    Coordinator::get().getKPE(is.label())->msg_received();
}

void KernelcallHandler::announceSrv(GateIStream &is) {
    mht_key_t id;
    m3::String name;
    is >> id >> name;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::announceSrv(id=" <<
        PRINT_HASH(id) << ", name=" << name << ")");

    RemoteServiceList::get().add(name, id);
    PEManager::get().start_pending(ServiceList::get(), RemoteServiceList::get());
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::revoke(GateIStream &is) {
    mht_key_t capID, parent, originCap;
    is >> capID >> parent >> originCap;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::revoke(capID=" <<
        PRINT_HASH(capID) << ", parent=" << PRINT_HASH(parent) << ", originCap="
        << PRINT_HASH(originCap) << ")");

    int awaited = 0;
    const MHTItem &capIt = MHTInstance::getInstance().get(capID);
    if(!capIt.isEmpty()) {
        Capability *cap = capIt.getData<Capability>();
        awaited = cap->table()->revoke(cap, capID, originCap);

        KLOG(KRNLC, "Generated " << awaited << " revoke requests");
    }
    else {
        // figure out the capTable to call revoke which checks whether the
        // revocation of this cap is still in flight
        if(HashUtil::hashToType(capID) == ItemType::MAPCAP)
            awaited = PEManager::get().vpe(HashUtil::hashToVpeId(capID)).mapcaps().revoke(
                nullptr, capID, originCap);
        else
            awaited = PEManager::get().vpe(HashUtil::hashToVpeId(capID)).objcaps().revoke(
                nullptr, capID, originCap);
    }
    // check if we can confirm the end of the revocation.
    // Meaning it was either revoking only leaf caps or it has been revoked
    // already or it never existed
    if(!awaited)
        Kernelcalls::get().revokeFinish(Coordinator::get().getKPE(is.label()), parent, -1, true);
    else
        Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::revokeFinish(GateIStream &is) {
    CAP_BENCH_TRACE_X_F(KERNEL_REV_FROM_RKERNEL);
    mht_key_t initiator;
    int awaits;
    bool includeReply;
    is >> initiator >> awaits >> includeReply;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::revokeFinish(initiator=" <<
        PRINT_HASH(initiator) << ", awaits=" << awaits << ", includeReply=" <<
        (includeReply ? "y" : "n") << ")");

    if(includeReply)
        Coordinator::get().getKPE(is.label())->msg_received();

    // find the revocation waiting for this acknowledgement
    Revocation *revoc = RevocationList::get().find(initiator);

    // sum up number of awaited responses to revocation entry
    revoc->awaitedResp += awaits;
    if(revoc->awaitedResp == 0) {
        if(revoc->capID == revoc->origin) {
            // Note: We don't notify our subscribers here since the notified
            // thread will do so.

            // ready to resume the origin thread
            m3::ThreadManager::get().notify(reinterpret_cast<void*>(revoc->tid));
        }
        else {
            revoc->notifySubscribers();
            // inform remote parent
            membership_entry::krnl_id_t parentAuthority =
                MHTInstance::getInstance().responsibleKrnl(HashUtil::hashToPeId(revoc->parent));
            if(parentAuthority != Coordinator::get().kid()) {
                Kernelcalls::get().revokeFinish(Coordinator::get().getKPE(parentAuthority),
                    revoc->parent, -1, false);
            }
            RevocationList::get().remove(revoc->capID);
        }
    }
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
}

void KernelcallHandler::requestShutdown(GateIStream& is) {
    Kernelcalls::OpStage stage;
    is >> stage;
    // TODO
    // shutdown hack, actually there should be no more incoming requests if we
    // deleted that KPE
    if(Coordinator::get().tryGetKPE(is.label()) == nullptr) {
        KLOG(ERR, "Ignoring late requestShutdown of kernel # " << is.label());
        return;
    }
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::requestShutdown(stage=" <<
        (stage == Kernelcalls::OpStage::KREQUEST ? "reque" : "reply") << ")");

    Coordinator &coord = Coordinator::get();
    // kernel 0 coordinates the shutdown
    if(coord.kid() == 0) {
        if(stage == Kernelcalls::OpStage::KREQUEST) {
            if(coord.getKPE(is.label())->getShutdownState() == KPE::ShutdownState::INFLIGHT)
                coord.closingRequests--;
            coord.getKPE(is.label())->setShutdownState(KPE::ShutdownState::READY);
            Kernelcalls::get().reply(coord.getKPE(is.label()));

            if(coord.closingRequests < 0
                && PEManager::get().used() <= PEManager::get().daemons())
                coord.closingRequests = coord.broadcastShutdownRequest();
        }
        else {
            coord.getKPE(is.label())->msg_received();
            if(coord.getKPE(is.label())->getShutdownState() != KPE::ShutdownState::READY)
                coord.closingRequests--;
        }
        if(!coord.closingRequests && PEManager::get().used() <= PEManager::get().daemons()) {
            KLOG(KRNLC, "Shutting down " << coord.numKPEs() << " kernels");
            coord.broadcastShutdown();
        }
    }
    else {
        // kernel #0 coordinates the shutdown so the others will never receive replies
        assert(stage == Kernelcalls::OpStage::KREQUEST);
        coord.shutdownRequests++;
        if(PEManager::get().used() <= PEManager::get().daemons()) {
            Kernelcalls::get().requestShutdown(Coordinator::get().getKPE(0), Kernelcalls::OpStage::KREPLY);
            coord.closingRequests++;
        }
    }
}

void KernelcallHandler::shutdown(GateIStream& is) {
    Kernelcalls::OpStage stage;
    is >> stage;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::shutdown(stage=" <<
        (stage == Kernelcalls::OpStage::KREQUEST ? "reque" : "reply") << ")");

    if(stage == Kernelcalls::OpStage::KREQUEST) {
        assert(Coordinator::get().kid());
        Coordinator::get().shutdownIssued = true;
        if(PEManager::get().terminate())
            Kernelcalls::get().shutdown(Coordinator::get().getKPE(0), Kernelcalls::OpStage::KREPLY);
    }
    else {
        assert(!Coordinator::get().kid());
        Coordinator::get().removeKPE(is.label());
        if(!Coordinator::get().numKPEs())
            PEManager::get().terminate();
    }
}

void KernelcallHandler::announceKrnls(GateIStream &is) {
    uint amount;
    membership_entry::krnl_id_t kid;
    membership_entry::pe_id_t core;
    is >> amount;
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::announceKrnls(amount=" << amount << ")");

    uint numPEs = MHTInstance::getInstance().localPEs();
    m3::PEDesc localPEs[numPEs];
    uint addedPEs = 0;
    for(uint i = 0; i < Platform::MAX_PES; i++) {
        m3::PEDesc pe = Platform::pe_by_index(i);
        // check if this PE is actually ours
        if(MHTInstance::getInstance().responsibleKrnl(pe.core_id()) == Coordinator::get().kid())
            localPEs[addedPEs++] = pe;
    }
    assert(addedPEs == numPEs);
    for(uint i = 0; i < amount; i++) {
        is >> kid >> core;
        // connect to other kernels
        int localEp = reserve_ep(kid);
        if(localEp == -1)
            PANIC("No msg slots for kernel #" << kid << " left");
        Kernelcalls::get().connect(kid, core, Kernelcalls::OpStage::KREQUEST, Coordinator::get().kid(),
            Platform::kernel_pe(), localEp, numPEs, MembershipFlags::NONE, localPEs);
    }

    // tell the creator that we are ready to participate in the system
    Kernelcalls::get().sigvital(Coordinator::get().getKPE(is.label()), Platform::creatorThread(),
        m3::Errors::NO_ERROR);

    // wake up the init thread to continue loading apps
    m3::ThreadManager::get().notify(reinterpret_cast<void*>(0));
}

void KernelcallHandler::connect(GateIStream &is) {
    Kernelcalls::OpStage stage;
    membership_entry::krnl_id_t kid;
    membership_entry::pe_id_t core;
    int epid;
    is >> stage >> kid >> core >> epid;
    LOG_ANONYM(kid, "kernelcall::connect(stage=" <<
        (stage == Kernelcalls::OpStage::KREQUEST ? "reque" : "reply") << ", kid=" << kid <<
        ", core=" << core << ", epid=" << epid<< ")");

    if(stage == Kernelcalls::OpStage::KREQUEST) {
        // add the new kernel to our KPE list
        int localEp = reserve_ep(kid);
        if(localEp == -1) {
            KLOG(ERR, "No msg slots to connect to kernel #" << kid << " left");
            Kernelcalls::get().connect(kid, core, Kernelcalls::OpStage::KREPLY, Coordinator::get().kid(),
                Platform::kernel_pe(), -1, 0, MembershipFlags::NONE, nullptr);
        }
        else {
            KLOG(KRNLC, "Connecting to kernel #" << kid << " with EP " << localEp);
            Coordinator::get().addKPE("kernel", kid, core, localEp, epid);
            MembershipFlags flags;
            uint numPEs;
            is >> flags >> numPEs;
            m3::PEDesc* newPEs = new m3::PEDesc[numPEs];
            for(size_t i = 0; i < numPEs; i++) {
                m3::PEDesc::value_t desc;
                is >> desc;
                newPEs[i] = m3::PEDesc(desc);
            }

            Kernelcalls::get().connect(Coordinator::get().getKPE(kid), Kernelcalls::OpStage::KREPLY,
                Coordinator::get().kid(), Platform::kernel_pe(), localEp);

            MHTInstance::getInstance().updateMembership(newPEs, numPEs, kid, core, flags, false);

            ServiceList &srv = ServiceList::get();
            for(auto s = srv.begin(); s != srv.end(); s++)
                Kernelcalls::get().announceSrv(Coordinator::get().getKPE(kid), s->id(), s->name());
        }
    }
    else {
        if(epid == -1)
            KLOG(ERR, "Kernel #" << kid << " refused connection");
        else {
            int localEp = -1;
            for(auto it = _connectionReqs.begin(); it != _connectionReqs.end(); it++) {
                if(it->kid == kid) {
                    localEp = it->epid;
                    _connectionReqs.remove(&*it);
                    delete &*it;
                }
            }
            if(localEp == -1)
                PANIC("Unexpected connection setup by kernel #" << kid);

            KLOG(KRNLC, "Connecting to kernel #" << kid << " with EP " << localEp);
            Coordinator::get().addKPE("kernel", kid, core, localEp, epid);
        }
    }
}

void KernelcallHandler::reply(GateIStream &is) {
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::reply()");
    Coordinator::get().getKPE(is.label())->msg_received();
}

void KernelcallHandler::startApps(GateIStream &is) {
    LOG_KRNL(Coordinator::get().getKPE(is.label()), "kernelcall::startApps()");
    Coordinator::get().startSignsAwaited--;
    PEManager::get().start_pending(ServiceList::get(), RemoteServiceList::get());
    Kernelcalls::get().reply(Coordinator::get().getKPE(is.label()));
#ifdef SYNC_APP_START
    // identify for runtime extraction script
    if(Coordinator::get().kid() != 0)
        m3::DTU::get().debug_msg(0x11000000);
#endif
}

}
