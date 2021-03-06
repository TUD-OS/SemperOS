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

.global _start

.text

_start:
    # use a small stack
    mov     $stack, %rsp
    mov     %rsp, %rbp

    # check if there is something to run
    call    try_run

    # _if we get here, there was none, so idle
    hlt
    jmp     _start

.data

    .align 16
    .space 64, 0
stack:
