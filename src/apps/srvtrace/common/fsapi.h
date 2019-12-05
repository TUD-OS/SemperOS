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

#include "op_types.h"

class Buffer;

class FSAPI {
public:
    virtual ~FSAPI() {
    }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual int error() = 0;
    virtual void checkpoint(int numReplayed, int numTraceOps, bool make_chkpt) = 0;
    virtual void waituntil(const waituntil_args_t *args, int lineNo) = 0;
    virtual void open(const open_args_t *args, int lineNo) = 0;
    virtual void openat(const openat_args_t *args, int lineNo) = 0;
    virtual void close(const close_args_t *args, int lineNo) = 0;
    virtual void fsync(const fsync_args_t *args, int lineNo) = 0;
    virtual ssize_t read(int fd, void *buffer, size_t size) = 0;
    virtual ssize_t write(int fd, const void *buffer, size_t size) = 0;
    virtual ssize_t pread(int fd, void *buffer, size_t size, off_t offset) = 0;
    virtual ssize_t pwrite(int fd, const void *buffer, size_t size, off_t offset) = 0;
    virtual void lseek(const lseek_args_t *args, int lineNo) = 0;
    virtual void ftruncate(const ftruncate_args_t *args, int lineNo) = 0;
    virtual void fstat(const fstat_args_t *args, int lineNo) = 0;
    virtual void newfstatat(const newfstatat_args_t *args, int lineNo) = 0;
    virtual void stat(const stat_args_t *args, int lineNo) = 0;
    virtual void rename(const rename_args_t *args, int lineNo) = 0;
    virtual void unlink(const unlink_args_t *args, int lineNo) = 0;
    virtual void rmdir(const rmdir_args_t *args, int lineNo) = 0;
    virtual void mkdir(const mkdir_args_t *args, int lineNo) = 0;
    virtual void sendfile(Buffer &buf, const sendfile_args_t *args, int lineNo) = 0;
    virtual void getdents(const getdents_args_t *args, int lineNo) = 0;
    virtual void createfile(const createfile_args_t *args, int lineNo) = 0;
    virtual bool select(const select_args_t *args, int lineNo) = 0;
    virtual void recvfrom(const recvfrom_args_t *args, int lineNo) = 0;
    virtual void modsendfile(Buffer &buf, const sendfile_args_t *args, int lineNo) = 0;
    virtual void writev(const writev_args_t *args, int lineNo) = 0;
};
