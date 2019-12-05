#
# Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
# Nils Asmussen <nils@os.inf.tu-dresden.de>
# Economic rights: Technische Universitaet Dresden (Germany)
#
# This file is part of SemperOS.
#
# SemperOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# SemperOS is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License version 2 for more details.
#

#include <base/Config.h>

.global _start
.extern env_run

_start:
    # stack pointer already set?
    mov     $0xDEADBEEF, %rcx
    cmp     %rcx, %rax
    je      1f

    # set default stack pointer
    mov     $(STACK_TOP - 8), %rsp
    mov     %rsp, %rbp

1:
    call    env_run

    # just to be sure
    hlt
