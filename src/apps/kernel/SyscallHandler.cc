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

#include <base/tracing/Tracing.h>
#include <base/log/Kernel.h>
#include <base/Init.h>
#include <base/Panic.h>
#include <base/WorkLoop.h>
#include <base/benchmark/capbench.h>
#include <m3/server/Server.h>

#include "pes/PEManager.h"
#include "com/Services.h"
#include "com/RecvBufs.h"
#include "Platform.h"
#include "SyscallHandler.h"
#include "Coordinator.h"
#include "KernelcallHandler.h"
#include "ddl/MHTInstance.h"

// #define SIMPLE_SYSC_LOG

#if defined(__host__)
extern int int_target;
#endif

namespace kernel {

INIT_PRIO_USER(3) SyscallHandler SyscallHandler::_inst;

#if defined(SIMPLE_SYSC_LOG)
#   define LOG_SYS(vpe, sysname, expr) \
        KLOG(SYSC, (vpe)->name() << (sysname))
#else
#   define LOG_SYS(vpe, sysname, expr) \
        KLOG(SYSC, (vpe)->name() << "@" << m3::fmt((vpe)->core(), "X") << (sysname) << expr)
#endif

#define SYS_ERROR(vpe, is, error, msg) { \
        KLOG(ERR, (vpe)->name() << ": " << msg << " (" << error << ")"); \
        reply_vmsg((is), (error)); \
        return; \
    }
#define SYS_ERROR_DELAYED(vpe, rinfo, error, msg) { \
        KLOG(ERR, (vpe).name() << ": " << msg << " (" << error << ")"); \
        auto reply = kernel::create_vmsg(error);\
        reply_to_vpe(vpe, rinfo, reply.bytes(), reply.total()); \
        return; \
    }

struct ReplyInfo {
    explicit ReplyInfo(const m3::DTU::Message &msg)
        : replylbl(msg.replylabel), replyep(msg.reply_epid()), crdep(msg.send_epid()),
          replycrd(msg.length) {
    }

    label_t replylbl;
    int replyep;
    int crdep;
    word_t replycrd;
};

static void reply_to_vpe(VPE &vpe, const ReplyInfo &info, const void *msg, size_t size) {
    DTU::get().reply_to(vpe.desc(), info.replyep, info.crdep, info.replycrd, info.replylbl, msg, size);
}

static m3::Errors::Code do_activate(VPE *vpe, size_t epid, MsgCapability *oldcapobj, MsgCapability *newcapobj) {
    if(newcapobj) {
        LOG_SYS(vpe, ": syscall::activate", ": setting ep[" << epid << "] to lbl="
                << m3::fmt(newcapobj->obj->label, "#0x", sizeof(label_t) * 2) << ", core=" << newcapobj->obj->core
                << ", ep=" << newcapobj->obj->epid
                << ", crd=#" << m3::fmt(newcapobj->obj->credits, "x"));
    }
    else
        LOG_SYS(vpe, ": syscall::activate", ": setting ep[" << epid << "] to NUL");

    m3::Errors::Code res = vpe->xchg_ep(epid, oldcapobj, newcapobj);
    if(res != m3::Errors::NO_ERROR)
        return res;

    if(oldcapobj)
        oldcapobj->localepid = -1;
    if(newcapobj)
        newcapobj->localepid = epid;
    return m3::Errors::NO_ERROR;
}

SyscallHandler::SyscallHandler() : _serv_ep(DTU::get().alloc_ep()) {
#if !defined(__t2__)
    // configure both receive buffers (we need to do that manually in the kernel)
    if(Platform::pe_count() > DTU::SYSC_GATES * m3::DTU::MAX_MSG_SLOTS)
        KLOG(ERR, "Not enough msg slots for " << Platform::pe_count() << " PEs");
    int buford = m3::getnextlog2(m3::DTU::MAX_MSG_SLOTS) + VPE::SYSC_CREDIT_ORD;
    size_t bufsize = static_cast<size_t>(1) << buford;
    for(uint i = 0; i < DTU::SYSC_GATES; i++)
        DTU::get().config_recv_local(epid(i),reinterpret_cast<uintptr_t>(new uint8_t[bufsize]),
            buford, VPE::SYSC_CREDIT_ORD, 0);

    for(uint i = 0; i < SYSC_SLOTS; i++)
        _epOccup[i] = -1;

    buford = m3::nextlog2<Service::SRV_MSG_SIZE * m3::DTU::MAX_MSG_SLOTS>::val;
    bufsize = static_cast<size_t>(1) << buford;
    DTU::get().config_recv_local(srvepid(), reinterpret_cast<uintptr_t>(new uint8_t[bufsize]),
        buford, m3::nextlog2<Service::SRV_MSG_SIZE>::val, 0);
#endif

    // add a dummy item to workloop; we handle everything manually anyway
    // but one item is needed to not stop immediately
    m3::env()->workloop()->add(nullptr, false);

    add_operation(m3::KIF::Syscall::PAGEFAULT, &SyscallHandler::pagefault);
    add_operation(m3::KIF::Syscall::CREATESRV, &SyscallHandler::createsrv);
    add_operation(m3::KIF::Syscall::CREATESESS, &SyscallHandler::createsess);
    add_operation(m3::KIF::Syscall::CREATESESSAT, &SyscallHandler::createsessat);
    add_operation(m3::KIF::Syscall::CREATEGATE, &SyscallHandler::creategate);
    add_operation(m3::KIF::Syscall::CREATEVPE, &SyscallHandler::createvpe);
    add_operation(m3::KIF::Syscall::CREATEMAP, &SyscallHandler::createmap);
    add_operation(m3::KIF::Syscall::ATTACHRB, &SyscallHandler::attachrb);
    add_operation(m3::KIF::Syscall::DETACHRB, &SyscallHandler::detachrb);
    add_operation(m3::KIF::Syscall::EXCHANGE, &SyscallHandler::exchange);
    add_operation(m3::KIF::Syscall::VPECTRL, &SyscallHandler::vpectrl);
    add_operation(m3::KIF::Syscall::DELEGATE, &SyscallHandler::delegate);
    add_operation(m3::KIF::Syscall::OBTAIN, &SyscallHandler::obtain);
    add_operation(m3::KIF::Syscall::ACTIVATE, &SyscallHandler::activate);
    add_operation(m3::KIF::Syscall::REQMEM, &SyscallHandler::reqmem);
    add_operation(m3::KIF::Syscall::DERIVEMEM, &SyscallHandler::derivemem);
    add_operation(m3::KIF::Syscall::REVOKE, &SyscallHandler::revoke);
    add_operation(m3::KIF::Syscall::EXIT, &SyscallHandler::exit);
    add_operation(m3::KIF::Syscall::NOOP, &SyscallHandler::noop);
#if defined(__host__)
    add_operation(m3::KIF::Syscall::COUNT, &SyscallHandler::init);
#endif
}

