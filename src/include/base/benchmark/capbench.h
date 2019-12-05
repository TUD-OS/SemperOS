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

#include <base/util/Profile.h>

#define CAP_BENCH_TRACE_S(marker)     m3::Profile::start(marker)
#define CAP_BENCH_TRACE_F(marker)     m3::Profile::stop(marker)
#define CAP_BENCH_TRACE_X_S(marker)
#define CAP_BENCH_TRACE_X_F(marker)
//#define CAP_BENCH_TRACE_X_S(marker)   m3::Profile::start(marker)
//#define CAP_BENCH_TRACE_X_F(marker)   m3::Profile::stop(marker)

// trace markers for capbench
#define TRACE_START_ANALYSIS        10
#define TRACE_FINISH_ANALYSIS       11
// obtain
#define APP_OBT_START               12
#define KERNEL_OBT_SYSC_RCV         13
#define KERNEL_OBT_TO_SRV           14
#define KERNEL_OBT_TO_RKERNEL       15
#define RKERNEL_OBT_TO_SRV          16
#define RKERNEL_OBT_FROM_SRV        17
#define KERNEL_OBT_FROM_RKERNEL     18
#define KERNEL_OBT_THRD_WAKEUP      19
#define KERNEL_OBT_FROM_SRV         20
#define KERNEL_OBT_SYSC_RESP        21
#define APP_OBT_FINISH              22

// revoke
#define APP_REV_START               23
#define KERNEL_REV_SYSC_RCV         24
#define KERNEL_REV_TO_RKERNEL       25
#define KERNEL_REV_FROM_RKERNEL     26
#define KERNEL_REV_THRD_WAKEUP      27
#define KERNEL_REV_SYSC_RESP        28
#define APP_REV_FINISH              29