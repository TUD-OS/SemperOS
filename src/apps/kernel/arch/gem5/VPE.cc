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

#include <base/util/Sync.h>
#include <base/util/Math.h>
#include <base/log/Kernel.h>
#include <base/ELF.h>
#include <base/Panic.h>

#include "cap/Capability.h"
#include "mem/MainMemory.h"
#include "pes/VPE.h"
#include "DTU.h"
#include "Platform.h"
#include "ddl/MHTInstance.h"

namespace kernel {

size_t VPE::count = 0;
BootModule VPE::mods[Platform::MAX_MODS];
uint64_t VPE::loaded = 0;
BootModule *VPE::idles[Platform::MAX_PES];
static char buffer[4096];

BootModule *get_mod(const char *name, bool *first) {
    static_assert(sizeof(VPE::loaded) * 8 >= Platform::MAX_MODS, "Too few bits for modules");

    if(VPE::count == 0) {
        for(size_t i = 0; i < Platform::MAX_MODS && Platform::mod(i); ++i) {
            uintptr_t addr = m3::DTU::noc_to_virt(reinterpret_cast<uintptr_t>(Platform::mod(i)));
            size_t pe = m3::DTU::noc_to_pe(reinterpret_cast<uintptr_t>(Platform::mod(i)));
            DTU::get().read_mem(VPEDesc(pe, 0), addr, &VPE::mods[i], sizeof(VPE::mods[i]));

            KLOG(KENV, "Module '" << VPE::mods[i].name << "':");
            KLOG(KENV, "  addr: " << m3::fmt(VPE::mods[i].addr, "p"));
            KLOG(KENV, "  size: " << m3::fmt(VPE::mods[i].size, "p"));

            VPE::count++;
        }

        static const char *types[] = {"imem", "emem", "mem"};
        MHTInstance &mht = MHTInstance::getInstance();
        size_t pecount = Platform::pe_count();
        for(size_t i = 0; i < pecount; ++i) {
            if(mht.responsibleKrnl(Platform::pe_by_index(i).core_id()) == Platform::kernelId()) {
                KLOG(KENV, "PE" << m3::fmt(Platform::pe_by_index(i).core_id(), 2) << ": "
                    << types[static_cast<size_t>(Platform::pe_by_index(i).type())] << " "
                    << (Platform::pe_by_index(i).mem_size() / 1024) << " KiB memory");
            }
            else if(pecount < Platform::MAX_PES - 1)
                pecount++;
        }
    }

    size_t len = strlen(name);
    for(size_t i = 0; i < VPE::count; ++i) {
        if(VPE::mods[i].name[len] == ' ' || VPE::mods[i].name[len] == '\0') {
            if(strncmp(name, VPE::mods[i].name, len) == 0) {
                *first = (VPE::loaded & (1 << i)) == 0;
                VPE::loaded |= 1 << i;
                return VPE::mods + i;
            }
        }
    }
    return nullptr;
}

static uint64_t alloc_mem(size_t size) {
    MainMemory::Allocation alloc = MainMemory::get().allocate(size);
    if(!alloc)
        PANIC("Not enough memory");
    return m3::DTU::build_noc_addr(alloc.pe(), alloc.addr);
}

void read_from_mod(BootModule *mod, void *data, size_t size, size_t offset) {
    if(offset + size < offset || offset + size > mod->size)
        PANIC("Invalid ELF file");

    uintptr_t addr = m3::DTU::noc_to_virt(mod->addr + offset);
    size_t pe = m3::DTU::noc_to_pe(mod->addr + offset);
    DTU::get().read_mem(VPEDesc(pe, 0), addr, data, size);
}

void copy_clear(const VPEDesc &vpe, uintptr_t dst, uintptr_t src, size_t size, bool clear) {
    if(clear)
        memset(buffer, 0, sizeof(buffer));

    size_t rem = size;
    while(rem > 0) {
        size_t amount = m3::Math::min(rem, sizeof(buffer));
        // read it from src, if necessary
        if(!clear) {
            DTU::get().read_mem(VPEDesc(m3::DTU::noc_to_pe(src), 0),
                m3::DTU::noc_to_virt(src), buffer, amount);
        }
        DTU::get().write_mem(vpe, m3::DTU::noc_to_virt(dst), buffer, amount);
        src += amount;
        dst += amount;
        rem -= amount;
    }
}

static void map_segment(VPE &vpe, uint64_t phys, uintptr_t virt, size_t size, uint perms) {
    if(Platform::pe_by_core(vpe.core()).has_virtmem()) {
        capsel_t dst = virt >> PAGE_BITS;
        size_t pages = m3::Math::round_up(size, PAGE_SIZE) >> PAGE_BITS;
        for(capsel_t i = 0; i < pages; ++i) {
            MapCapability *mapcap = new MapCapability(&vpe.mapcaps(), dst + i, phys, perms, 0); // TODO: capid
            vpe.mapcaps().set(dst + i, mapcap);
            phys += PAGE_SIZE;
        }
    }
    else
        copy_clear(vpe.desc(), virt, phys, size, false);
}

static uintptr_t load_mod(VPE &vpe, BootModule *mod, bool copy, bool needs_heap) {
    // load and check ELF header
    m3::ElfEh header;
    read_from_mod(mod, &header, sizeof(header), 0);

    if(header.e_ident[0] != '\x7F' || header.e_ident[1] != 'E' || header.e_ident[2] != 'L' ||
        header.e_ident[3] != 'F')
        PANIC("Invalid ELF file");

    // map load segments
    uintptr_t end = 0;
    off_t off = header.e_phoff;
    for(uint i = 0; i < header.e_phnum; ++i, off += header.e_phentsize) {
        /* load program header */
        m3::ElfPh pheader;
        read_from_mod(mod, &pheader, sizeof(pheader), off);

        // we're only interested in non-empty load segments
        if(pheader.p_type != PT_LOAD || pheader.p_memsz == 0)
            continue;

        int perms = 0;
        if(pheader.p_flags & PF_R)
            perms |= m3::DTU::PTE_R;
        if(pheader.p_flags & PF_W)
            perms |= m3::DTU::PTE_W;
        if(pheader.p_flags & PF_X)
            perms |= m3::DTU::PTE_X;

        uintptr_t offset = m3::Math::round_dn(pheader.p_offset, PAGE_SIZE);
        uintptr_t virt = m3::Math::round_dn(pheader.p_vaddr, PAGE_SIZE);

        // do we need new memory for this segment?
        if((copy && (perms & m3::DTU::PTE_W)) || pheader.p_filesz == 0) {
            // allocate memory
            size_t size = m3::Math::round_up((pheader.p_vaddr & PAGE_BITS) + pheader.p_memsz, PAGE_SIZE);
            uintptr_t phys = alloc_mem(size);

            // map it
            map_segment(vpe, phys, virt, size, perms);
            end = virt + size;

            copy_clear(vpe.desc(), virt, mod->addr + offset, size, pheader.p_filesz == 0);
        }
        else {
            assert(pheader.p_memsz == pheader.p_filesz);
            size_t size = (pheader.p_offset & PAGE_BITS) + pheader.p_filesz;
            map_segment(vpe, mod->addr + offset, virt, size, perms);
            end = virt + size;
        }
    }

    if(needs_heap) {
        // create initial heap
        uintptr_t phys = alloc_mem(MOD_HEAP_SIZE);
        map_segment(vpe, phys, m3::Math::round_up(end, PAGE_SIZE), MOD_HEAP_SIZE, m3::DTU::PTE_RW);
    }

    return header.e_entry;
}

void map_idle(VPE &vpe) {
    BootModule *idle = VPE::idles[vpe.core()];
    if(!idle) {
        bool first;
        BootModule *tmp = get_mod("idle", &first);
        idle = new BootModule;

        // copy the ELF file
        size_t size = m3::Math::round_up(tmp->size, PAGE_SIZE);
        uintptr_t phys = alloc_mem(size);
        copy_clear(VPEDesc(m3::DTU::noc_to_pe(phys), 0), phys, tmp->addr, tmp->size, false);

        // remember the copy
        strcpy(idle->name, "idle");
        idle->addr = phys;
        idle->size = tmp->size;
        VPE::idles[vpe.core()] = idle;
    }
    if(!idle)
        PANIC("Unable to find boot module 'idle'");

    // load idle
    load_mod(vpe, idle, false, false);
}

void map_idle(const VPEDesc &vpe, KernelAllocation& kernMem, bool vm) {
    // Don't store idles of remote kernels
    bool first;
    BootModule *tmp = get_mod("idle", &first);

    // copy the ELF file
    size_t size = m3::Math::round_up(tmp->size, PAGE_SIZE);
    if(vm) {
        uintptr_t phys = kernMem.alloc_elf_segment(size);
        copy_clear(VPEDesc(m3::DTU::noc_to_pe(phys), 0), phys, tmp->addr, tmp->size, false);
    }

    if(!tmp)
        PANIC("Unable to find boot module 'idle'");

    // load idle
    load_kernel_mod(vpe, tmp, false, false, kernMem, vm);
}

void VPE::init_memory(int argc, const char *argv) {
    if(_flags & MEMINIT)
        return;
    _flags |= MEMINIT;

    bool vm = Platform::pe_by_core(core()).has_virtmem();

    if(_flags & BOOTMOD) {
        bool appFirst;
        BootModule *mod = get_mod(argv, &appFirst);
        if(!mod)
            PANIC("Unable to find boot module '" << argv << "'");

        KLOG(KENV, "Loading mod '" << mod->name << "':");

        if(vm) {
            DTU::get().config_pf_remote(desc(), address_space()->root_pt(), _syscEP);

            // map runtime space
            uintptr_t virt = RT_START;
            uintptr_t phys = alloc_mem(STACK_TOP - virt);
            map_segment(*this, phys, virt, STACK_TOP - virt, m3::DTU::PTE_RW);
        }

        // load app
        uint64_t entry = load_mod(*this, mod, !appFirst, true);

        // copy arguments and arg pointers to buffer
        char **argptr = (char**)buffer;
        char *args = buffer + argc * sizeof(char*);
        char c;
        size_t i, off = args - buffer, argscopied = 0;
        *argptr++ = (char*)(RT_SPACE_START + off);
        for(i = 0; i < sizeof(buffer) && ((c = argv[i]) || (argscopied < static_cast<size_t>(argc) - 1)); ++i) {
            if(c == '\0') {
                args[i] = '\0';
                *argptr++ = (char*)(RT_SPACE_START + off + i + 1);
                argscopied++;
            }
            else {
                // Hack for fstrace arguments:
                // We start many instances with the kernel parameter repeat=xx
                // but each instance needs its own prefix, which we need to manipulate here manually
                if((argscopied == static_cast<size_t>(argc) - 1)
                    && (_replaceLastArg.length() != 0)
                    && (strncmp(mod->name, "fstrace-m3fs", sizeof("fstrace-m3fs") - 1) == 0))
                    break;

                args[i] = c;
            }
        }

        // Hack for fstrace arguments
        if(strncmp(mod->name, "fstrace-m3fs", sizeof("fstrace-m3fs") - 1) == 0) {
            for(size_t j = 0; i < sizeof(buffer) && (c = *(_replaceLastArg.c_str() + j)); ++i) {
                if(c == '\0') {
                    args[i] = '\0';
                    *argptr++ = (char*)(RT_SPACE_START + off + i + 1);
                    argscopied++;
                }
                else
                    args[i] = c;

                j++;
            }
        }

        if(i == sizeof(buffer))
            PANIC("Not enough space for arguments");

        // write buffer to the target PE
        size_t argssize = m3::Math::round_up(off + i, DTU_PKG_SIZE);
        DTU::get().write_mem(desc(), RT_SPACE_START, buffer, argssize);

        // write env to targetPE
        m3::Env senv;
        memset(&senv, 0, sizeof(senv));

        senv.argc = argc;
        senv.argv = reinterpret_cast<char**>(RT_SPACE_START);
        senv.sp = STACK_TOP - sizeof(word_t);
        senv.entry = entry;
        senv.pe = Platform::pe_by_core(core());
        if(!vm)
            senv.heapsize = 0;
        else
            senv.heapsize = MOD_HEAP_SIZE;
        // HACK
        // load generator PEs get additional space for receive buffers
        if(name().contains("loadgen"))
            senv.secondaryrecvbuf = SECONDARY_RECVBUF_SIZE_SPM;
        else
            senv.secondaryrecvbuf = 0;

        DTU::get().write_mem(desc(), RT_START, &senv, sizeof(senv));
    }

    map_idle(*this);

    if(vm) {
        // map receive buffer
        uintptr_t phys = alloc_mem(RECVBUF_SIZE);
        map_segment(*this, phys, RECVBUF_SPACE, RECVBUF_SIZE, m3::DTU::PTE_RW);
    }
    DTU::get().set_rw_barrier(desc(), Platform::rw_barrier(core()) -
        /* HACK: load generator PEs get additional space for receive buffers */
        (name().contains(("loadgen")) ? SECONDARY_RECVBUF_SIZE_SPM : 0));
}

}