void SyscallHandler::createsrv(GateIStream &is) {
    EVENT_TRACER_Syscall_createsrv();
    VPE *vpe = is.gate().session<VPE>();
    m3::String name;
    capsel_t gatesel, srv;
    is >> gatesel >> srv >> name;
    LOG_SYS(vpe, ": syscall::createsrv", "(gate=" << gatesel
        << ", srv=" << srv << ", name=" << name << ")");

    MsgCapability *gatecap = static_cast<MsgCapability*>(vpe->objcaps().get(gatesel, Capability::MSG));
    if(gatecap == nullptr || name.length() == 0)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap or name");
    if(ServiceList::get().find(name) != nullptr)
        SYS_ERROR(vpe, is, m3::Errors::EXISTS, "Service does already exist");

    // TODO this depends on the credits that the kernel has
    int capacity = m3::Server<m3::Handler<> >::MAX_KRNL_MSGS;
    Service *s = ServiceList::get().add(*vpe, srv, name,
        gatecap->obj->epid, gatecap->obj->label, capacity,
        HashUtil::structured_hash(vpe->core(), vpe->id(), SERVICE, srv));
    vpe->objcaps().set(srv, new ServiceCapability(&vpe->objcaps(), srv, s,
        HashUtil::structured_hash(vpe->core(), vpe->id(), SRVCAP, srv)));

#if defined(__host__)
    // TODO ugly hack
    if(name == "interrupts")
        int_target = vpe->pid();
#endif

    // maybe there are VPEs that now have all requirements fullfilled
    // Hack: don't propagate pipe services for the sake of simulation time
    if(!name.contains("pipe"))
        Coordinator::get().broadcastAnnounceSrv(name, s->id());
    PEManager::get().start_pending(ServiceList::get(), RemoteServiceList::get());

    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::pagefault(UNUSED GateIStream &is) {
#if defined(__gem5__)
    VPE *vpe = is.gate().session<VPE>();
    uint64_t virt, access;
    is >> virt >> access;
    LOG_SYS(vpe, ": syscall::pagefault", "(virt=" << m3::fmt(virt, "p")
        << ", access " << m3::fmt(access, "#x") << ")");

    if(!vpe->address_space())
        SYS_ERROR(vpe, is, m3::Errors::NOT_SUP, "No address space / PF handler");

    // if we don't have a pager, it was probably because of speculative execution. just return an
    // error in this case and don't print anything
    capsel_t gcap = vpe->address_space()->gate();
    MsgCapability *msg = static_cast<MsgCapability*>(vpe->objcaps().get(gcap, Capability::MSG));
    if(msg == nullptr) {
        // we never map page 0 and thus we tell the DTU to remember that there is no mapping
        if((virt & ~PAGE_MASK) == 0) {
            KLOG(PTES, "No mapping at page 0");
            reply_vmsg(is, m3::Errors::NO_MAPPING);
            return;
        }
        reply_vmsg(is, m3::Errors::INV_ARGS);
        return;
    }

    // TODO this might also indicates that the pf handler is not available (ctx switch, migrate, ...)
    if(msg->localepid != -1)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "EP already configured");

    m3::Errors::Code res = do_activate(vpe, vpe->address_space()->ep(), nullptr, msg);
    if(res != m3::Errors::NO_ERROR)
        SYS_ERROR(vpe, is, res, "Activate failed");
#endif

    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::createsess(GateIStream &is) {
    EVENT_TRACER_Syscall_createsess();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tvpe, cap;
    m3::String name;
    is >> tvpe >> cap >> name;
    LOG_SYS(vpe, ": syscall::createsess",
        "(vpe=" << tvpe << ", name=" << name << ", cap=" << cap << ")");

    Service *s = ServiceList::get().find(name);
    // local service
    if(s != nullptr){
        VPECapability *tvpeobj = static_cast<VPECapability*>(vpe->objcaps().get(tvpe, Capability::VIRTPE));
        if(tvpeobj == nullptr)
            SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");

        if(!tvpeobj->vpe->objcaps().unused(cap))
            SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap");

        if(s->closing)
            SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Unknown service");

        ReplyInfo rinfo(is.message());
        m3::Reference<Service> rsrv(s);
        vpe->service_gate().subscribe([this, rsrv, cap, vpe, tvpeobj, rinfo]
                (GateIStream &reply, m3::Subscriber<GateIStream&> *sub) {
            EVENT_TRACER_Syscall_createsess();
            m3::Errors::Code res;
            reply >> res;

            LOG_SYS(vpe, ": syscall::createsess-cb", "(res=" << res << ")");

            if(res != m3::Errors::NO_ERROR) {
                KLOG(SYSC, vpe->id() << ": Server denied session creation (" << res << ")");
                auto reply = kernel::create_vmsg(res);
                reply_to_vpe(*vpe, rinfo, reply.bytes(), reply.total());
            }
            else {
                word_t sess;
                reply >> sess;

                // inherit the session-cap from the service-cap. this way, it will be automatically
                // revoked if the service-cap is revoked
                Capability *srvcap = rsrv->vpe().objcaps().get(rsrv->selector(), Capability::SERVICE);
                assert(srvcap != nullptr);
                SessionCapability *sesscap = new SessionCapability(
                    &tvpeobj->vpe->objcaps(), cap, const_cast<Service*>(&*rsrv),
                    HashUtil::structured_hash(rsrv->vpe().core(), rsrv->vpe().id(), SERVICE, rsrv->selector()),
                    sess, HashUtil::structured_hash(vpe->core(), vpe->id(), SESSCAP, cap));
                tvpeobj->vpe->objcaps().inherit_and_set(srvcap, sesscap, cap);

                auto reply = kernel::create_vmsg(res);
                reply_to_vpe(*vpe, rinfo, reply.bytes(), reply.total());
            }

            const_cast<m3::Reference<Service>&>(rsrv)->received_reply();
            vpe->service_gate().unsubscribe(sub);
        });

        AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<m3::KIF::Service::Command>(), is.remaining()));
        msg << m3::KIF::Service::OPEN;
        msg.put(is);
        s->send(&vpe->service_gate(), msg.bytes(), msg.total(), msg.is_on_heap());
        msg.claim();
    }
    else { // it might be a remote service
        AutoGateOStream msg(m3::vostreamsize(is.remaining()));
        msg.put(is);
        mht_key_t sessCapId = HashUtil::structured_hash(vpe->core(), vpe->id(), SESSCAP, cap);
        label_t srvLocation;
        m3::Errors::Code res;

        vpe->srvLookupResult(nullptr);
        // check if service has been announced before
        RemoteServiceList::RemoteService *remoteSrv = RemoteServiceList::get().find(name);
        if(remoteSrv) {
            Kernelcalls::get().createSessFwd(Coordinator::get().getKPE(
                MHTInstance::getInstance().responsibleMember(remoteSrv->id)),
                vpe->id(), name, sessCapId, msg);

            vpe->sessAwaitingResp(1);
            // wait for the response
            m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(m3::ThreadManager::get().current()->id()));
        }
        else if(!vpe->srvLookupResult()) {
            uint awaitedResp = Coordinator::get().broadcastCreateSess(vpe->id(), name, sessCapId, msg);
            // if no other kernels in the system the service isn't existing
            if(!awaitedResp)
                SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Unknown service");

            vpe->sessAwaitingResp(awaitedResp);

            // when all responses arrived we will be woken up
            m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(m3::ThreadManager::get().current()->id()));
        }

        GateOStream *resp = vpe->srvLookupResult();
        if(!resp)
            SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Unknown service");

        m3::Unmarshaller srvRes(resp->bytes(), resp->total());
        srvRes >> srvLocation >> res;
        if(res != m3::Errors::NO_ERROR) {
            KLOG(SYSC, vpe->id() << ": Server denied session creation (" << res << ")");
            reply_vmsg(is, res);
        }
        else {
            word_t sess;
            mht_key_t srvcap;
            srvRes >> sess >> srvcap;
            // in error cases we need to inform the remote kernel that it removes the child
            // reference again since the session was not created
            VPECapability *tvpeobj = static_cast<VPECapability*>(vpe->objcaps().get(tvpe, Capability::VIRTPE));
            if(tvpeobj == nullptr) {
                Kernelcalls::get().createSessFail(Coordinator::get().getKPE(srvLocation), sessCapId,
                    HashUtil::structured_hash(
                    HashUtil::hashToPeId(srvcap), HashUtil::hashToVpeId(srvcap),
                    SERVICE, HashUtil::hashToObjId(srvcap)));
                SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");
            }

            if(!tvpeobj->vpe->objcaps().unused(cap)) {
                Kernelcalls::get().createSessFail(Coordinator::get().getKPE(srvLocation), sessCapId,
                    HashUtil::structured_hash(
                    HashUtil::hashToPeId(srvcap), HashUtil::hashToVpeId(srvcap),
                    SERVICE, HashUtil::hashToObjId(srvcap)));
                SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap");
            }

            // inherit the session-cap from the service-cap. this way, it will be automatically
            // revoked if the service-cap is revoked
            SessionCapability *sesscap = new SessionCapability(
                &tvpeobj->vpe->objcaps(), cap, nullptr,
                HashUtil::structured_hash(
                    HashUtil::hashToPeId(srvcap), HashUtil::hashToVpeId(srvcap),
                    SERVICE, HashUtil::hashToObjId(srvcap)),
                sess, sessCapId);
            tvpeobj->vpe->objcaps().setparent_and_set(srvcap, sesscap, cap);

            reply_vmsg(is, res);
        }
    }
}

