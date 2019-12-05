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

#include <base/log/Services.h>
#include <base/Panic.h>

#include "FSHandle.h"
#include "INodes.h"

using namespace m3;

bool FSHandle::load_superblock(MemGate &mem, SuperBlock *sb) {
    mem.read_sync(sb, sizeof(*sb), 0);
    SLOG(FS, "Superblock:");
    SLOG(FS, "  blocksize=" << sb->blocksize);
    SLOG(FS, "  total_inodes=" << sb->total_inodes);
    SLOG(FS, "  total_blocks=" << sb->total_blocks);
    SLOG(FS, "  free_inodes=" << sb->free_inodes);
    SLOG(FS, "  free_blocks=" << sb->free_blocks);
    SLOG(FS, "  first_free_inode=" << sb->first_free_inode);
    SLOG(FS, "  first_free_block=" << sb->first_free_block);
    if(sb->checksum != sb->get_checksum())
        PANIC("Superblock checksum is invalid. Terminating.");
    return true;
}

FSHandle::FSHandle(capsel_t mem)
        : _mem(MemGate::bind(mem)), _dummy(load_superblock(_mem, &_sb)), _cache(_mem, _sb.blocksize),
          _blocks(_sb.first_blockbm_block(), &_sb.first_free_block, &_sb.free_blocks,
                _sb.total_blocks, _sb.blockbm_blocks()),
          _inodes(_sb.first_inodebm_block(), &_sb.first_free_inode, &_sb.free_inodes,
                _sb.total_inodes, _sb.inodebm_blocks()) {
}
