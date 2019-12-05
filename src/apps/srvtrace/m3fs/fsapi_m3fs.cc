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

#include "fsapi_m3fs.h"

FSAPI_M3FS::FSAPI_M3FS(const m3::String& prefix)
: _start(), _prefix(prefix), fdMap(), dirMap() {
    // open logfile
    trace_op_t *op = trace_ops;

    // skip to the end where the logfile is opened
    while (op && op->opcode != INVALID_OP)
        op++;
    op++;
    if(!op || op->opcode != OPEN_OP)
        exitmsg("Invalid server trace. Did not find log opening statement");

    pipe = globpipe;
    open(&op->args.open, 1000);
}
