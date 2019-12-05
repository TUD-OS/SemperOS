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

#include <base/Common.h>

#include <m3/vfs/File.h>
#include <m3/vfs/FileTable.h>
#include <m3/vfs/VFS.h>

namespace m3 {

/**
 * Convenience class that opens a file, stores it and deletes it on destruction.
 */
class FileRef {
public:
    /**
     * Opens the file given by <path>.
     *
     * @param path the path to open
     * @param perms the permissions (FILE_*)
     */
    explicit FileRef(const char *path, int perms) : _fd(VFS::open(path, perms)) {
    }
    FileRef(FileRef &&f) : _fd(f._fd) {
        f._fd = FileTable::INVALID;
    }
    FileRef(const FileRef&) = delete;
    FileRef &operator=(const FileRef&) = delete;
    ~FileRef() {
        if(_fd != FileTable::INVALID)
            VFS::close(_fd);
    }

    File *get() {
        return VPE::self().fds()->get(_fd);
    }

    File *operator->() {
        return get();
    }
    const File *operator->() const {
        return const_cast<FileRef*>(this)->get();
    }
    File &operator*() {
        return *get();
    }
    const File &operator*() const {
        return *const_cast<FileRef*>(this)->get();
    }

private:
    fd_t _fd;
};

}
