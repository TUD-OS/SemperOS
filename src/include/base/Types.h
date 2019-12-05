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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

using uchar     = unsigned char;
using ushort    = unsigned short;
using uint      = unsigned int;
using ulong     = unsigned long;
using llong     = long long;
using ullong    = unsigned long long;

using word_t    = unsigned long;
using label_t   = word_t;
using capsel_t  = unsigned;
using off_t     = long;
using fd_t      = int;
using cycles_t  = uint64_t;
