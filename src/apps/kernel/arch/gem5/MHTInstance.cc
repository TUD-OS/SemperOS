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

#include "ddl/MHTInstance.h"
#include "base/util/Profile.h"

namespace kernel {

MHTInstance::MHTInstance(uint64_t memberTab, uint64_t parts, size_t partsSize)
    : partitions(), _migratingPartitions()
{
    memberTable = new membership_entry[Platform::MAX_PES];
    if(!memberTable)
        PANIC("Not enought memory to create membership table");
    membership_entry::krnl_id_t kid = Platform::kernelId();
    DTU::get().read_mem(VPEDesc(0, 0), m3::DTU::noc_to_virt(reinterpret_cast<uintptr_t>(memberTab)),
        memberTable, sizeof(membership_entry) * (1UL << PE_BITS));

    void *partitionContent = m3::Heap::alloc(partsSize);
    DTU::get().read_mem(VPEDesc(0, 0), m3::DTU::noc_to_virt(reinterpret_cast<uintptr_t>(parts)),
        partitionContent, partsSize);
    m3::Unmarshaller input(const_cast<const unsigned char*>(reinterpret_cast<unsigned char*>(partitionContent)), partsSize);
    for(size_t i = 0; i < (1L << PE_BITS); i++) {
        if(memberTable[i].krnl_id == kid) {
            MHTPartition *part = new MHTPartition(i);
            part->deserialize(input);
            partitions.append(new PartitionEntry(part));
            // remove the MIGRATING status of partitions
            memberTable[i].flags = static_cast<MembershipFlags>(
                static_cast<uint8_t>(memberTable[i].flags) &
                ~(MembershipFlags::MIGRATING));
        }
    }
    m3::Heap::free(partitionContent);
}
}
