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

#include <base/Env.h>

#include <m3/com/MemGate.h>
#include <m3/com/RecvBuf.h>
#include <m3/stream/Standard.h>

#include <sys/mman.h>

#include "Commands.h"

using namespace m3;

static void *map_page() {
    void *addr = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,-1,0);
    if(addr == MAP_FAILED) {
        exitmsg("mmap failed. Skipping test.");
        return nullptr;
    }
    return addr;
}
static void unmap_page(void *addr) {
    munmap(addr, 0x1000);
}

void CommandsTestSuite::ReadCmdTestCase::run() {
    const size_t rcvepid = 3;
    const size_t sndepid = 4;
    DTU &dtu = DTU::get();
    RecvBuf buf = RecvBuf::create(sndepid, nextlog2<256>::val, nextlog2<128>::val, 0);
    // only necessary to set the msgqid
    RecvBuf rbuf = RecvBuf::create(rcvepid, nextlog2<64>::val, RecvBuf::NO_RINGBUF);

    void *addr = map_page();
    if(!addr)
        return;

    const size_t datasize = sizeof(word_t) * 4;
    word_t *data = reinterpret_cast<word_t*>(addr);
    data[0] = 1234;
    data[1] = 5678;
    data[2] = 1122;
    data[3] = 3344;

    cout << "-- Test errors --\n";
    {
        dtu.configure(sndepid, reinterpret_cast<word_t>(data) | MemGate::R, env()->coreid,
            rcvepid, datasize);

        dmacmd(nullptr, 0, sndepid, 0, datasize, DTU::WRITE);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);

        dmacmd(nullptr, 0, sndepid, 0, datasize + 1, DTU::READ);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);

        dmacmd(nullptr, 0, sndepid, datasize, 0, DTU::READ);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);

        dmacmd(nullptr, 0, sndepid, sizeof(word_t), datasize, DTU::READ);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);
    }

    cout << "-- Test reading --\n";
    {
        dtu.configure(sndepid, reinterpret_cast<word_t>(data) | MemGate::R, env()->coreid,
            rcvepid, datasize);

        word_t buf[datasize / sizeof(word_t)];

        dmacmd(buf, datasize, sndepid, 0, datasize, DTU::READ);
        assert_false(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);
        dtu.wait_for_mem_cmd();
        for(size_t i = 0; i < 4; ++i)
            assert_word(buf[i], data[i]);
    }

    unmap_page(addr);
    dtu.configure(sndepid, 0, 0, 0, 0);
}

void CommandsTestSuite::WriteCmdTestCase::run() {
    const size_t rcvepid = 3;
    const size_t sndepid = 4;
    DTU &dtu = DTU::get();
    RecvBuf buf = RecvBuf::create(sndepid, nextlog2<64>::val, nextlog2<32>::val, 0);
    // only necessary to set the msgqid
    RecvBuf rbuf = RecvBuf::create(rcvepid, nextlog2<64>::val, RecvBuf::NO_RINGBUF);

    void *addr = map_page();
    if(!addr)
        return;

    cout << "-- Test errors --\n";
    {
        word_t data[] = {1234, 5678, 1122, 3344};
        dtu.configure(sndepid, reinterpret_cast<word_t>(addr) | MemGate::W, env()->coreid,
            rcvepid, sizeof(data));

        dmacmd(nullptr, 0, sndepid, 0, sizeof(data), DTU::READ);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);
    }

    cout << "-- Test writing --\n";
    {
        word_t data[] = {1234, 5678, 1122, 3344};
        dtu.configure(sndepid, reinterpret_cast<word_t>(addr) | MemGate::W, env()->coreid,
            rcvepid, sizeof(data));

        dmacmd(data, sizeof(data), sndepid, 0, sizeof(data), DTU::WRITE);
        assert_false(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);
        getmsg(rcvepid, 1);
        for(size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i)
            assert_word(reinterpret_cast<word_t*>(addr)[i], data[i]);
        dtu.mark_read(rcvepid);
    }

    unmap_page(addr);
    dtu.configure(sndepid, 0, 0, 0, 0);
}

void CommandsTestSuite::CmpxchgCmdTestCase::run() {
    const size_t rcvepid = 3;
    const size_t sndepid = 4;
    DTU &dtu = DTU::get();
    RecvBuf buf = RecvBuf::create(sndepid, nextlog2<128>::val, nextlog2<64>::val, 0);
    // only necessary to set the msgqid
    RecvBuf rbuf = RecvBuf::create(rcvepid, nextlog2<1>::val, RecvBuf::NO_RINGBUF);

    void *addr = map_page();
    if(!addr)
        return;

    const size_t refdatasize = sizeof(word_t) * 2;
    word_t *refdata = reinterpret_cast<word_t*>(addr);
    refdata[0] = 1234;
    refdata[1] = 5678;

    cout << "-- Test errors --\n";
    {
        word_t data[] = {1234, 567, 1122, 3344};
        dtu.configure(sndepid, reinterpret_cast<word_t>(refdata) | MemGate::X, env()->coreid,
            rcvepid, refdatasize);

        dmacmd(data, sizeof(data), sndepid, 0, sizeof(refdata), DTU::READ);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);

        dmacmd(data, sizeof(data), sndepid, 0, sizeof(data), DTU::CMPXCHG);
        assert_true(dtu.get_cmd(DTU::CMD_CTRL) & DTU::CTRL_ERROR);
    }

    cout << "-- Test failure --\n";
    {
        word_t data[] = {1234, 567, 1122, 3344};
        dtu.configure(sndepid, reinterpret_cast<word_t>(refdata) | MemGate::X, env()->coreid,
            rcvepid, refdatasize);

        dmacmd(data, sizeof(data), sndepid, 0, refdatasize, DTU::CMPXCHG);
        assert_false(dtu.wait_for_mem_cmd());
        assert_word(refdata[0], 1234);
        assert_word(refdata[1], 5678);
    }

    cout << "-- Test success --\n";
    {
        word_t data[] = {1234, 5678, 1122, 3344};
        dtu.configure(sndepid, reinterpret_cast<word_t>(refdata) | MemGate::X, env()->coreid,
            rcvepid, refdatasize);

        dmacmd(data, sizeof(data), sndepid, 0, refdatasize, DTU::CMPXCHG);
        assert_true(dtu.wait_for_mem_cmd());
        assert_word(refdata[0], 1122);
        assert_word(refdata[1], 3344);
    }

    cout << "-- Test offset --\n";
    {
        word_t data[] = {3344, 4455};
        dtu.configure(sndepid, reinterpret_cast<word_t>(refdata) | MemGate::X, env()->coreid,
            rcvepid, refdatasize);

        dmacmd(data, sizeof(data), sndepid, sizeof(word_t), sizeof(word_t), DTU::CMPXCHG);
        assert_true(dtu.wait_for_mem_cmd());
        assert_word(refdata[0], 1122);
        assert_word(refdata[1], 4455);
    }

    unmap_page(addr);
    dtu.configure(sndepid, 0, 0, 0, 0);
}