void SyscallHandler::createsessat(GateIStream &is) {
    EVENT_TRACER_Syscall_createsess();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t srvcap, sesscap;
    word_t ident;
    is >> srvcap >> sesscap >> ident;
    LOG_SYS(vpe, ": syscall::createsessat",
        "(service=" << srvcap << ", session=" << sesscap << ", ident=#" << m3::fmt(ident, "0x") << ")");

    if(!vpe->objcaps().unused(sesscap))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid session selector");

    ServiceCapability *scapobj = static_cast<ServiceCapability*>(
        vpe->objcaps().get(srvcap, Capability::SERVICE));
    if(scapobj == nullptr || scapobj->inst->closing)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Service capability is invalid");

    SessionCapability *sess = new SessionCapability(&vpe->objcaps(), sesscap,
        const_cast<Service*>(&*scapobj->inst), 0, ident, 0); // TODO: capid
    sess->obj->servowned = true;
    vpe->objcaps().set(sesscap, sess);

    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::creategate(GateIStream &is) {
    EVENT_TRACER_Syscall_creategate();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap,dstcap;
    label_t label;
    size_t epid;
    word_t credits;
    is >> tcap >> dstcap >> label >> epid >> credits;
    LOG_SYS(vpe, ": syscall::creategate", "(vpe=" << tcap << ", cap=" << dstcap
        << ", label=" << m3::fmt(label, "#0x", sizeof(label_t) * 2)
        << ", ep=" << epid << ", crd=#" << m3::fmt(credits, "0x") << ")");

#if defined(__gem5__)
    if(credits == m3::KIF::UNLIM_CREDITS)
        PANIC("Unlimited credits are not yet supported on gem5");
#endif

    VPECapability *tcapobj = static_cast<VPECapability*>(vpe->objcaps().get(tcap, Capability::VIRTPE));
    if(tcapobj == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");

    // 0 points to the SEPs and can't be delegated to someone else
    if(epid == 0 || epid >= EP_COUNT || !vpe->objcaps().unused(dstcap))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap or ep");

    vpe->objcaps().set(dstcap,
        new MsgCapability(&vpe->objcaps(), dstcap, label, tcapobj->vpe->core(),
            tcapobj->vpe->id(), epid, credits,
            HashUtil::structured_hash(tcapobj->vpe->core(), tcapobj->vpe->id(), MSGOBJ, label), 0)); // TODO: capid
    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::createvpe(GateIStream &is) {
    EVENT_TRACER_Syscall_createvpe();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap, mcap, gcap;
    m3::PEDesc::value_t pe;
    m3::String name;
    size_t ep;
    is >> tcap >> mcap >> name >> pe >> gcap >> ep;
    LOG_SYS(vpe, ": syscall::createvpe", "(name=" << name
        << ", pe=" << static_cast<int>(m3::PEDesc(pe).type())
        << ", tcap=" << tcap << ", mcap=" << mcap << ", pfgate=" << gcap
        << ", pfep=" << ep << ")");

    // we don't need to reserve these slots since the calling VPE won't do any other operation in the mean time
    if(!vpe->objcaps().unused(tcap) || !vpe->objcaps().unused(mcap))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid VPE or memory cap");

    // if it has a pager, we need a gate cap
    MsgCapability *msg = nullptr;
    if(gcap != m3::KIF::INV_SEL) {
        msg = static_cast<MsgCapability*>(vpe->objcaps().get(gcap, Capability::MSG));
        if(msg == nullptr)
            SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap(s)");
    }

    // create VPE
    VPE *nvpe = PEManager::get().create(std::move(name), m3::PEDesc(pe), ep, gcap);
    if(nvpe == nullptr) {
        // TODO
        // forward request to another kernel
        SYS_ERROR(vpe, is, m3::Errors::NO_FREE_CORE, "No free and suitable core found");
    }
    else {
        // delegate VPE and memory cap
        // inherit VPE and mem caps to the parent
        vpe->objcaps().obtain(tcap, nvpe->objcaps().get(0));
        vpe->objcaps().obtain(mcap, nvpe->objcaps().get(1));

        // initialize paging
        if(gcap != m3::KIF::INV_SEL) {
            // delegate pf gate to the new VPE
            nvpe->objcaps().obtain(gcap, msg);

            DTU::get().config_pf_remote(nvpe->desc(), nvpe->address_space()->root_pt(), ep);
        }

        reply_vmsg(is, m3::Errors::NO_ERROR, Platform::pe_by_core(nvpe->core()).value());
    }
}

void SyscallHandler::createmap(UNUSED GateIStream &is) {
#if defined(__gem5__)
    EVENT_TRACER_Syscall_createmap();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap, mcap;
    capsel_t first, pages, dst;
    int perms;
    is >> tcap >> mcap >> first >> pages >> dst >> perms;
    LOG_SYS(vpe, ": syscall::createmap", "(vpe=" << tcap << ", mem=" << mcap
        << ", first=" << first << ", pages=" << pages << ", dst=" << dst
        << ", perms=" << perms << ")");

    VPECapability *tcapobj = static_cast<VPECapability*>(vpe->objcaps().get(tcap, Capability::VIRTPE));
    if(tcapobj == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");
    MemCapability *mcapobj = static_cast<MemCapability*>(vpe->objcaps().get(mcap, Capability::MEM));
    if(mcapobj == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Memory capability is invalid");

    if((mcapobj->addr() & PAGE_MASK) || (mcapobj->size() & PAGE_MASK))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Memory capability is not page aligned");
    if(perms & ~mcapobj->perms())
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid permissions");

    size_t total = mcapobj->size() >> PAGE_BITS;
    if(first >= total || first + pages <= first || first + pages > total)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Region of memory capability is invalid");

    uintptr_t phys = m3::DTU::build_noc_addr(mcapobj->obj->core, mcapobj->addr() + PAGE_SIZE * first);
    CapTable &mcaps = tcapobj->vpe->mapcaps();
    for(capsel_t i = 0; i < pages; ++i) {
        MapCapability *mapcap = static_cast<MapCapability*>(mcaps.get(dst + i, Capability::MAP));
        if(mapcap == nullptr) {
            MapCapability *mapcap = new MapCapability(&mcaps, dst + i, phys, perms, 0); // TODO: capid
            mcaps.inherit_and_set(mcapobj, mapcap, dst + i);
        }
        else
            mapcap->remap(perms);
        phys += PAGE_SIZE;
    }
#endif

    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::attachrb(GateIStream &is) {
    EVENT_TRACER_Syscall_attachrb();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap;
    uintptr_t addr;
    size_t ep;
    int order, msgorder;
    uint flags;
    is >> tcap >> ep >> addr >> order >> msgorder >> flags;
    LOG_SYS(vpe, ": syscall::attachrb", "(vpe=" << tcap << ", ep=" << ep
        << ", addr=" << m3::fmt(addr, "p") << ", size=" << m3::fmt(1UL << order, "#x")
        << ", msgsize=" << m3::fmt(1UL << msgorder, "#x") << ", flags=" << m3::fmt(flags, "#x") << ")");

    VPECapability *tcapobj = static_cast<VPECapability*>(vpe->objcaps().get(tcap, Capability::VIRTPE));
    if(tcapobj == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");

    if(addr < (Platform::rw_barrier(tcapobj->vpe->core()) -
        /* HACK: load generator PEs get additional space for receive buffers */
        (vpe->name().contains(("loadgen")) ? SECONDARY_RECVBUF_SIZE_SPM : 0))
        || (order > 20) || (msgorder > order))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Not in receive buffer space");

    m3::Errors::Code res = RecvBufs::attach(*tcapobj->vpe, ep, addr, order, msgorder, flags);
    if(res != m3::Errors::NO_ERROR)
        SYS_ERROR(vpe, is, res, "Unable to attach receive buffer");

    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::detachrb(GateIStream &is) {
    EVENT_TRACER_Syscall_detachrb();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap;
    size_t ep;
    is >> tcap >> ep;
    LOG_SYS(vpe, ": syscall::detachrb", "(vpe=" << tcap << ", ep=" << ep << ")");

    VPECapability *tcapobj = static_cast<VPECapability*>(vpe->objcaps().get(tcap, Capability::VIRTPE));
    if(tcapobj == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");

    RecvBufs::detach(*tcapobj->vpe, ep);
    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::exchange(GateIStream &is) {
    EVENT_TRACER_Syscall_exchange();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap;
    m3::CapRngDesc own, other;
    bool obtain;
    is >> tcap >> own >> other >> obtain;
    LOG_SYS(vpe, ": syscall::exchange", "(vpe=" << tcap << ", own=" << own
        << ", other=" << other << ", obtain=" << obtain << ")");

    VPECapability *vpecap = static_cast<VPECapability*>(
            vpe->objcaps().get(tcap, Capability::VIRTPE));
    if(vpecap == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid VPE cap");

    VPE *t1 = obtain ? vpecap->vpe : vpe;
    VPE *t2 = obtain ? vpe : vpecap->vpe;
    m3::Errors::Code res = do_exchange(t1, t2, own, other, obtain);
    reply_vmsg(is, res);
}

void SyscallHandler::vpectrl(GateIStream &is) {
    EVENT_TRACER_Syscall_vpectrl();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tcap;
    m3::KIF::Syscall::VPECtrl op;
    int pid;
    is >> tcap >> op >> pid;
    LOG_SYS(vpe, ": syscall::vpectrl", "(vpe=" << tcap << ", op=" << op
            << ", pid=" << pid << ")");

    VPECapability *vpecap = static_cast<VPECapability*>(
            vpe->objcaps().get(tcap, Capability::VIRTPE));
    if(vpecap == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap");

    // TODO
    // if the VPE cap points to a remote VPE, we can't just access the vpe pointer
    if(!MHTInstance::getInstance().keyLocality(vpecap->vpeId))
        SYS_ERROR(vpe, is, m3::Errors::NOT_SUP, "Controlling remote VPEs not implemented");
    switch(op) {
        case m3::KIF::Syscall::VCTRL_START:
            vpecap->vpe->start(0, nullptr, pid);
            reply_vmsg(is, m3::Errors::NO_ERROR);
            break;

        case m3::KIF::Syscall::VCTRL_WAIT:
            if(vpecap->vpe->state() == VPE::DEAD)
                reply_vmsg(is, m3::Errors::NO_ERROR, vpecap->vpe->exitcode());
            else {
                ReplyInfo rinfo(is.message());
                vpecap->vpe->subscribe_exit([vpe, is, rinfo] (int exitcode, m3::Subscriber<int> *) {
                    EVENT_TRACER_Syscall_vpectrl();

                    LOG_SYS(vpe, ": syscall::vpectrl-cb", "(exitcode=" << exitcode << ")");

                    auto reply = kernel::create_vmsg(m3::Errors::NO_ERROR,exitcode);
                    reply_to_vpe(*vpe, rinfo, reply.bytes(), reply.total());
                });
            }
            break;
    }
}

void SyscallHandler::reqmem(GateIStream &is) {
    EVENT_TRACER_Syscall_reqmem();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t cap;
    uintptr_t addr;
    size_t size;
    int perms;
    is >> cap >> addr >> size >> perms;
    LOG_SYS(vpe, ": syscall::reqmem", "(cap=" << cap
        << ", addr=#" << m3::fmt(addr, "x") << ", size=#" << m3::fmt(size, "x")
        << ", perms=" << perms << ")");

    if(!vpe->objcaps().unused(cap))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap");
    if(size == 0 || (size & m3::KIF::Perm::RWX) || perms == 0 || (perms & ~(m3::KIF::Perm::RWX)))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Size or permissions invalid");

    MainMemory &mem = MainMemory::get();
    MainMemory::Allocation alloc =
        addr == (uintptr_t)-1 ? mem.allocate(size) : mem.allocate_at(addr, size);
    if(!alloc)
        SYS_ERROR(vpe, is, m3::Errors::OUT_OF_MEM, "Not enough memory");

    // TODO if addr was 0, we don't want to free it on revoke
    vpe->objcaps().set(cap, new MemCapability(&vpe->objcaps(), cap,
        alloc.addr, alloc.size, perms, alloc.pe(), 0, 0,
        HashUtil::structured_hash(vpe->core(), vpe->id(), MEMOBJ, alloc.addr), 0)); // TODO: capid
    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::derivemem(GateIStream &is) {
    EVENT_TRACER_Syscall_derivemem();
    VPE *vpe = is.gate().session<VPE>();
    capsel_t src, dst;
    size_t offset, size;
    int perms;
    is >> src >> dst >> offset >> size >> perms;
    LOG_SYS(vpe, ": syscall::derivemem", "(src=" << src << ", dst=" << dst
        << ", size=" << size << ", off=" << offset << ", perms=" << perms << ")");

    MemCapability *srccap = static_cast<MemCapability*>(
            vpe->objcaps().get(src, Capability::MEM));
    if(srccap == nullptr || !vpe->objcaps().unused(dst))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap(s)");

    if(offset + size < offset || offset + size > srccap->size() || size == 0 ||
            (perms & ~(m3::KIF::Perm::RWX)))
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid args");

    MemCapability *dercap = static_cast<MemCapability*>(vpe->objcaps().obtain(dst, srccap));
    dercap->obj = m3::Reference<MsgObject>(new MemObject(
        srccap->addr() + offset,
        size,
        perms & srccap->perms(),
        srccap->obj->core,
        srccap->obj->vpe,
        srccap->obj->epid,
        HashUtil::structured_hash(srccap->obj->core, srccap->obj->vpe, MEMOBJ, srccap->addr() + offset)
    ));
    dercap->obj->derived = true;
    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::delegate(GateIStream &is) {
    EVENT_TRACER_Syscall_delegate();
    exchange_over_sess(is, false);
}

void SyscallHandler::obtain(GateIStream &is) {
    EVENT_TRACER_Syscall_obtain();
    CAP_BENCH_TRACE_X_S(KERNEL_OBT_SYSC_RCV);
    exchange_over_sess(is, true);
}

m3::Errors::Code SyscallHandler::do_exchange(VPE *v1, VPE *v2, const m3::CapRngDesc &c1,
        const m3::CapRngDesc &c2, bool obtain) {
    VPE &src = obtain ? *v2 : *v1;
    VPE &dst = obtain ? *v1 : *v2;
    const m3::CapRngDesc &srcrng = obtain ? c2 : c1;
    const m3::CapRngDesc &dstrng = obtain ? c1 : c2;

    if(c1.type() != c2.type()) {
        KLOG(SYSC, v1->id() << ": Descriptor types don't match (" << m3::Errors::INV_ARGS << ")");
        return m3::Errors::INV_ARGS;
    }
    if((obtain && c2.count() > c1.count()) || (!obtain && c2.count() != c1.count())) {
        KLOG(SYSC, v1->id() << ": Server gave me invalid CRD (" << m3::Errors::INV_ARGS << ")");
        return m3::Errors::INV_ARGS;
    }
    if(!dst.objcaps().range_unused(dstrng)) {
        KLOG(SYSC, v1->id() << ": Invalid destination caps (" << m3::Errors::INV_ARGS << ")");
        return m3::Errors::INV_ARGS;
    }

    CapTable &srctab = c1.type() == m3::CapRngDesc::OBJ ? src.objcaps() : src.mapcaps();
    CapTable &dsttab = c1.type() == m3::CapRngDesc::OBJ ? dst.objcaps() : dst.mapcaps();
    for(uint i = 0; i < c2.count(); ++i) {
        capsel_t srccap = srcrng.start() + i;
        capsel_t dstcap = dstrng.start() + i;
        Capability *scapobj = srctab.get(srccap);
        assert(dsttab.get(dstcap) == nullptr);
        dsttab.obtain(dstcap, scapobj);
    }
    return m3::Errors::NO_ERROR;
}

void SyscallHandler::exchange_over_sess(GateIStream &is, bool obtain) {
    VPE *vpe = is.gate().session<VPE>();
    capsel_t tvpe, sesscap;
    m3::CapRngDesc caps;
    is >> tvpe >> sesscap >> caps;
    // TODO compiler-bug? if I try to print caps, it hangs on T2!? doing it here manually works
    LOG_SYS(vpe, (obtain ? "syscall::obtain" : "syscall::delegate"),
            "(vpe=" << tvpe << ", sess=" << sesscap << ", caps="
            << caps.start() << ":" << caps.count() << ")");

    VPECapability *tvpeobj = static_cast<VPECapability*>(vpe->objcaps().get(tvpe, Capability::VIRTPE));
    if(tvpeobj == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "VPE capability is invalid");

    SessionCapability *sess = static_cast<SessionCapability*>(
        tvpeobj->vpe->objcaps().get(sesscap, Capability::SESSION));
    if(sess == nullptr)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid session-cap");

    if(MHTInstance::getInstance().keyLocality(sess->obj->srvID)) { // local service
        if(sess->obj->srv->closing)
            SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Server is shutting down");

        ReplyInfo rinfo(is.message());
        // only pass in the service-reference. we can't be sure that the session will still exist
        // when we receive the reply
        m3::Reference<Service> rsrv(sess->obj->srv);
        vpe->service_gate().subscribe([this, rsrv, caps, vpe, tvpeobj, obtain, rinfo]
                (GateIStream &reply, m3::Subscriber<GateIStream&> *sub) {
            CAP_BENCH_TRACE_X_F(KERNEL_OBT_FROM_SRV);
            EVENT_TRACER_Syscall_delob_done();
            m3::CapRngDesc srvcaps;

            m3::Errors::Code res;
            reply >> res;

            LOG_SYS(vpe, (obtain ? ": syscall::obtain-cb" : ": syscall::delegate-cb"),
                "(vpe=" << tvpeobj->sel() << ", res=" << res << ")");

            if(res != m3::Errors::NO_ERROR) {
                KLOG(SYSC, tvpeobj->vpe->id() << ": Server denied cap-transfer (" << res << ")");

                auto reply = kernel::create_vmsg(res);
                reply_to_vpe(*vpe, rinfo, reply.bytes(), reply.total());
                goto error;
            }

            reply >> srvcaps;
            if((res = do_exchange(tvpeobj->vpe, &rsrv->vpe(), caps, srvcaps, obtain)) != m3::Errors::NO_ERROR) {
                auto reply = kernel::create_vmsg(res);
                reply_to_vpe(*vpe, rinfo, reply.bytes(), reply.total());
                goto error;
            }

            {
                AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<m3::Errors, m3::CapRngDesc>(),
                    reply.remaining()));
                msg << m3::Errors::NO_ERROR;
                msg.put(reply);
                CAP_BENCH_TRACE_X_F(KERNEL_OBT_SYSC_RESP);
                reply_to_vpe(*vpe, rinfo, msg.bytes(), msg.total());
            }

        error:
            const_cast<m3::Reference<Service>&>(rsrv)->received_reply();
            vpe->service_gate().unsubscribe(sub);
        });

        AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<m3::KIF::Service::Command, word_t,
            m3::CapRngDesc>(), is.remaining()));
        msg << (obtain ? m3::KIF::Service::OBTAIN : m3::KIF::Service::DELEGATE) << sess->obj->ident << caps.count();
        msg.put(is);
        CAP_BENCH_TRACE_X_S(KERNEL_OBT_TO_SRV);
        sess->obj->srv->send(&vpe->service_gate(), msg.bytes(), msg.total(), msg.is_on_heap());
        msg.claim();
    }
    else { // remote service
        // information flow:
        // app --- obtain, vpeCap, sessCap, capRange, [remainder] ---> krnl
        //      krnl --- obtain, vpeID, srvID, sessID, capRange, [remainder] ---> rkrnl
        //                  rkrnl: --- obtain, srvID, sessID, capcount, [remainder] --> srv
        //                  rknrl: <--- res, [capRange(type, start, count)], [reply] --- srv
        //      krnl <--- res, [capRange(type, start, count)], [reply] --- rknrl
        //      [krnl --- cancel ---> rkrnl]    (only sent on failure)
        // app <--- res, [reply] --- krnl

        int vpeID = vpe->id();
        AutoGateOStream remain(m3::vostreamsize(is.remaining()));
        if(is.remaining())
            remain.put(is);
        Kernelcalls::get().exchangeOverSession(Coordinator::get().getKPE(
            MHTInstance::getInstance().responsibleMember(sess->obj->srvID)), obtain,
            tvpeobj->id(), sess->obj->srvID, sess->obj->ident, caps, remain);

        m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(m3::ThreadManager::get().current()->id()));
        CAP_BENCH_TRACE_X_F(KERNEL_OBT_THRD_WAKEUP);
        m3::Unmarshaller replyLength(m3::ThreadManager::get().get_current_msg(),
            m3::ostreamsize<m3::Errors::Code, unsigned long int>());
        m3::Errors::Code res;
        replyLength >> res;
        LOG_SYS(vpe, (obtain ? ": syscall::obtain-cont" : ": syscall::delegate-cont"),
            ": Received reply from remote kernel. res=" << res);

        if(res == m3::Errors::NO_ERROR) {
            size_t size;
            m3::CapRngDesc::Type srvcapsType;
            capsel_t srvcapsStart;
            uint srvcapsCount;
            int repliertid;

            replyLength >> size;
            m3::Unmarshaller reply(m3::ThreadManager::get().get_current_msg() +
                m3::ostreamsize<m3::Errors::Code, unsigned long int>(), size);
            reply >> repliertid >> srvcapsType >> srvcapsStart >> srvcapsCount;

            // insert caps
            // if obtain:   Deserialize caps here and call obtain on them
            // if delegate: The cloning has to be done at the other kernel.
            //              Add the child here and serialize the caps into
            //              the reply. The remote kernel will use the
            //              reserved cap slots and execute the obtain.
            if(obtain && PEManager::get().exists(vpeID) && !vpe->objcaps().range_unused(caps)) {
                // send cancel request to other kernel to remove child pointers
                if(srvcapsCount > 0) {
                    Capability *firstSrvcapPtr = Capability::createFromStream(reply);
                    Kernelcalls::get().removeChildCapPtr(Coordinator::get().getKPE(
                        MHTInstance::getInstance().responsibleMember(sess->obj->srvID)),
                        DDLCapRngDesc(firstSrvcapPtr->id(), srvcapsCount),
                        DDLCapRngDesc(
                            HashUtil::structured_hash(vpe->id(), vpe->id(), NOTYPE, caps.start()),
                            caps.count())
                    );
                }
                SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid destination caps");
            }

            if(obtain) {
                Capability *srvcapsPtr[srvcapsCount];
                for(size_t i = 0; i < srvcapsCount; i++) {
                    srvcapsPtr[i] = Capability::createFromStream(reply);
                }
                CapTable &dsttab = caps.type() == m3::CapRngDesc::OBJ ? vpe->objcaps() : vpe->mapcaps();
                for(uint i = 0; i < srvcapsCount; ++i) {
                    capsel_t dstcap = caps.start() + i;
                    assert(dsttab.get(dstcap) == nullptr);
                    dsttab.obtain(dstcap, srvcapsPtr[i]);
                    delete srvcapsPtr[i];
                }
                // Note: Sending an acknowledgement to other kernel is not necessary
                //       when obtaining, since the other kernel only needs to insert
                //       the child pointers which it already did.
            }
            else {
                PANIC("Delegate not ready");
            }

            // reply to VPE
            AutoGateOStream msg(m3::vostreamsize(m3::ostreamsize<m3::Errors, m3::CapRngDesc>(),
                reply.remaining()));
            msg << m3::Errors::NO_ERROR;
            msg.put(reply);
            CAP_BENCH_TRACE_X_F(KERNEL_OBT_SYSC_RESP);
            is.reply(msg.bytes(), msg.total());
        }
        else {
            KLOG(SYSC, tvpeobj->vpe->id() << ": Remote server denied cap-transfer (" << res << ")");
            reply_vmsg(is, res);
        }
    }
}

void SyscallHandler::activate(GateIStream &is) {
    EVENT_TRACER_Syscall_activate();
    VPE *vpe = is.gate().session<VPE>();
    size_t epid;
    capsel_t oldcap, newcap;
    is >> epid >> oldcap >> newcap;
    LOG_SYS(vpe, ": syscall::activate", "(ep=" << epid << ", old=" <<
        oldcap << ", new=" << newcap << ")");

    MsgCapability *oldcapobj = oldcap == m3::KIF::INV_SEL ? nullptr : static_cast<MsgCapability*>(
            vpe->objcaps().get(oldcap, Capability::MSG | Capability::MEM));
    MsgCapability *newcapobj = newcap == m3::KIF::INV_SEL ? nullptr : static_cast<MsgCapability*>(
            vpe->objcaps().get(newcap, Capability::MSG | Capability::MEM));
    // ep 0 can never be used for sending
    if(epid == 0 || (oldcapobj == nullptr && newcapobj == nullptr)) {
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Invalid cap(s) (old=" << oldcap << "," << oldcapobj
            << ", new=" << newcap << "," << newcapobj << ")");
    }

    if(newcapobj && newcapobj->type() == Capability::MSG) {
        if(MHTInstance::getInstance().responsibleKrnl(newcapobj->obj->core) == Coordinator::get().kid()) {
            if(!RecvBufs::is_attached(newcapobj->obj->core, newcapobj->obj->epid)) {
                ReplyInfo rinfo(is.message());
                LOG_SYS(vpe, ": syscall::activate", ": waiting for receive buffer "
                    << newcapobj->obj->core << ":" << newcapobj->obj->epid << " to get attached");

                auto callback = [rinfo, vpe, epid, oldcapobj, newcapobj](bool success, m3::Subscriber<bool> *) {
                    EVENT_TRACER_Syscall_activate();
                    m3::Errors::Code res = success ? m3::Errors::NO_ERROR : m3::Errors::RECV_GONE;

                    LOG_SYS(vpe, ": syscall::activate-cb", "(res=" << res << ")");

                    if(success)
                        res = do_activate(vpe, epid, oldcapobj, newcapobj);
                    if(res != m3::Errors::NO_ERROR)
                        KLOG(ERR, vpe->name() << ": activate failed (" << res << ")");

                    auto reply = kernel::create_vmsg(res);
                    reply_to_vpe(*vpe, rinfo, reply.bytes(), reply.total());
                };
                RecvBufs::subscribe(newcapobj->obj->core, newcapobj->obj->epid, callback);
                return;
            }
        }
        else {
            // destination receive buffer is managed by another kernel
//            KLOG(SYSC, "Receive buffer @" << newcapobj->obj->core << " managed by kernel " << MHTInstance::getInstance().responsibleKrnl(newcapobj->obj->core) << " my kid: " <<
//                Coordinator::get().kid());
//            MHTInstance::getInstance().printMembership();
            Kernelcalls::get().recvBufisAttached(Coordinator::get().getKPE(
                MHTInstance::getInstance().responsibleKrnl(newcapobj->obj->core)),
                newcapobj->obj->core, newcapobj->obj->epid);
            m3::ThreadManager::get().wait_for(reinterpret_cast<void*>(
                m3::ThreadManager::get().current()->id()));

            m3::Errors::Code res = *(reinterpret_cast<m3::Errors::Code*>(
                const_cast<unsigned char*>(m3::ThreadManager::get().get_current_msg())));

            if(res != m3::Errors::NO_ERROR) {
                KLOG(ERR, vpe->name() << ": activate failed (" << res << ")");
                reply_vmsg(is, res);
                return;
            }
        }
    }

    m3::Errors::Code res = do_activate(vpe, epid, oldcapobj, newcapobj);
    if(res != m3::Errors::NO_ERROR)
        SYS_ERROR(vpe, is, res, "cmpxchg failed");
    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::revoke(GateIStream &is) {
    CAP_BENCH_TRACE_X_S(KERNEL_REV_SYSC_RCV);
    EVENT_TRACER_Syscall_revoke();
    VPE *vpe = is.gate().session<VPE>();
    m3::CapRngDesc crd;
    bool own;
    is >> crd >> own;
    LOG_SYS(vpe, ": syscall::revoke", "(" << crd << ", own=" << own << ")");

    if(crd.type() == m3::CapRngDesc::OBJ && crd.start() < 2)
        SYS_ERROR(vpe, is, m3::Errors::INV_ARGS, "Cap 0 and 1 are not revokeable");

    CapTable &table = crd.type() == m3::CapRngDesc::OBJ ? vpe->objcaps() : vpe->mapcaps();
    m3::Errors::Code res = table.revoke(crd, own);
    if(res != m3::Errors::NO_ERROR)
        SYS_ERROR(vpe, is, res, "Revoke failed");

    CAP_BENCH_TRACE_X_F(KERNEL_REV_SYSC_RESP);
    reply_vmsg(is, m3::Errors::NO_ERROR);
}

void SyscallHandler::exit(GateIStream &is) {
    EVENT_TRACER_Syscall_exit();
    VPE *vpe = is.gate().session<VPE>();
    int exitcode;
    is >> exitcode;
    LOG_SYS(vpe, ": syscall::exit", "(" << exitcode << ")");

    vpe->exit(exitcode);
    vpe->unref();

    tryTerminate();
}

void SyscallHandler::tryTerminate() {
    if(!Coordinator::get().numKPEs()) {
        PEManager::get().terminate();
    }
    else {
        Coordinator &coord = Coordinator::get();
        if(PEManager::get().used() <= PEManager::get().daemons()) {
            // if there are other kernels ask first kernel to initiate shutdown
            // if kernel 0 asked us to shut down before, we will send the acknowledgement now
            // if we are kernel 0 we ask all others to shut down
            if(coord.kid() != 0) {
                if(coord.closingRequests >= 0) {
                    if(coord.shutdownIssued && !PEManager::get().daemons()) {
                        if(PEManager::get().terminate())
                            Kernelcalls::get().shutdown(coord.getKPE(0), Kernelcalls::OpStage::KREPLY);

                    }
                    else if(coord.shutdownRequests > 0) {
                        Kernelcalls::get().requestShutdown(coord.getKPE(0), Kernelcalls::OpStage::KREPLY);
                    }
                }
                else {
                    Kernelcalls::get().requestShutdown(coord.getKPE(0), Kernelcalls::OpStage::KREQUEST);
                    coord.closingRequests++;
                }
            }
            else {
                coord.closingRequests = coord.broadcastShutdownRequest();
                if (coord.closingRequests == 0)
                    PEManager::get().terminate();
            }
        }
    }
}

void SyscallHandler::noop(GateIStream &is) {
    reply_vmsg(is, 0);
}

#if defined(__host__)
void SyscallHandler::init(GateIStream &is) {
    VPE *vpe = is.gate().session<VPE>();
    void *addr;
    is >> addr;
    vpe->activate_sysc_ep(addr);
    LOG_SYS(vpe, "syscall::init", "(" << addr << ")");

    reply_vmsg(is, m3::Errors::NO_ERROR);
}
#endif

}
