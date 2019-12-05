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

#define CAP_TOTAL           8192
#define FS_IMG_OFFSET       0x0

#define PAGE_BITS           12
#define PAGE_SIZE           (static_cast<size_t>(1) << PAGE_BITS)
#define PAGE_MASK           (PAGE_SIZE - 1)

#define MOD_HEAP_SIZE       (64 * 1024)
#define APP_HEAP_SIZE       (64 * 1024 * 1024)

#define HEAP_SIZE           0x10000
#define EP_COUNT            16

#define RT_START            0x3000
#define RT_SIZE             0x20000
#define RT_END              (RT_START + RT_SIZE)

#define STACK_SIZE          0x1000
#define STACK_TOP           (RT_END + STACK_SIZE)
#define STACK_BOTTOM        RT_END

#define MAX_RB_SIZE         32

#define RECVBUF_SPACE       0x5B0000000
#define RECVBUF_SIZE        (24 * PAGE_SIZE)
#define RECVBUF_SIZE_SPM    131072
#define SECONDARY_RECVBUF_SIZE_SPM  1048576

#define DEF_RCVBUF_ORDER    8
#define DEF_RCVBUF_SIZE     (1 << DEF_RCVBUF_ORDER)
#define DEF_RCVBUF          RECVBUF_SPACE

#define MEMCAP_END          RECVBUF_SPACE

#define KRNL_INIT_MEM_SIZE  (32 * 1024 * 1024)
#define KRNL_HEAP_SIZE      0x1000
#define KENV_START          0x7000
#define KENV_SIZE           0x5000

#define SYNC_APP_START      1
#define CASCADING_APP_START 0
#define KERNEL_STATISTICS   1
