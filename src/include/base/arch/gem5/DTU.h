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

#pragma once

#include <base/Common.h>
#include <base/Env.h>
#include <base/util/Util.h>
#include <base/util/Sync.h>
#include <base/Errors.h>
#include <assert.h>

#define DTU_PKG_SIZE        (static_cast<size_t>(8))

#define ID_BITS         64
#define RESERVED_BITS   5
#define VALID_BITS      1
#define CORE_BITS       10
#define VPE_BITS        11
#define OFFSET_BITS     (ID_BITS - RESERVED_BITS - VALID_BITS - CORE_BITS - VPE_BITS)

#define VALID_SHIFT     (ID_BITS - RESERVED_BITS)
#define CORE_SHIFT      (VALID_SHIFT - CORE_BITS)
#define VPE_SHIFT       (CORE_SHIFT - VPE_BITS)

#define CORE_VALID_OFFSET     (static_cast<unsigned long>(1) << (CORE_BITS + 1))

#define REG_SIZE        64
#define EP_BITS         8
#define MAX_MSG_SZ_BITS 16
#define CREDITS_BITS    16
#define FLAGS_BITS      4

namespace kernel {
class DTU;
}

namespace m3 {

class DTU {
    friend class kernel::DTU;

    explicit DTU() {
    }

public:
    typedef uint64_t reg_t;

private:
    static const uintptr_t BASE_ADDR        = 0x5C0000000;
    static const size_t DTU_REGS            = 8;
    static const size_t CMD_REGS            = 6;
    static const size_t EP_REGS             = 3;

    static const size_t CREDITS_UNLIM       = ((static_cast<size_t>(1) << CREDITS_BITS) - 1);

    enum class DtuRegs {
        STATUS              = 0,
        ROOT_PT             = 1,
        PF_EP               = 2,
        LAST_PF             = 3,
        RW_BARRIER          = 4,
        VPE_ID              = 5,
        MSGCNT              = 6,
        EXT_CMD             = 7,
    };

    enum class CmdRegs {
        COMMAND             = 8,
        DATA_ADDR           = 9,
        DATA_SIZE           = 10,
        OFFSET              = 11,
        REPLY_EP            = 12,
        REPLY_LABEL         = 13,
    };

    enum MemFlags : reg_t {
        R                   = 1 << 0,
        W                   = 1 << 1,
    };

    enum StatusFlags : reg_t {
        PRIV                = 1 << 0,
        PAGEFAULTS          = 1 << 1,
    };

    enum class EpType {
        INVALID,
        SEND,
        RECEIVE,
        MEMORY
    };

    enum class CmdOpCode {
        IDLE                = 0,
        SEND                = 1,
        REPLY               = 2,
        READ                = 3,
        WRITE               = 4,
        FETCH_MSG           = 5,
        ACK_MSG             = 6,
        DEBUG_MSG           = 7,
    };

    enum class ExtCmdOpCode {
        WAKEUP_CORE         = 0,
        INV_PAGE            = 1,
        INV_TLB             = 2,
        INV_CACHE           = 3,
        INJECT_IRQ          = 4,
    };

public:
    typedef uint64_t pte_t;

    enum {
        PTE_BITS            = 3,
        PTE_SIZE            = 1 << PTE_BITS,
        LEVEL_CNT           = 2,
        LEVEL_BITS          = PAGE_BITS - PTE_BITS,
        LEVEL_MASK          = (1 << LEVEL_BITS) - 1,
        PTE_REC_IDX         = LEVEL_MASK,
    };

    enum {
        PTE_R               = 1,
        PTE_W               = 2,
        PTE_X               = 4,
        PTE_I               = 8,
        PTE_GONE            = 16,
        PTE_RW              = PTE_R | PTE_W,
        PTE_RWX             = PTE_RW | PTE_X,
        PTE_IRWX            = PTE_RWX | PTE_I,
    };

    struct Header {
        uint8_t flags; // if bit 0 is set its a reply, if bit 1 is set we grant credits
        uint16_t senderCoreId;
        uint8_t senderEpId;
        uint8_t replyEpId; // for a normal message this is the reply epId
                           // for a reply this is the enpoint that receives credits
        uint16_t length;
        uint16_t senderVpeId;

        uint64_t label;
        uint64_t replylabel;
    } PACKED;

    struct Message : Header {
        int send_epid() const {
            return senderEpId;
        }
        int reply_epid() const {
            return replyEpId;
        }

        unsigned char data[];
    } PACKED;

    static const size_t HEADER_SIZE         = sizeof(Header);

    // TODO not yet supported
    static const int FLAG_NO_RINGBUF        = 0;
    static const int FLAG_NO_HEADER         = 1;

    static const int MEM_EP                 = 0;    // unused
    static const int SYSC_EP                = 0;
    static const int DEF_RECVEP             = 1;
    static const int FIRST_FREE_EP          = 2;
    static const int MAX_MSG_SLOTS          = 32;

    static DTU &get() {
        return inst;
    }

