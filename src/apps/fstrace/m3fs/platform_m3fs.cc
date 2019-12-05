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

#include <m3/stream/Standard.h>

#include <stdarg.h>

#include "fsapi_m3fs.h"
#include "platform.h"

/*
 * *************************************************************************
 */

void Platform::init(int /*argc*/, const char * const * /*argv*/) {

}


FSAPI *Platform::fsapi(const char *root) {
    return new FSAPI_M3FS(root);
}


void Platform::shutdown() {

}


void Platform::checkpoint_fs() {

}


void Platform::sync_fs() {

}


void Platform::drop_caches() {

}


void Platform::log(const char *msg) {
    m3::cout << msg << "\n";
}

void Platform::logf(UNUSED const char *fmt, ...) {
#if defined(__host__)
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
}

static const size_t MAX_TMP_FILES   = 16;
static const bool VERBOSE           = 0;

void Platform::cleanup(const char *prefix) {
    m3::Dir dir(prefix);
    if(m3::Errors::occurred())
        return;

    size_t x = 0;
    m3::String *entries[MAX_TMP_FILES];

    if(VERBOSE)
        m3::cout << "Collecting files in " << prefix << "\n";

    // remove all entries; we assume here that they are files
    m3::Dir::Entry e;
    char path[128];
    while(dir.readdir(e)) {
        if(strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0 || strcmp(e.name, "") == 0)
            continue;

        m3::OStringStream file(path, sizeof(path));
        file << prefix << "/" << e.name;
        if(x > ARRAY_SIZE(entries))
            THROW("Too few entries");
        entries[x++] = new m3::String(file.str());
    }

    for(; x > 0; --x) {
        if(VERBOSE)
            m3::cout << "Unlinking " << *(entries[x - 1]) << "\n";
        m3::VFS::unlink(entries[x - 1]->c_str());
        delete entries[x - 1];
    }
}
