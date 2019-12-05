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

#define PACKED                  __attribute__ ((packed))
#define NORETURN                __attribute__ ((__noreturn__))
#define NOINLINE                __attribute__ ((noinline))
#define ALWAYS_INLINE           __attribute__ ((always_inline))
#define EXPECT_FALSE(X)         __builtin_expect(!!(X), 0)
#define EXPECT_TRUE(X)          __builtin_expect(!!(X), 1)
#define USED                    __attribute__ ((used))
#define UNUSED                  __attribute__ ((unused))
#define UNREACHED               __builtin_unreachable()
#define WEAK                    __attribute__ ((weak))
#define INIT_PRIO(X)            __attribute__ ((init_priority((X))))
#define ALIGNED(X)              __attribute__ ((aligned(X)))

#ifdef __clang__
#    define COMPILER_NAME       "clang " __VERSION__
#elif defined(__GNUC__)
#    define COMPILER_NAME       "gcc " __VERSION__
#else
#    error "Unknown compiler"
#endif

#ifdef __cplusplus
#    define EXTERN_C             extern "C"
#else
#    define EXTERN_C
#endif
