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

#include <m3/stream/Standard.h>
#include <m3/vfs/VFS.h>

using namespace m3;

int main(int argc, char **argv) {
    if(argc != 3)
        exitmsg("Usage: " << argv[0] << " <target> <linkname>");

    if(VFS::link(argv[1], argv[2]) != Errors::NO_ERROR)
        errmsg("Link of " << argv[1] << " to " << argv[2] << " failed");
    return 0;
}
