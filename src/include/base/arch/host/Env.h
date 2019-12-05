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

#include <base/util/String.h>
#include <base/util/BitField.h>
#include <base/EnvBackend.h>
#include <base/PEDesc.h>

#include <m3/com/RecvBuf.h>

#include <pthread.h>
#include <assert.h>
#include <string>

namespace m3 {

class Env;
class RecvGate;

class HostEnvBackend : public EnvBackend {
    friend class Env;

    void exit(int) override {
    }

public:
    explicit HostEnvBackend();
    virtual ~HostEnvBackend();
};

class Env {
    struct Init {
        Init();
        ~Init();
    };
    struct PostInit {
        PostInit();
    };

public:
    static Env &get() {
        assert(_inst != nullptr);
        return *_inst;
    }

    static const char *executable_path() {
        if(*_exec == '\0')
            init_executable();
        return _exec;
    }
    static const char *executable() {
        if(_exec_short_ptr == nullptr)
            init_executable();
        return _exec_short_ptr;
    }

    explicit Env(EnvBackend *backend, int logfd);
    ~Env();

    void reset();

    WorkLoop *workloop() {
        return backend->_workloop;
    }

    RecvGate *mem_rcvgate() {
        return _mem_recvgate;
    }
    bool is_kernel() const {
        return coreid == 0;
    }
    int log_fd() const {
        return _logfd;
    }
    void log_lock() {
        pthread_mutex_lock(&_log_mutex);
    }
    void log_unlock() {
        pthread_mutex_unlock(&_log_mutex);
    }
    const String &shm_prefix() const {
        return _shm_prefix;
    }
    void print() const;

    void init_dtu();
    void set_params(int core, const std::string &shmprefix, label_t sysc_label,
                    size_t sysc_epid, word_t sysc_credits) {
        coreid = core;
        pe = PEDesc(PEType::COMP_IMEM, 1024 * 1024);
        _shm_prefix = shmprefix.c_str();
        _sysc_label = sysc_label;
        _sysc_epid = sysc_epid;
        _sysc_credits = sysc_credits;
    }

    void exit(int code) NORETURN {
        ::exit(code);
    }

private:
    void init_syscall(void *sepregs);

    static int set_inst(Env *e) {
        _inst = e;
        // coreid
        return 0;
    }
    static void init_executable();

public:
    int coreid;
    EnvBackend *backend;
    PEDesc pe;

private:
    int _logfd;
    String _shm_prefix;
    label_t _sysc_label;
    size_t _sysc_epid;
    word_t _sysc_credits;
    pthread_mutex_t _log_mutex;
    RecvBuf _mem_recvbuf;
    RecvGate *_mem_recvgate;

    static const char *_exec_short_ptr;
    static char _exec[];
    static char _exec_short[];
    static Env *_inst;
    static Init _init;
    static PostInit _postInit;
};

static inline Env *env() {
    return &Env::get();
}

}
