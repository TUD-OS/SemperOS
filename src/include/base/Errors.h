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

namespace m3 {

/**
 * The error codes for M3
 */
struct Errors {
    enum Code {
        NO_ERROR,
        // DTU errors
        MISS_CREDITS,
        NO_RING_SPACE,
        VPE_GONE,
        NO_MAPPING,
        // SW errors
        INV_ARGS,
        OUT_OF_MEM,
        NO_SUCH_FILE,
        NO_PERM,
        NOT_SUP,
        NO_FREE_CORE,
        INVALID_ELF,
        NO_SPACE,
        EXISTS,
        XFS_LINK,
        DIR_NOT_EMPTY,
        IS_DIR,
        IS_NO_DIR,
        EP_INVALID,
        RECV_GONE,
        END_OF_FILE,
        MSGS_WAITING,
        XFS_RENAME,
    };

    /**
     * @param code the error code
     * @return the statically allocated error message for <code>
     */
    static const char *to_string(Code code);

    /**
     * @return true if an error occurred
     */
    static bool occurred() {
        return last != NO_ERROR;
    }

    /**
     * @return the last error code
     */
    static Code last;
};

}
