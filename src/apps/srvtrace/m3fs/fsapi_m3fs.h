/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>,
 * Matthias Hille <matthias.hille@tu-dresden.de>
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

#include <base/util/Profile.h>

#include <m3/stream/Standard.h>
#include <m3/vfs/File.h>
#include <m3/vfs/Dir.h>
#include <m3/vfs/VFS.h>
#include <m3/VPE.h>
#include <m3/pipe/LocalDirectPipe.h>
#include <m3/pipe/AggregateDirectPipe.h>

#include "common/exceptions.h"
#include "common/fsapi.h"
#include "common/buffer.h"

extern m3::LocalDirectPipe *globpipe;

class FSAPI_M3FS : public FSAPI {
    enum { MaxOpenFds = 16 };

    void checkFd(int fd) {
        if(fdMap[fd] == 0)
            exitmsg("Using uninitialized file @ " << fd);
    }

#if defined(__gem5__)
    static cycles_t rdtsc() {
        uint32_t u, l;
        asm volatile ("rdtsc" : "=a" (l), "=d" (u) : : "memory");
        return (cycles_t)u << 32 | l;
    }
#endif

public:
    explicit FSAPI_M3FS(m3::String const &prefix);

    virtual void start() override {
        _start = m3::Profile::start(0);
    }
    virtual void stop() override {
        cycles_t end = m3::Profile::stop(0);
        m3::cout << "Total time: " << (end - _start) << " cycles\n";
    }

    virtual int error() override {
        return m3::Errors::last;
    }

    virtual void checkpoint(int, int, bool) override {
        // TODO not implemented
    }

    virtual void waituntil(UNUSED const waituntil_args_t *args, int) override {
#if defined(__t2__) || defined(__t3__)
        int rem = args->timestamp / 4;
        while(rem > 0)
            asm volatile ("addi.n %0, %0, -1" : "+r"(rem));
#elif defined(__gem5__)
        cycles_t finish = rdtsc() + args->timestamp;
        while(rdtsc() < finish)
            ;
#endif
    }

    virtual void open(const open_args_t *args, UNUSED int lineNo) override {
        if(args->fd == -1) {
            int err = m3::VFS::open(add_prefix(args->name), args->flags);
            if(err != m3::FileTable::INVALID)
                THROW1(ReturnValueException, err, args->fd, lineNo);
            return;
        }
        if(fdMap[args->fd] != 0 || dirMap[args->fd] != nullptr){
            // debugging
            if(fdMap[args->fd] != 0)
                m3::cout << "fdMap filled\n";
            else
                m3::cout << "dirmap filled\n";
            exitmsg("Overwriting already used file/dir @ " << args->fd);
        }

        if(args->flags & O_DIRECTORY) {
            // debugging
            m3::cout << "putting fd " << args->fd << " into dirmap\n";
            dirMap[args->fd] = new m3::Dir(add_prefix(args->name));
            if (dirMap[args->fd] == nullptr && args->fd >= 0)
                THROW1(ReturnValueException, m3::Errors::last, args->fd, lineNo);
        }
        else {
            fdMap[args->fd] = m3::VFS::open(add_prefix(args->name), args->flags);
            if(fdMap[args->fd] == m3::FileTable::INVALID)
                THROW1(ReturnValueException, m3::Errors::last, args->fd, lineNo);
        }
    }

    virtual void openat(const openat_args_t *args, UNUSED int lineNo) override {
        // only supporting absolute paths
        if(strncmp(args->name, "/", 1) != 0)
            exitmsg("Unsupported Operation: OPENAT with relative paths");

        open_args_t forwardArgs;
        forwardArgs.fd = args->fd;
        forwardArgs.name = args->name;
        forwardArgs.flags = args->flags;
        forwardArgs.mode = args->mode;
        open(&forwardArgs, lineNo);
    }

    virtual void close(const close_args_t *args, int ) override {
        if(fdMap[args->fd]) {
            m3::VFS::close(fdMap[args->fd]);
            fdMap[args->fd] = 0;
        }
        else if(dirMap[args->fd]) {
            delete dirMap[args->fd];
            dirMap[args->fd] = nullptr;
        }
        else
            exitmsg("Using uninitialized file @ " << args->fd);
    }

