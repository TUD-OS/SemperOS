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

#include <base/Machine.h>

#include <m3/vfs/File.h>

namespace m3 {

/**
 * The base-class for a file that reads/writes from/to a pipe. Can't be instantiated.
 */
class SerialFile : public File {
public:
    explicit SerialFile() : File() {
    }

    virtual int stat(FileInfo &) const override {
        // not supported
        return -1;
    }
    virtual off_t seek(off_t, int) override {
        // not supported
        return 0;
    }

    virtual Buffer *create_buf(size_t size) override {
        return new File::Buffer(size);
    }

    virtual ssize_t read(void *buffer, size_t count) override {
        return Machine::read(reinterpret_cast<char*>(buffer), count);
    }
    virtual ssize_t write(const void *buffer, size_t count) override {
        int res = Machine::write(reinterpret_cast<const char*>(buffer), count);
        return res < 0 ? res : count;
    }

    virtual char type() const override {
        return 'S';
    }
    virtual size_t serialize_length() override {
        return 0;
    }
    virtual void delegate(VPE &) override {
        // nothing to do
    }
    virtual void serialize(Marshaller &) override {
        // nothing to do
    }
    static SerialFile *unserialize(Unmarshaller &) {
        return new SerialFile();
    }

private:
    virtual bool seek_to(off_t) override {
        return false;
    }
};

}
