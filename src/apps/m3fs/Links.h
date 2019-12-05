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

#include "FSHandle.h"

class Links {
    Links() = delete;

public:
    static m3::Errors::Code create(FSHandle &h, m3::INode *dir, const char *name, size_t namelen, m3::INode *inode);
    static m3::Errors::Code remove(FSHandle &h, m3::INode *dir, const char *name, size_t namelen, bool isdir);
    static m3::Errors::Code rename(FSHandle &h, m3::INode *olddir, const char *oldname, size_t oldnamelen,
        m3::INode *newdir, const char *newname, size_t newnamelen, bool isdir);
};