    virtual void fsync(const fsync_args_t *, int ) override {
        // TODO not implemented
    }

    virtual ssize_t read(int fd, void *buffer, size_t size) override {
        checkFd(fd);
        return static_cast<ssize_t>(m3::VPE::self().fds()->get(fdMap[fd])->read(buffer, size));
    }

    virtual ssize_t write(int fd, const void *buffer, size_t size) override {
        checkFd(fd);
        return static_cast<ssize_t>(m3::VPE::self().fds()->get(fdMap[fd])->write(buffer, size));
    }

    virtual ssize_t pread(int fd, void *buffer, size_t size, off_t offset) override {
        checkFd(fd);
        m3::VPE::self().fds()->get(fdMap[fd])->seek(static_cast<size_t>(offset), SEEK_SET);
        return static_cast<ssize_t>(m3::VPE::self().fds()->get(fdMap[fd])->read(buffer, size));
    }

    virtual ssize_t pwrite(int fd, const void *buffer, size_t size, off_t offset) override {
        checkFd(fd);
        m3::VPE::self().fds()->get(fdMap[fd])->seek(static_cast<size_t>(offset), SEEK_SET);
        return static_cast<ssize_t>(m3::VPE::self().fds()->get(fdMap[fd])->write(buffer, size));
    }

    virtual void lseek(const lseek_args_t *args, UNUSED int lineNo) override {
        checkFd(args->fd);
        m3::VPE::self().fds()->get(fdMap[args->fd])->seek(static_cast<size_t>(args->offset), args->whence);
        // if (res != args->err)
        //     THROW1(ReturnValueException, res, args->offset, lineNo);
    }

    virtual void ftruncate(const ftruncate_args_t *, int ) override {
        // TODO not implemented
    }

