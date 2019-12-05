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
#include <cstring>

#if defined(__gem5__)
const char *strstr(const char *str1, const char *str2) {
#else
char *strstr(const char *str1, const char *str2) {
#endif
    char *res = nullptr;
    char *sub;
    /* handle special case to prevent looping the string */
    if(!*str2)
        return (char*)str1;
    while(*str1) {
        /* matching char? */
        if(*str1++ == *str2) {
            res = (char*)--str1;
            sub = (char*)str2;
            /* continue until the strings don't match anymore */
            while(*sub && *str1 == *sub) {
                str1++;
                sub++;
            }
            /* complete substring matched? */
            if(!*sub)
                return res;
        }
    }
    return nullptr;
}
