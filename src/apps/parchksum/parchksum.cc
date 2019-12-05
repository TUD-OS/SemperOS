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

#include <base/stream/IStringStream.h>

#include <m3/com/MemGate.h>
#include <m3/com/SendGate.h>
#include <m3/com/RecvGate.h>
#include <m3/com/GateStream.h>
#include <m3/stream/Standard.h>
#include <m3/VPE.h>

using namespace m3;

struct Worker {
    MemGate submem;
    SendGate sgate;
    VPE vpe;

    Worker(RecvGate &rgate, MemGate &mem, size_t offset, size_t size)
            : submem(mem.derive(offset, size)),
              sgate(SendGate::create(DTU_PKG_SIZE + DTU::HEADER_SIZE, &rgate)), vpe("worker") {
        vpe.delegate_obj(submem.sel());
        vpe.fds(*VPE::self().fds());
    }
};

static const size_t BUF_SIZE    = 4096;

int main(int argc, char **argv) {
    size_t memPerVPE = 1024 * 1024;
    int vpes = 2;
    if(argc > 1)
        vpes = IStringStream::read_from<int>(argv[1]);
    if(argc > 2)
        memPerVPE = IStringStream::read_from<size_t>(argv[2]);

    const size_t MEM_SIZE    = vpes * memPerVPE;
    const size_t SUBMEM_SIZE = MEM_SIZE / vpes;

    RecvBuf rbuf = RecvBuf::create(VPE::self().alloc_ep(),
        getnextlog2(vpes * (DTU_PKG_SIZE + DTU::HEADER_SIZE)), 0);
    RecvGate rgate = RecvGate::create(&rbuf);

    MemGate mem = MemGate::create_global(MEM_SIZE, MemGate::RW);

    // create worker
    Worker **worker = new Worker*[vpes];
    for(int i = 0; i < vpes; ++i) {
        worker[i] = new Worker(rgate, mem, i * SUBMEM_SIZE, SUBMEM_SIZE);
        if(Errors::last != Errors::NO_ERROR)
            exitmsg("Unable to create worker");
    }

    // write data into memory
    for(int i = 0; i < vpes; ++i) {
        MemGate &vpemem = worker[i]->submem;
        worker[i]->vpe.run([&vpemem, SUBMEM_SIZE] {
            uint *buffer = new uint[BUF_SIZE / sizeof(uint)];
            size_t rem = SUBMEM_SIZE;
            size_t offset = 0;
            while(rem > 0) {
                for(size_t i = 0; i < BUF_SIZE / sizeof(uint); ++i)
                    buffer[i] = i;
                vpemem.write_sync(buffer, BUF_SIZE, offset);
                offset += BUF_SIZE;
                rem -= BUF_SIZE;
            }
            cout << "Memory initialization of " << SUBMEM_SIZE << " bytes finished\n";
            return 0;
        });
    }

    // wait for all workers
    for(int i = 0; i < vpes; ++i)
        worker[i]->vpe.wait();

    // now build the checksum
    for(int i = 0; i < vpes; ++i) {
        worker[i]->vpe.delegate_obj(worker[i]->sgate.sel());
        MemGate &vpemem = worker[i]->submem;
        SendGate &vpegate = worker[i]->sgate;
        worker[i]->vpe.run([&vpemem, &vpegate, SUBMEM_SIZE] {
            uint *buffer = new uint[BUF_SIZE / sizeof(uint)];

            uint checksum = 0;
            size_t rem = SUBMEM_SIZE;
            size_t offset = 0;
            while(rem > 0) {
                vpemem.read_sync(buffer, BUF_SIZE, offset);
                for(size_t i = 0; i < BUF_SIZE / sizeof(uint); ++i)
                    checksum += buffer[i];
                offset += BUF_SIZE;
                rem -= BUF_SIZE;
            }

            cout << "Checksum for sub area finished\n";
            send_vmsg(vpegate, checksum);
            return 0;
        });
    }

    // reduce
    uint checksum = 0;
    for(int i = 0; i < vpes; ++i) {
        uint vpechksum;
        receive_vmsg(rgate, vpechksum);
        checksum += vpechksum;
    }

    cout << "Checksum: " << checksum << "\n";

    for(int i = 0; i < vpes; ++i) {
        worker[i]->vpe.wait();
        delete worker[i];
    }
    return 0;
}
