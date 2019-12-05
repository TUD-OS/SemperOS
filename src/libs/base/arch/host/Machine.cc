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

#include <base/stream/Serial.h>
#include <base/Env.h>
#include <base/Machine.h>

#include <sys/file.h>
#include <unistd.h>

namespace m3 {

void Machine::shutdown() {
    exit(EXIT_FAILURE);
}

int Machine::write(const char *str, size_t len) {
    return ::write(env()->log_fd(), str, len);
}

ssize_t Machine::read(char *buf, size_t len) {
    return ::read(STDIN_FILENO, buf, len);
}

}
