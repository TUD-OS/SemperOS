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

#include <base/arch/host/SharedMemory.h>

#include "Device.h"

namespace kernel {

class VGAConsoleDevice : public Device {
public:
    explicit VGAConsoleDevice();
    ~VGAConsoleDevice();

    virtual void run() override;

private:
    void check();

    m3::SharedMemory _vgamem;
    m3::SharedMemory _kbdmem;
};

}
