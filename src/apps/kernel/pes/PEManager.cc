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

#include <base/stream/OStringStream.h>
#include <base/stream/IStringStream.h>
#include <base/log/Kernel.h>
#include <base/Panic.h>
#include <base/col/SList.h>

#include <string.h>

#include "pes/PEManager.h"
#include "Platform.h"
#include "ddl/MHTInstance.h"

namespace kernel {

bool PEManager::_shutdown = false;
PEManager *PEManager::_inst;

PEManager::PEManager() :
        _vpes(new VPE*[Platform::MAX_PES]()),
        _count(), _daemons(), _pending() {
    // initial kernel acts differently
    if(Platform::kernelId() == Platform::creatorKernelId()) {
        deprivilege_pes();
        Coordinator::create(Platform::kernelId());
        MHTInstance::create();
        MHTInstance::getInstance().printMembership();
    }
    else {
        unsigned int creatorid = Platform::creatorKernelId();
        Coordinator::create(Platform::kernelId(), m3::String("kernel"), creatorid, creatorid + 1);
        // create DDL with the information given by the creator
        MHTInstance::create(Platform::ddl_member_table(), Platform::ddl_partitions(), Platform::ddl_partitions_size());
        MHTInstance::getInstance().printMembership();
    }
}

void PEManager::load(int argc, char **argv) {
    char const *separator;
    if(Platform::kernelId() == Platform::creatorKernelId())
        separator = "--";
    else
        separator = "++";

    size_t kids = 1;
    int repeat = 1;
    for(int i = 0; i < argc; ++i) {
        repeat = 1;
        if(strcmp(argv[i], separator) == 0)
            continue;

        // start multiple instances of an application
        for(int k = 0; k < repeat; k++) {
            size_t no = free_compute_core();
            if(no == Platform::MAX_PES)
                PANIC("No free cores left");
            // for idle or kernels, don't create a VPE
            if(strcmp(argv[i], "idle") != 0 && strcmp(argv[i], "kernel") != 0) {
                // allow multiple applications with the same name
                m3::OStringStream name;
                name << path_to_name(m3::String(argv[i]), nullptr).c_str() << "-" << no;
                int ep = SyscallHandler::get().reserve_ep(no);
                if(ep == -1) {
                    KLOG(ERR, "Not enough message slots for VPE " << no << ". Messages might get lost");
                    ep = m3::DTU::SYSC_EP + DTU::SYSC_GATES - 1;
                }
                _vpes[no] = new VPE(m3::String(name.str()), no, no, true, ep);
                _count++;
            }

            // find end of arguments
            bool daemon = false;
            bool karg = false;
            int j = i + 1, end = i + 1, pes = -1;
            for(; j < argc; ++j) {
                if(strncmp(argv[j], "pes=", sizeof("pes=") - 1) == 0) {
                    // 'pes=' argument shall only be used to start another kernel
                    if((strcmp(argv[i], "kernel") != 0))
                        PANIC("Illegal argument pes=...");
                    m3::String strTmp(argv[j] + sizeof("pes=") - 1);
                    m3::IStringStream numPesArg(strTmp);
                    numPesArg >> pes;
                }
                else if(strcmp(argv[i], "kernel") == 0 && strcmp(argv[j], separator) != 0) {
                    end++;
                }
                else if(strcmp(argv[j], "daemon") == 0) {
                    daemon = true;
                    _vpes[no]->make_daemon();
                    karg = true;
                }
                else if(strncmp(argv[j], "requires=", sizeof("requires=") - 1) == 0) {
                     _vpes[no]->add_requirement(argv[j] + sizeof("requires=") - 1);
                    karg = true;
                }
                else if(strncmp(argv[j], "repeat=", sizeof("repeat=") - 1) == 0) {
                    // 'repeat=' argument shall only be used to start multiple app instances
                    // without causing too long module name for the second kernel
                    if((strcmp(argv[i], "kernel") != 0)) {
                        m3::String strTmp(argv[j] + sizeof("repeat=") - 1);
                        m3::IStringStream repeatArg(strTmp);
                        repeatArg >> repeat;
                    }
                }
                else if(strcmp(argv[j], separator) == 0)
                    break;
                else if(karg && (strcmp(argv[i], "kernel") != 0))
                    PANIC("Kernel argument before program argument");
                else
                    end++;
            }

            if(strcmp(argv[i], "kernel") == 0) {
                // at the moment only the initial kernel is allowed to start other kernels
                // to keep the kernel IDs consistent
                assert((Platform::kernelId() == Platform::creatorKernelId()));
                // release some PEs for the new kernel; default: just the one the kernel runs on
                if(pes == -1)
                    pes = 1;
                // position the kernel at the back of the free cores
                no = free_compute_core_from_back(pes - 1);
                if(no == Platform::MAX_PES)
                    PANIC("No free cores left");
                Coordinator::get().startKrnl(end - i, &argv[i], m3::String(argv[i]), kids++, no, pes);
            }
            // start it, or register pending item
            else if(strcmp(argv[i], "idle") != 0) {
                // Hack for fstrace arguments
                if(k != 0 && strncmp(_vpes[no]->name().c_str(), "fstrace-m3fs", sizeof("fstrace-m3fs") - 1) == 0) {
                    m3::String lastArgStr(argv[end - 1] + 1);
                    int lastArgInt;
                    m3::IStringStream tmpLastArg(lastArgStr);
                    tmpLastArg >> lastArgInt;
                    m3::OStringStream newLastArg;
                    lastArgInt += k;
                    newLastArg << "/" << lastArgInt;
                    lastArgStr.reset(newLastArg.str(), newLastArg.length());
                    _vpes[no]->replace_last_arg(lastArgStr);
                }
                if(_vpes[no]->requirements().length() == 0 && daemon)
                    _vpes[no]->start(end - i, argv + i, 0);
                else
                    _pending.append(new Pending(_vpes[no], end - i, argv + i));
            }

            if(k == repeat - 1)
                i = j;
            if(daemon)
                _daemons++;
        }
    }

    // check for pending requirements
    start_pending(ServiceList::get(), RemoteServiceList::get());

    #ifdef KERNEL_TESTS
    if(Platform::kernelId() == Platform::creatorKernelId()) {
        Coordinator::get().finishStartup();
    }
    #endif
}

void PEManager::start_pending(ServiceList &serv, RemoteServiceList &rsrv) {
#ifdef SYNC_APP_START
#ifdef CASCADING_APP_START
    // start only the srvtrace apps
    for(auto it = _pending.begin(); it != _pending.end(); ) {
        bool fullfilled = true;
        for(auto &r : it->vpe->requirements()) {
            if(!serv.find(r.name) && !rsrv.exists(r.name)) {
                fullfilled = false;
                break;
            }
        }

        if(fullfilled && it->vpe->name().contains("srvtrace")) {
            auto old = it++;
            old->vpe->start(old->argc, old->argv, 0);
            _pending.remove(&*old);
            delete &*old;
        }
        else
            it++;
    }
#endif
    if(!Coordinator::get().startSignSent) {
        if(!Coordinator::get().kid()) {
            if(!open_requirements(ServiceList::get(), RemoteServiceList::get())
                && Coordinator::get().startSignsAwaited == 0) {
                Coordinator::get().startSignSent = true;
                Coordinator::get().broadcastStartApps();
            }
            else
                return;
        }
        else {
            // secondary kernels issue the startApps kernelcall to signal
            // that they are done setting up their services
            if(!open_requirements(ServiceList::get(), RemoteServiceList::get())) {
                Coordinator::get().startSignSent = true;
                Kernelcalls::get().startApps(Coordinator::get().getKPE(0));
            }
        }

    }

    if(Coordinator::get().startSignsAwaited != 0)
        return;
#endif
    for(auto it = _pending.begin(); it != _pending.end(); ) {
        bool fullfilled = true;
        for(auto &r : it->vpe->requirements()) {
            if(!serv.find(r.name) && !rsrv.exists(r.name)) {
                fullfilled = false;
                break;
            }
        }

        if(fullfilled) {
            auto old = it++;
            old->vpe->start(old->argc, old->argv, 0);
            _pending.remove(&*old);
            delete &*old;
        }
        else
            it++;
    }
}

uint PEManager::open_requirements(ServiceList &serv, RemoteServiceList &rsrv) {
    uint openReqs = 0;
    for(auto it = _pending.begin(); it != _pending.end(); it++)
        for(auto &r : it->vpe->requirements())
            if(!serv.find(r.name) && !rsrv.exists(r.name))
                openReqs++;

    return openReqs;
}

void PEManager::shutdown() {
    if(_shutdown)
        return;

    _shutdown = true;
    ServiceList &serv = ServiceList::get();
    for(auto &s : serv) {
        m3::Reference<Service> ref(&s);
        AutoGateOStream msg(m3::ostreamsize<m3::KIF::Service::Command>());
        msg << m3::KIF::Service::SHUTDOWN;
        KLOG(SERV, "Sending SHUTDOWN message to " << ref->name());
        serv.send_and_receive(ref, msg.bytes(), msg.total(), msg.is_on_heap());
        msg.claim();
    }
}

bool PEManager::terminate() {
    // check if we're waiting for replies from other kernels
    for(auto it = Coordinator::get().getKPEList().begin(); it != Coordinator::get().getKPEList().end(); it++)
        if(it->val->waitingThreads() != 0)
            return false;
    // if there are no VPEs left, we can stop everything
    if(_count == 0) {
        destroy();
        m3::env()->workloop()->stop();
#ifdef KERNEL_STATISTICS
        KLOG(INFO, "Kernel # " << Coordinator::get().kid() << " msgs (total/delay): normal= "
            << KPE::normalMsgs + KPE::delayedNormalMsgs << "/" << KPE::delayedNormalMsgs <<
            " revocation= " << KPE::revocationMsgs + KPE::delayedRevocationMsgs
            << "/" << KPE::delayedRevocationMsgs << " replies= " << KPE::replies + KPE::delayedReplies
            << "/" << KPE::delayedReplies);
#endif
        return true;
    }
    // if there are only daemons left, start the shutdown-procedure
    else if(_count == _daemons)
        shutdown();

    return false;
}


m3::String PEManager::path_to_name(const m3::String &path, const char *suffix) {
    static char name[256];
    strncpy(name, path.c_str(), sizeof(name));
    name[sizeof(name) - 1] = '\0';
    m3::OStringStream os;
    char *start = name;
    size_t len = strlen(name);
    for(ssize_t i = len - 1; i >= 0; --i) {
        if(name[i] == '/') {
            start = name + i + 1;
            break;
        }
    }

    os << start;
    if(suffix)
        os << "-" << suffix;
    return os.str();
}

VPE *PEManager::create(m3::String &&name, const m3::PEDesc &pe, int ep, capsel_t pfgate) {
    size_t i = free_core(pe);
    if(i == Platform::MAX_PES)
        return nullptr;

    // a pager without virtual memory support, doesn't work
    if(!Platform::pe_by_core(i).has_virtmem() && pfgate != m3::KIF::INV_SEL)
        return nullptr;

    int syscEP = SyscallHandler::get().reserve_ep(i);
    if(syscEP == -1) {
        KLOG(ERR, "Not enough message slots for VPE " << i << ". Messages might get lost");
        syscEP = m3::DTU::SYSC_EP + DTU::SYSC_GATES - 1;
    }

    _vpes[i] = new VPE(std::move(name), i, i, false, syscEP, ep, pfgate);
    _count++;
    return _vpes[i];
}

void PEManager::remove(int id, bool daemon) {
    assert(_vpes[id]);
    delete _vpes[id];
    _vpes[id] = nullptr;

    if(daemon) {
        assert(_daemons > 0);
        _daemons--;
    }

    assert(_count > 0);
    _count--;
}

}
