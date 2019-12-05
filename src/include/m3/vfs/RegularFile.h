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
#include <base/util/CapRngDesc.h>

#include <m3/session/M3FS.h>
#include <m3/vfs/LocList.h>
#include <m3/vfs/FileSystem.h>
#include <m3/vfs/File.h>

namespace m3 {

class M3FS;
class FStream;
class RegFileBuffer;

/**
 * The File implementation for regular files. Note that this is the low-level API for working with
 * files. It expects your buffer and read-/write-sizes to be aligned by DTU_PKG_SIZE and every
 * read/write operation will actually issue a read/write operation using the DTU. It thus allows you
 * to reach the best possible performance. On the other hand, it might be inconvenient to use. For
 * that reason, FStream is provided as a buffered version on top of this that does not require
 * proper alignment and will delay read/write operations due to buffering. On the other hand, it
 * has a bit of overhead, obviously.
 * You can't instantiate this class, because VFS::open should be used.
 */
class RegularFile : public File {
    friend class FStream;
    friend class M3FS;
    friend class RegFileBuffer;

    struct Position {
        explicit Position() : local(MAX_LOCS), global(), offset() {
        }

        bool valid() const {
            return local < MAX_LOCS;
        }

        void next_extent() {
            local++;
            global++;
            offset = 0;
        }

        uint16_t local;
        uint16_t global;
        size_t offset;
    } PACKED;

    enum {
        // the number of blocks by which we extend a file when appending
        WRITE_INC_BLOCKS    = 512
    };

    explicit RegularFile(int fd, Reference<M3FS> fs, int perms);
public:
    virtual ~RegularFile();

    /**
     * @return the file descriptor
     */
    int fd() const {
        return _fd;
    }
    /**
     * @return the M3FS instance
     */
    const Reference<M3FS> &fs() const {
        return _fs;
    }

    virtual Buffer *create_buf(size_t size) override;
    virtual int stat(FileInfo &info) const override;
    virtual off_t seek(off_t offset, int whence) override;
    virtual ssize_t read(void *buffer, size_t count) override {
        return do_read(buffer, count, _pos);
    }
    virtual ssize_t write(const void *buffer, size_t count) override {
        return do_write(buffer, count, _pos);
    }

    virtual char type() const override {
        return 'M';
    }
    static void setMemoryAccess(bool v) {
        RegularFile::doMemAccess = v;
    }
    virtual size_t serialize_length() override;
    virtual void delegate(VPE &vpe) override;
    virtual void serialize(Marshaller &m) override;
    static RegularFile *unserialize(Unmarshaller &um);

private:
    virtual bool seek_to(off_t offset) override;
    ssize_t fill(void *buffer, size_t size);
    ssize_t do_read(void *buffer, size_t count, Position &pos) const;
    ssize_t do_write(const void *buffer, size_t count, Position &pos) const;
    ssize_t get_location(Position &pos, bool writing) const;
    size_t get_amount(size_t length, size_t count, Position &pos) const;
    void adjust_written_part();

    int _fd;
    mutable bool _extended;
    mutable off_t _begin;
    mutable off_t _length;
    mutable Position _pos;
    mutable CapRngDesc _memcaps;
    mutable loclist_type _locs;
    mutable MemGate _lastmem;
    mutable uint16_t _last_extent;
    mutable size_t _last_off;
    Reference<M3FS> _fs;
    static bool doMemAccess;
};

}
