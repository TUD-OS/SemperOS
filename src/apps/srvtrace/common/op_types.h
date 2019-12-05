// vim:ft=cpp
/*
 * (c) 2011 Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING-GPL-2 file for details.
 */

#ifndef __TRACE_BENCH_OP_TYPES_H
#define __TRACE_BENCH_OP_TYPES_H

#ifdef __cplusplus
#define BEGIN_EXTERN_C extern "C" {
#define END_EXTERN_C } // extern "C"
#else
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#endif

/*
 * *************************************************************************
 */

// #ifndef _LARGEFILE64_SOURCE
// # define _LARGEFILE64_SOURCE
// #endif
// #ifndef _FILE_OFFSET_BITS
// # define _FILE_OFFSET_BITS 64
// #endif

#if defined(__LINUX__)
// for O_CLOEXEC and O_DIRECTORY
#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE
#   endif
#   include <sys/types.h>
#   include <stdint.h>
#   include <unistd.h>
#   include <fcntl.h>
#else
#   include <stddef.h>
#   include <stdint.h>
#   include <stdlib.h>
#   include <stdio.h>
#   ifndef O_RDWR
#       define O_RDONLY     1
#       define O_WRONLY     2
#       define O_RDWR       3
#       define O_TRUNC      8
#       define O_APPEND     16
#       define O_CREAT      32
#       define O_LARGEFILE  0
#       define O_EXCL       0
#       define O_NONBLOCK   0
#       define O_CLOEXEC    0
#       define O_DIRECTORY  2048
#   endif
#   ifndef AT_FDCWD
#       define AT_FDCWD     -100
#   endif
#   ifndef O_DIRECTORY
#       ifdef __O_DIRECTORY
#           define O_DIRECTORY   __O_DIRECTORY
#       else
#          define O_DIRECTORY   0
#       endif
#   endif
#   ifndef O_NOFOLLOW
#       ifdef __O_NOFOLLOW
#           define O_NOFOLLOW   __O_NOFOLLOW
#       else
#          define O_NOFOLLOW   0
#       endif
#   endif
#   ifndef O_CLOEXEC
#       ifdef __O_CLOEXEC
#           define O_CLOEXEC   __O_CLOEXEC
#       else
#          define O_CLOEXEC   0
#       endif
#   endif
#   ifndef O_NOCTTY
#       ifdef __O_NOCTTY
#           define O_NOCTTY     __O_NOCTTY
#       else
#          define O_NOCTTY      0
#       endif
#   endif
#   ifndef __cplusplus
typedef long off_t;
#   endif
#endif


/*
 * *************************************************************************
 */

BEGIN_EXTERN_C

/*
 * *************************************************************************
 */

#ifndef O_EXLOCK
# define O_EXLOCK 0
#endif

#ifndef O_EVTONLY
# define O_EVTONLY 0
#endif

#ifndef O_SHLOCK
# define O_SHLOCK 0
#endif

/*
 * *************************************************************************
 */

typedef enum {
    INVALID_OP,
    WAITUNTIL_OP,
    OPEN_OP,
    CLOSE_OP,
    FSYNC_OP,
    READ_OP,
    WRITE_OP,
    PREAD_OP,
    PWRITE_OP,
    LSEEK_OP,
    FTRUNCATE_OP,
    FSTAT_OP,
    STAT_OP,
    RENAME_OP,
    UNLINK_OP,
    RMDIR_OP,
    MKDIR_OP,
    SENDFILE_OP,
    GETDENTS_OP,
    CREATEFILE_OP,
    NEWFSTATAT_OP,
    OPENAT_OP,
    SELECT_OP,
    RECVFROM_OP,
    MODSENDFILE_OP,
    WRITEV_OP,
} trace_opcode_t;

/*
 * *************************************************************************
 */

typedef struct { int err; uint64_t timestamp; } waituntil_args_t;
typedef struct { int fd; char const * name; int flags; int mode; } open_args_t;
typedef struct { int fd; int dirfd; char const * name; int flags; int mode; } openat_args_t;
typedef struct { int err; int fd; } close_args_t;
typedef struct { int err; int fd; } fsync_args_t;
typedef struct { int err; int fd; size_t size; unsigned count; } read_args_t;
typedef struct { int err; int fd; size_t size; unsigned count; } write_args_t;
typedef struct { int err; int fd; size_t size; off_t offset; } pread_args_t;
typedef struct { int err; int fd; size_t size; off_t offset; } pwrite_args_t;
typedef struct { off_t err; int fd; off_t offset; int whence; } lseek_args_t;
typedef struct { int err; int fd; off_t offset; } ftruncate_args_t;
typedef struct { int err; int fd; } fstat_args_t;
typedef struct { int err; char const * name; } stat_args_t;
typedef struct { int err; char const * from; char const * to; } rename_args_t;
typedef struct { int err; char const * name; } unlink_args_t;
typedef struct { int err; char const * name; } rmdir_args_t;
typedef struct { int err; char const * name; int mode; } mkdir_args_t;
typedef struct { int err; int out_fd; int in_fd; off_t *offset; size_t count; } sendfile_args_t;
typedef struct { int err; int fd; int count; size_t bufsize; } getdents_args_t;
typedef struct { int err; char const * name; int mode; off_t size; } createfile_args_t;
typedef struct { int err; int dirfd; char const * name; } newfstatat_args_t;
typedef struct { int tv_sev; unsigned tv_usec; } timeval_t;
typedef struct { int err; int nfds; int * readfds; int * writefds;
    int * exceptfds; timeval_t * timeout; } select_args_t;
typedef struct { int todo; } sockaddr_t; // TODO
typedef struct { int err; int socket; void * buffer; size_t length; int flags;
    sockaddr_t * address; size_t * address_len; } recvfrom_args_t;
typedef struct { char const * iov_base; size_t iov_len; } iovec_t;
typedef struct { int err; int fildes; iovec_t iovec; int iovcnt; } writev_args_t;

typedef struct {
    int opcode;
    union {
        waituntil_args_t  waituntil;
        open_args_t       open;
        openat_args_t     openat;
        close_args_t      close;
        fsync_args_t      fsync;
        read_args_t       read;
        write_args_t      write;
        pread_args_t      pread;
        pwrite_args_t     pwrite;
        lseek_args_t      lseek;
        ftruncate_args_t  ftruncate;
        fstat_args_t      fstat;
        stat_args_t       stat;
        rename_args_t     rename;
        unlink_args_t     unlink;
        rmdir_args_t      rmdir;
        mkdir_args_t      mkdir;
        sendfile_args_t   sendfile;
        getdents_args_t   getdents;
        createfile_args_t createfile;
        newfstatat_args_t newfstatat;
        select_args_t     select;
        recvfrom_args_t   recvfrom;
        writev_args_t     writev;
    } args;
} trace_op_t;

/*
 * *************************************************************************
 */

extern trace_op_t trace_ops[];

/*
 * *************************************************************************
 */

END_EXTERN_C

#endif // __TRACE_BENCH_OP_TYPES_H
