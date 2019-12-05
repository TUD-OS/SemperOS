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

#include <base/util/Sync.h>
#include <base/Config.h>
#include <base/ELF.h>
#include <c/div.h>

#include "DTU.h"
#include "pes/KPE.h"
#include "Platform.h"
#include "Coordinator.h"
#include "pes/VPE.h"
#include "ddl/MHTInstance.h"

namespace kernel {

static char buffer[PAGE_SIZE*4];

static void map_kernel_segment(const VPEDesc &vpe, uint64_t phys, uintptr_t virt, size_t size, uint perms, KernelAllocation& kernMem) {
    if(Platform::pe_by_core(vpe.core).has_virtmem()) {
        size_t pages = m3::Math::round_up(size, PAGE_SIZE) >> PAGE_BITS;
        for(capsel_t i = 0; i < pages; ++i) {
            DTU::get().map_kernel_page(vpe, (virt + PAGE_SIZE * i) & (~PAGE_MASK), phys, perms, kernMem);
            phys += PAGE_SIZE;
        }
    }
    else
        copy_clear(vpe, virt, phys, size, false);
}

static uint max_req_ptes(BootModule *mod) {
    // PTEs for stack and RT
    uint ptes = 0;
    uint rem = 0;
    ptes += divide(STACK_TOP - RT_START, PAGE_SIZE, &rem);
    if(rem)
        ptes++;

    // load and check ELF header
    m3::ElfEh header;
    read_from_mod(mod, &header, sizeof(header), 0);

    // determine maximum PTEs we need for mapping the binary
    off_t off = header.e_phoff;
    for(uint i = 0; i < header.e_phnum; ++i, off += header.e_phentsize) {
        /* load program header */
        m3::ElfPh pheader;
        read_from_mod(mod, &pheader, sizeof(pheader), off);

        if(pheader.p_type != PT_LOAD || pheader.p_memsz == 0)
            continue;

        uint rem = 0;
        ptes += divide(pheader.p_memsz, PAGE_SIZE, &rem);
        if(rem)
            ptes++;
    }

    ptes += divide(KRNL_HEAP_SIZE + KENV_SIZE, PAGE_SIZE, &rem);
    if(rem)
        ptes++;
    return ptes;
}

uintptr_t load_kernel_mod(const VPEDesc &vpe, BootModule *mod, bool copy, bool needs_heap,
        KernelAllocation& kernMem, bool vm) {
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
            if(vm) {
                uintptr_t phys = kernMem.alloc_elf_segment(size);
                // map it
                map_kernel_segment(vpe, phys, virt, size, perms, kernMem);
            }
            end = virt + size;

            copy_clear(vpe, virt, mod->addr + offset, size, pheader.p_filesz == 0);
        }
        else {
            assert(pheader.p_memsz == pheader.p_filesz);
            size_t size = (pheader.p_offset & PAGE_BITS) + pheader.p_filesz;
            map_kernel_segment(vpe, mod->addr + offset, virt, size, perms, kernMem);
            end = virt + size;
        }
        if(!vm)
            kernMem.elf_segments_end = end;
    }

    if(needs_heap && vm) {
        // create initial heap
        kernMem.alloc_heap(KRNL_HEAP_SIZE);
        // check if heap fits into initial memory module
        assert(((kernMem.heap + KRNL_HEAP_SIZE) - m3::DTU::build_noc_addr(kernMem.mem_mods[1]->pe(), kernMem.mem_mods[1]->addr()))
                < kernMem.mem_mods[1]->size());
        uintptr_t phys = kernMem.heap;
        map_kernel_segment(vpe, phys, m3::Math::round_up(end, PAGE_SIZE), KRNL_HEAP_SIZE, m3::DTU::PTE_RW, kernMem);
    }

    return header.e_entry;
}

