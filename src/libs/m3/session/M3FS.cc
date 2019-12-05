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

#include <m3/com/GateStream.h>
#include <m3/session/M3FS.h>
#include <m3/vfs/RegularFile.h>

namespace m3 {

File *M3FS::open(const char *path, int perms) {
    int fd;
    // ensure that the message gets acked immediately.
    {
        GateIStream resp = send_receive_vmsg(_gate, OPEN, path, perms);
        resp >> Errors::last;
        if(Errors::last != Errors::NO_ERROR)
            return nullptr;
        resp >> fd;
    }
    return new RegularFile(fd, Reference<M3FS>(this), perms);
}

Errors::Code M3FS::stat(const char *path, FileInfo &info) {
    GateIStream reply = send_receive_vmsg(_gate, STAT, path);
    reply >> Errors::last;
    if(Errors::last != Errors::NO_ERROR)
        return Errors::last;
    reply >> info;
    return Errors::NO_ERROR;
}

int M3FS::fstat(int fd, FileInfo &info) {
    GateIStream reply = send_receive_vmsg(_gate, FSTAT, fd);
    reply >> Errors::last;
    if(Errors::last != Errors::NO_ERROR)
        return Errors::last;
    reply >> info;
    return Errors::NO_ERROR;
}

int M3FS::seek(int fd, off_t off, int whence, size_t &global, size_t &extoff, off_t &pos) {
    GateIStream reply = send_receive_vmsg(_gate, SEEK, fd, off, whence, global, extoff);
    reply >> Errors::last;
    if(Errors::last != Errors::NO_ERROR)
        return Errors::last;
    reply >> global >> extoff >> pos;
    return Errors::NO_ERROR;
}

Errors::Code M3FS::mkdir(const char *path, mode_t mode) {
    GateIStream reply = send_receive_vmsg(_gate, MKDIR, path, mode);
    reply >> Errors::last;
    return Errors::last;
}

Errors::Code M3FS::rmdir(const char *path) {
    GateIStream reply = send_receive_vmsg(_gate, RMDIR, path);
    reply >> Errors::last;
    return Errors::last;
}

Errors::Code M3FS::link(const char *oldpath, const char *newpath) {
    GateIStream reply = send_receive_vmsg(_gate, LINK, oldpath, newpath);
    reply >> Errors::last;
    return Errors::last;
}

Errors::Code M3FS::unlink(const char *path) {
    GateIStream reply = send_receive_vmsg(_gate, UNLINK, path);
    reply >> Errors::last;
    return Errors::last;
}

void M3FS::close(int fd, size_t extent, size_t off) {
    // wait for the reply because we want to get our credits back
    send_receive_vmsg(_gate, CLOSE, fd, extent, off);
}

Errors::Code M3FS::rename(const char *oldpath, const char *newpath) {
    GateIStream reply = send_receive_vmsg(_gate, RENAME, oldpath, newpath);
    reply >> Errors::last;
    return Errors::last;
}

}
