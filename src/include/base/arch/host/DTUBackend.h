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

#include <base/Config.h>
#include <base/DTU.h>

#include <sys/un.h>

namespace m3 {

class MsgBackend : public DTU::Backend {
    static constexpr int BASE_MSGQID        = 0x12340000;

public:
    virtual void create() override;
    virtual void destroy() override;
    virtual void reset() override;
    virtual void send(int core, int ep, const DTU::Buffer *buf) override;
    virtual ssize_t recv(int ep, DTU::Buffer *buf) override;

private:
    static key_t get_msgkey(int core, int rep) {
        return BASE_MSGQID + core * EP_COUNT + rep;
    }

    int _ids[EP_COUNT * MAX_CORES];
};

class SocketBackend : public DTU::Backend {
public:
    explicit SocketBackend();
    virtual void create() override {
    }
    virtual void destroy() override {
    }
    virtual void reset() override {
    }
    virtual void send(int core, int ep, const DTU::Buffer *buf) override;
    virtual ssize_t recv(int ep, DTU::Buffer *buf) override;

private:
    int _sock;
    int _localsocks[EP_COUNT];
    sockaddr_un _endpoints[MAX_CORES * EP_COUNT];
};

}
