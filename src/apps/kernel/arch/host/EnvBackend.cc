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

#include <base/Common.h>
#include <base/Env.h>
#include <base/EnvBackend.h>

#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include "WorkLoop.h"

namespace kernel {

class HostKEnvBackend : public m3::HostEnvBackend {
public:
    explicit HostKEnvBackend() {
        _workloop = new WorkLoop();
    }

    virtual void exit(int) override {
    }
};

static const char *gen_prefix() {
    static char prefix[32];
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    srand(tv.tv_usec);
    snprintf(prefix, sizeof(prefix), "/m3-%d-", rand());
    return prefix;
}

EXTERN_C void init_env() {
    int logfd = open("run/log.txt", O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0644);

    new m3::Env(new HostKEnvBackend(), logfd);
    m3::env()->set_params(0, gen_prefix(), 0, 0, 0);
}

}
