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

#include <m3/com/MemGate.h>
#include <m3/session/Pipe.h>
#include <m3/vfs/File.h>

namespace m3 {

class IndirectPipe {
public:
    explicit IndirectPipe(size_t memsize);
    ~IndirectPipe();

    /**
     * @return the file descriptor for the reader
     */
    fd_t reader_fd() const {
        return _rdfd;
    }
    /**
     * Closes the read-end
     */
    void close_reader();

    /**
     * @return the file descriptor for the writer
     */
    fd_t writer_fd() const {
        return _wrfd;
    }
    /**
     * Closes the write-end
     */
    void close_writer();

private:
    MemGate _mem;
    Pipe _pipe;
    fd_t _rdfd;
    fd_t _wrfd;
};

class IndirectPipeFile : public File {
    friend class IndirectPipe;

public:
    explicit IndirectPipeFile(capsel_t mem, capsel_t sess,
        capsel_t metagate, capsel_t rdgate, capsel_t wrgate)
        : _lastid(), _mem(MemGate::bind(mem)), _pipe(sess, metagate, rdgate, wrgate) {
    }

    virtual Buffer *create_buf(size_t size) override {
        return new File::Buffer(size);
    }

    virtual int stat(FileInfo &) const override {
        // not supported
        return -1;
    }
    virtual off_t seek(off_t, int) override {
        // not supported
        return 0;
    }

    virtual size_t serialize_length() override;
    virtual void delegate(VPE &vpe) override;
    virtual void serialize(Marshaller &m) override;
    static File *unserialize(Unmarshaller &um);

private:
    virtual bool seek_to(off_t) override {
        return false;
    }

protected:
    int _lastid;
    MemGate _mem;
    Pipe _pipe;
};

}