void KPE::init_memory(int argc, char **argv, size_t pe_count, m3::PEDesc PEs[]) {
    static_assert(RT_SIZE > sizeof(m3::Env),
            "Runtime space too small for kernel environment");
    static_assert(KENV_SIZE > sizeof(Platform::KEnv),
            "Kernel enviroment space to small");

    // memory layout:
    // PTEs | KENV, memberTable, partitions | RT | Elf_segments | Initial heap |

    bool vm = Platform::pe_by_core(core()).has_virtmem();

    KernelAllocation kernMem;
    // get memory for the kernel
    MemoryModule mem = MainMemory::get().detach(KRNL_INIT_MEM_SIZE);
    if(!mem.size())
        PANIC("No memory available for new kernel");
    // the first mod keeps fs image and stuff
    const MemoryModule& fsmod = MainMemory::get().module(0);
    MemoryModule memfs(fsmod.available(), fsmod.pe(), fsmod.addr(), fsmod.size());
    kernMem.mem_mods[0] = &memfs;
    kernMem.mem_mods[1] = &mem;

    if(vm)
        kernMem.set_root_pt(m3::DTU::build_noc_addr(mem.pe(), mem.addr()));

    size_t off;
    // we assume the kernel is present as a bootmodule
    bool appFirst;
    BootModule *mod = get_mod(argv[0], &appFirst);
    if(!mod)
        PANIC("Unable to find kernel boot module '" << argv[0] << "'");

    KLOG(KPES, "Loading KPE mod '" << mod->name << "':");

    if(vm) {
        // allocate space for PTEs
        uint ptes = max_req_ptes(mod);
        kernMem.set_max_ptes(ptes);
        DTU::get().config_pt_remote(VPEDesc(core(), VPE::INVALID_ID), kernMem.rootpt);

        // map runtime space
        uintptr_t virt = RT_START;
        map_kernel_segment(VPEDesc(core(), VPE::INVALID_ID), kernMem.rtspace, virt, STACK_TOP - virt, m3::DTU::PTE_RW, kernMem);
        kernMem.elf_segments = m3::Math::round_up<uintptr_t>(kernMem.rtspace + (STACK_TOP - virt), PAGE_SIZE);
        kernMem.elf_segments_end = kernMem.elf_segments;
    }
    else {
        kernMem.elf_segments = m3::DTU::build_noc_addr(_core, STACK_TOP);
        kernMem.elf_segments_end = kernMem.elf_segments;
    }

    // map_idle
    map_idle(VPEDesc(core(), VPE::INVALID_ID), kernMem, vm);

    // load app
    uint64_t entry = load_kernel_mod(VPEDesc(core(), VPE::INVALID_ID), mod, !appFirst, true, kernMem, vm);

    // copy arguments and arg pointers to buffer
    memset(buffer, 0, PAGE_SIZE);
    char **argptr = (char**)buffer;
    char *args = buffer + argc * sizeof(char*);
    size_t i;
    off = args - buffer;
    int written = 0;
    *argptr++ = (char*)(RT_SPACE_START + off);
    for(i = 0; i < sizeof(buffer) && written < argc; ++i) {
        args[i] = argv[0][i];
        if(argv[0][i] == '\0') {
            written++;
            if(written < argc)
                *argptr++ = (char*)(RT_SPACE_START + off + i + 1);
        }
    }
    if(i == sizeof(buffer))
        PANIC("Not enough space for arguments");

    // write buffer to the target PE
    size_t argssize = m3::Math::round_up(off + i, DTU_PKG_SIZE);
    assert((sizeof(m3::Env) + argssize) <= RT_SIZE);
    if(argssize)
        DTU::get().write_mem(VPEDesc(core(), VPE::INVALID_ID), RT_SPACE_START, buffer, argssize);

    // write env to targetPE
    m3::Env senv;
    memset(&senv, 0, sizeof(senv));

    senv.coreid = (uint64_t)core();
    senv.argc = argc;
    senv.argv = reinterpret_cast<char**>(RT_SPACE_START);
    senv.sp = STACK_TOP - sizeof(word_t);
    senv.entry = entry;
    senv.pe = Platform::pe_by_core(core());

    if(vm)
        senv.heapsize = KRNL_HEAP_SIZE;
    else {
        // if this PE has SPM spend the remaining memory for the heap
        senv.heapsize = Platform::pe_by_core(core()).mem_size() - kernMem.elf_segments_end;
        kernMem.reserve_kenv(m3::DTU::build_noc_addr(kernMem.mem_mods[1]->pe(), kernMem.mem_mods[1]->addr()));
    }

    senv.kenv = kernMem.kenv;

    DTU::get().write_mem(VPEDesc(core(), VPE::INVALID_ID), RT_START, &senv, sizeof(senv));

    // set up KEnv for the remote kernel
    // Note: We do not include the memory modules in the number of PEs, since the partition
    //       of a memory PE is handled by exactly one kernel, hence the others do not own it.
    Platform::KEnv *kenv = new Platform::KEnv(Platform::mods(), pe_count, PEs, (uint32_t)id(), (uint32_t)(Coordinator::get().kid()),
        Platform::kernel_pe(), m3::ThreadManager::get().current()->id(), _localEP, kernMem.mem_mods, kernMem.offset);

    // copy memberTable and DDL partitions belonging to the PEs of the new kernel
    MHTInstance &mht = MHTInstance::getInstance();
    kenv->memberTable = m3::Math::round_up<uintptr_t>(senv.kenv + sizeof(Platform::KEnv), DTU_PKG_SIZE);
    kenv->ddlPartitions = m3::Math::round_up<uintptr_t>(kenv->memberTable + sizeof(membership_entry) * (1UL << PE_BITS), DTU_PKG_SIZE);

    DTU::get().write_mem(VPEDesc(0, 0), m3::DTU::noc_to_virt(kenv->memberTable),
        mht.memberTable, m3::Math::round_up<size_t>(sizeof(membership_entry) * (1UL << PE_BITS), DTU_PKG_SIZE));

    // calculate size necessary for migrating DDL partitions
    MHTPartition *partitions[pe_count];
    size_t partsFound = 0;
    size_t partitionSize = 0;
    for(size_t i = 0; i < pe_count; i++) {
        // collect partitions, we can't use findPartition here anymore, since the
        // membership table has already been updated to point to this kernel for the partitions.
        // we can, however, find the partitions in our local store since we only
        // transfer partitions that were owned by us
        bool found = false;
        for(auto it = mht.partitions.begin(); it != mht.partitions.end(); it++) {
            if(it->partition->_id == PEs[i].core_id()) {
                partitions[partsFound] = it->partition;
                found = true;
                continue;
            }
        }
        if(!found)
            PANIC("Partition to migrate is not store locally");
        partitionSize += partitions[partsFound]->serializedSize();
        partsFound++;
    }
    assert(sizeof(Platform::KEnv) + partitionSize + (sizeof(membership_entry) * (1UL << PE_BITS)) <= KENV_SIZE);

    unsigned char *partitionStore = static_cast<unsigned char*>(m3::Heap::alloc(partitionSize));
    if(!partitionStore)
        PANIC("No memory to serialize DDL partitions");
    GateOStream ser(partitionStore, partitionSize);
    for(size_t i = 0; i < pe_count; i++) {
        partitions[i]->serialize(ser);
    }
    assert(ser.total() == partitionSize);
    kenv->ddlPartitionsSize = m3::Math::round_up<size_t>(ser.total(), DTU_PKG_SIZE);
    DTU::get().write_mem(VPEDesc(0, 0), m3::DTU::noc_to_virt(kenv->ddlPartitions),
        ser.bytes(), kenv->ddlPartitionsSize);

    DTU::get().write_mem(VPEDesc(0, 0), m3::DTU::noc_to_virt(senv.kenv),
        kenv, sizeof(Platform::KEnv));
}

}
