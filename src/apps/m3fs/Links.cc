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

#include "INodes.h"
#include "Links.h"
#include "Dirs.h"

using namespace m3;

Errors::Code Links::create(FSHandle &h, INode *dir, const char *name, size_t namelen, INode *inode) {
    size_t rem;
    DirEntry *e;

    foreach_block(h, dir, bno) {
        foreach_direntry(h, bno, de) {
            rem = de->next - (sizeof(DirEntry) + de->namelen);
            if(rem >= sizeof(DirEntry) + namelen) {
                // change previous entry
                de->next = de->namelen + sizeof(DirEntry);
                // get pointer to new one
                e = reinterpret_cast<DirEntry*>(reinterpret_cast<uintptr_t>(de) + de->next);
                h.cache().mark_dirty(bno);
                goto found;
            }
        }
    }

    // no suitable space found; extend directory
    {
        Extent *ext = INodes::get_extent(h, dir, dir->extents, nullptr, true);
        if(!ext)
            return Errors::NO_SPACE;

        // insert one block in extent
        INodes::fill_extent(h, dir, ext, 1);
        if(ext->length == 0)
            return Errors::NO_SPACE;

        // put entry at the beginning of the block
        e = reinterpret_cast<DirEntry*>(h.cache().get_block(ext->start, true));
        rem = h.sb().blocksize;
    }

found:
    // write entry
    e->namelen = namelen;
    e->nodeno = inode->inode;
    e->next = rem;
    strncpy(e->name, name, namelen);

    inode->links++;
    INodes::mark_dirty(h, inode->inode);
    return Errors::NO_ERROR;
}

Errors::Code Links::remove(FSHandle &h, INode *dir, const char *name, size_t namelen, bool isdir) {
    foreach_block(h, dir, bno) {
        DirEntry *prev = nullptr;
        foreach_direntry(h, bno, e) {
            if(e->namelen == namelen && strncmp(e->name, name, namelen) == 0) {
                // if we're not removing a dir, we're coming from unlink(). in this case, directories
                // are not allowed
                INode *inode = INodes::get(h, e->nodeno);
                if(!isdir && M3FS_ISDIR(inode->mode))
                    return Errors::IS_DIR;

                // remove entry by skipping over it or making it invalid
                if(prev)
                    prev->next += e->next;
                else
                    e->namelen = 0;
                h.cache().mark_dirty(bno);

                // reduce links and free, if necessary
                if(--inode->links == 0)
                    INodes::free(h, inode);
                return Errors::NO_ERROR;
            }

            prev = e;
        }
    }
    return Errors::NO_SUCH_FILE;
}

m3::Errors::Code Links::rename(FSHandle& h, m3::INode *olddir, const char* oldname, size_t oldnamelen,
        m3::INode *newdir, const char* newname, size_t newnamelen, bool isdir) {
    // TODO
    // implement directory renaming
    if(isdir)
        return Errors::IS_DIR;

    foreach_block(h, olddir, bno) {
        DirEntry *prev = nullptr;
        foreach_direntry(h, bno, e) {
            if(e->namelen == oldnamelen && strncmp(e->name, oldname, oldnamelen) == 0) {

                INode *inode = INodes::get(h, e->nodeno);
                // TODO
                // implement directory renaming
                if(M3FS_ISDIR(inode->mode))
                    return Errors::IS_DIR;

                // remove entry by skipping over it or making it invalid
                if(prev)
                    prev->next += e->next;
                else
                    e->namelen = 0;
                h.cache().mark_dirty(bno);

                // decrease link count because create will increase it again
                --inode->links;
                return create(h, newdir, newname, newnamelen, inode);
            }

            prev = e;
        }
    }
    return Errors::NO_SUCH_FILE;
}