    virtual void fstat(const fstat_args_t *args, UNUSED int lineNo) override {
        int res;
        m3::FileInfo info;
        if(fdMap[args->fd])
            res = m3::VPE::self().fds()->get(fdMap[args->fd])->stat(info);
        else if(dirMap[args->fd])
            res = dirMap[args->fd]->stat(info);
        else
            exitmsg("Using uninitialized file/dir @ " << args->fd);

        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void newfstatat(const newfstatat_args_t *args, int lineNo) {
        m3::FileInfo info;
        // only supporting absolute paths
        if(strncmp(args->name, "/", 1) != 0)
            exitmsg("Unsupported Operation: NEWFSTATAT with relative paths");

        int res = m3::VFS::stat(add_prefix(args->name), info);
        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void stat(const stat_args_t *args, UNUSED int lineNo) override {
        m3::FileInfo info;
        int res = m3::VFS::stat(add_prefix(args->name), info);
        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void rename(const rename_args_t *args, UNUSED int lineNo) override {
        char fromprefixed[255];
        strcpy(fromprefixed, add_prefix(args->from));
        char toprefixed[255];
        strcpy(toprefixed, add_prefix(args->to));
        int res = m3::VFS::rename(fromprefixed, toprefixed);
        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void unlink(const unlink_args_t *args, UNUSED int lineNo) override {
        int res = m3::VFS::unlink(add_prefix(args->name));
        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void rmdir(const rmdir_args_t *args, UNUSED int lineNo) override {
        int res = m3::VFS::rmdir(add_prefix(args->name));
        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void mkdir(const mkdir_args_t *args, UNUSED int lineNo) override {
        int res = m3::VFS::mkdir(add_prefix(args->name), 0777 /*args->mode*/);
        if ((res == m3::Errors::NO_ERROR) != (args->err == 0))
            THROW1(ReturnValueException, res, args->err, lineNo);
    }

    virtual void sendfile(Buffer &buf, const sendfile_args_t *args, int) override {
        assert(args->offset == nullptr);
        checkFd(args->in_fd);
        checkFd(args->out_fd);
        m3::File *in = m3::VPE::self().fds()->get(fdMap[args->in_fd]);
        m3::File *out = m3::VPE::self().fds()->get(fdMap[args->out_fd]);
        char *rbuf = buf.readBuffer(Buffer::MaxBufferSize);
        size_t rem = args->count;
        while(rem > 0) {
            size_t amount = m3::Math::min(static_cast<size_t>(Buffer::MaxBufferSize), rem);
            size_t res = static_cast<size_t>(in->read(rbuf, amount));
            out->write(rbuf, res);
            rem -= amount;
        }
    }

    virtual void getdents(const getdents_args_t *args, UNUSED int lineNo) override {
        if(dirMap[args->fd] == nullptr)
            exitmsg("Using uninitialized dir @ " << args->fd);
        m3::Dir::Entry e;
        int i;
        // we don't check the result here because strace is often unable to determine the number of
        // fetched entries.
        if(args->count == 0 && dirMap[args->fd]->readdir(e))
            ; //THROW1(ReturnValueException, 1, args->count, lineNo);
        else {
            for(i = 0; i < args->count && dirMap[args->fd]->readdir(e); ++i)
                ;
            //if(i != args->count)
            //    THROW1(ReturnValueException, i, args->count, lineNo);
        }
    }

    virtual void createfile(const createfile_args_t *, int ) override {
        // TODO not implemented
    }

    virtual bool select(const select_args_t *, UNUSED int lineNo) override {
        // wait on pipe
        m3::GateIStream is = pipe->wait();
        // should be one int in message
        if(is.length() != sizeof(int))
            THROW1(ReturnValueException, is.length(), sizeof(int), lineNo);
        int res;
        is >> res;
        // if we receive a 0 we should shutdown
        if(res)
            return true;
        return false;
    }

    virtual void recvfrom(const recvfrom_args_t *args, UNUSED int lineNo) {
        // wait on pipe
        m3::GateIStream is = pipe->wait();
        // read request
        if(static_cast<int>(is.length()) != args->err)
            THROW1(ReturnValueException, is.length(), args->err, lineNo);
    }

    virtual void modsendfile(Buffer &buf, const sendfile_args_t *args, UNUSED int lineNo) {
        // write into pipe
        assert(args->offset == nullptr);
        checkFd(args->in_fd);
        m3::File *in = m3::VPE::self().fds()->get(fdMap[args->in_fd]);
        char *rbuf = buf.readBuffer(m3::Math::round_dn(m3::AggregateDirectPipe::MSG_SIZE - m3::DTU::HEADER_SIZE, DTU_PKG_SIZE));
        size_t rem = args->count;
        while(rem > 0) {
            size_t amount = m3::Math::min(static_cast<size_t>(m3::Math::round_dn(
                    m3::AggregateDirectPipe::MSG_SIZE - m3::DTU::HEADER_SIZE, DTU_PKG_SIZE)), rem);
            size_t res = static_cast<size_t>(in->read(rbuf, amount));
            assert(res);
            pipe->writer()->send(rbuf, res);
            rem -= amount;
        }
    }

    virtual void writev(const writev_args_t *args, UNUSED int lineNo) {
        // write response header into pipe
        if(pipe->writer()->send(args->iovec.iov_base, args->iovec.iov_len) != m3::Errors::NO_ERROR)
            THROW1(ReturnValueException, m3::Errors::NO_ERROR, m3::Errors::last, lineNo);
    }

    m3::LocalDirectPipe *pipe;

private:
    const char *add_prefix(const char *path) {
        static char tmp[255];
	// don't add prefix to absolute paths, except for the /tmp/ directory
        if(_prefix.length() == 0 || (path[0] == '/' && strstr(path, "/tmp/") == NULL))
            return path;

        m3::OStringStream os(tmp, sizeof(tmp));
        os << _prefix << "/" << path;
        return tmp;
    }

    cycles_t _start;
    const m3::String _prefix;
    fd_t fdMap[MaxOpenFds];
    m3::Dir *dirMap[MaxOpenFds];
};
