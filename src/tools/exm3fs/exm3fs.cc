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

#include <base/Common.h>

#include <fs/internal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

FILE *file;
m3::SuperBlock sb;

static void export_rec(const char *dest, m3::inodeno_t dirno, const char *src) {
    m3::INode inode = read_inode(dirno);
    char path[255];
    snprintf(path, sizeof(path), "%s/%s", dest, src);

    char *buffer = new char[sb.blocksize];
    if(S_ISDIR(inode.mode)) {
        if(mkdir(path, 0755) == -1)
            err(1, "Unable to create directory '%s'", path);

        size_t blockcount = (inode.size + sb.blocksize - 1) / sb.blocksize;
        for(uint32_t i = 0; i < blockcount; ++i) {
            read_from_block(buffer, sb.blocksize, get_block_no(inode, i));

            m3::DirEntry *e = reinterpret_cast<m3::DirEntry*>(buffer);
            m3::DirEntry *end = reinterpret_cast<m3::DirEntry*>(buffer + sb.blocksize);
            while(e->next > 0 && e < end) {
                if((e->namelen != 1 || strncmp(e->name, ".", 1) != 0) &&
                    (e->namelen != 2 || strncmp(e->name, "..", 2) != 0)) {
                    char epath[128];
                    snprintf(epath, sizeof(epath), "%s/%.*s", src, e->namelen, e->name);
                    export_rec(dest, e->nodeno, epath);
                }

                e = reinterpret_cast<m3::DirEntry*>(reinterpret_cast<char*>(e) + e->next);
            }
        }
    }
    else {
        FILE *f = fopen(path, "w");
        if(f == nullptr)
            err(1, "Unable to open '%s' for writing", path);

        size_t blockcount = (inode.size + sb.blocksize - 1) / sb.blocksize;
        size_t count = 0;
        for(uint32_t i = 0; i < blockcount; ++i) {
            read_from_block(buffer, sb.blocksize, get_block_no(inode, i));

            size_t amount = i < blockcount - 1 ? sb.blocksize : inode.size - count;
            if(fwrite(buffer, 1, amount, f) != amount)
                err(1, "fwrite to '%s' failed", path);

            count += sb.blocksize;
        }
        fclose(f);
    }

    delete[] buffer;
}

static void usage(const char *name) {
    fprintf(stderr, "Usage: %s <image> <dest>\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if(argc != 3)
        usage(argv[0]);

    file = fopen(argv[1], "r");
    if(!file)
        err(1, "Unable to open %s for reading", argv[1]);

    fread(&sb, sizeof(sb), 1, file);

    export_rec(argv[2], 0, "/");

    fclose(file);
    return 0;
}