    static size_t noc_to_pe(uint64_t noc) {
        return (noc >> CORE_SHIFT) - CORE_VALID_OFFSET;
    }
    static uintptr_t noc_to_virt(uint64_t noc) {
        return noc & ((static_cast<uint64_t>(1) << CORE_SHIFT) - 1);
    }
    static uint64_t build_noc_addr(int pe, uintptr_t virt) {
        return (static_cast<uintptr_t>(CORE_VALID_OFFSET + pe) << CORE_SHIFT) | virt;
    }

    uintptr_t get_last_pf() const {
        return read_reg(DtuRegs::LAST_PF);
    }

    Errors::Code send(int ep, const void *msg, size_t size, label_t replylbl, int reply_ep);
    Errors::Code reply(int ep, const void *msg, size_t size, size_t off);
    Errors::Code read(int ep, void *msg, size_t size, size_t off);
    Errors::Code write(int ep, const void *msg, size_t size, size_t off);
    Errors::Code cmpxchg(int, const void *, size_t, size_t, size_t) {
        // TODO unsupported
        return Errors::NO_ERROR;
    }

    bool is_valid(int epid) const {
        reg_t r0 = read_reg(epid, 0);
        return static_cast<EpType>(r0 >> 61) != EpType::INVALID;
    }

    Message *fetch_msg(int epid) const {
        write_reg(CmdRegs::COMMAND, buildCommand(epid, CmdOpCode::FETCH_MSG));
        Sync::memory_barrier();
        return reinterpret_cast<Message*>(read_reg(CmdRegs::OFFSET));
    }

    size_t get_msgoff(int, const Message *msg) const {
        return reinterpret_cast<uintptr_t>(msg);
    }

    void mark_read(int ep, size_t off) {
        write_reg(CmdRegs::OFFSET, off);
        // ensure that we are really done with the message before acking it
        Sync::memory_barrier();
        write_reg(CmdRegs::COMMAND, buildCommand(ep, CmdOpCode::ACK_MSG));
        // ensure that we don't do something else before the ack
        Sync::memory_barrier();
    }

    bool wait() const {
        // wait until the DTU wakes us up
        // note that we have a race-condition here. if a message arrives between the check and the
        // hlt, we miss it. this case is handled by a pin at the CPU, which indicates whether
        // unprocessed messages are available. if so, hlt does nothing. in this way, the ISA does
        // not have to be changed.
        if(read_reg(DtuRegs::MSGCNT) == 0)
            asm volatile ("hlt");
        return true;
    }
    void wait_until_ready(int) const {
        // this is superfluous now, but leaving it here improves the syscall time by 40 cycles (!!!)
        // compilers are the worst. let's get rid of them and just write assembly code again ;)
        while((read_reg(CmdRegs::COMMAND) & 0x7) != 0)
            ;
    }
    bool wait_for_mem_cmd() const {
        // we've already waited
        return true;
    }

    void debug_msg(uint msg) {
        write_reg(CmdRegs::COMMAND, buildCommand(msg, CmdOpCode::DEBUG_MSG));
    }

private:

    static Errors::Code get_error() {
        while(true) {
            reg_t cmd = read_reg(CmdRegs::COMMAND);
            if(static_cast<CmdOpCode>(cmd & 0x7) == CmdOpCode::IDLE)
                return static_cast<Errors::Code>(cmd >> 11);
        }
        UNREACHED;
    }

    static reg_t read_reg(DtuRegs reg) {
        return read_reg(static_cast<size_t>(reg));
    }
    static reg_t read_reg(CmdRegs reg) {
        return read_reg(static_cast<size_t>(reg));
    }
    static reg_t read_reg(int ep, size_t idx) {
        return read_reg(DTU_REGS + CMD_REGS + EP_REGS * ep + idx);
    }
    static reg_t read_reg(size_t idx) {
        reg_t res;
        asm volatile (
            "mov (%1), %0"
            : "=r"(res)
            : "r"(BASE_ADDR + idx * sizeof(reg_t))
        );
        return res;
    }

    static void write_reg(DtuRegs reg, reg_t value) {
        write_reg(static_cast<size_t>(reg), value);
    }
    static void write_reg(CmdRegs reg, reg_t value) {
        write_reg(static_cast<size_t>(reg), value);
    }
    static void write_reg(size_t idx, reg_t value) {
        asm volatile (
            "mov %0, (%1)"
            : : "r"(value), "r"(BASE_ADDR + idx * sizeof(reg_t))
        );
    }

    static uintptr_t dtu_reg_addr(DtuRegs reg) {
        return BASE_ADDR + static_cast<size_t>(reg) * sizeof(reg_t);
    }
    static uintptr_t cmd_reg_addr(CmdRegs reg) {
        return BASE_ADDR + static_cast<size_t>(reg) * sizeof(reg_t);
    }
    static uintptr_t ep_regs_addr(int ep) {
        return BASE_ADDR + (DTU_REGS + CMD_REGS + ep * EP_REGS) * sizeof(reg_t);
    }

    static reg_t buildCommand(int ep, CmdOpCode c) {
        return static_cast<uint>(c) | (ep << 3);
    }

    static DTU inst;
};

}
