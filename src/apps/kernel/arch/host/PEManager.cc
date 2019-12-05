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

#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <csignal>
#include <unistd.h>

#include "pes/PEManager.h"
#include "Platform.h"

namespace kernel {

PEManager::~PEManager() {
    for(size_t i = 0; i < Platform::pe_count(); ++i) {
        if(_vpes[i]) {
            kill(_vpes[i]->pid(), SIGTERM);
            waitpid(_vpes[i]->pid(), nullptr, 0);
            _vpes[i]->unref();
        }
    }
}

m3::String PEManager::fork_name(const m3::String &name) {
    char buf[256];
    m3::OStringStream nname(buf, sizeof(buf));
    size_t pos = strrchr(name.c_str(), '-') - name.c_str();
    nname << m3::fmt(name.c_str(), 1, pos) << '-' << rand();
    return nname.str();
}

}
