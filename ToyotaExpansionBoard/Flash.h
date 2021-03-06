/*
 * Copyright (C) 2021 adrian_007, adrian-007 on o2 point pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#pragma once

#include <avr/pgmspace.h>

template<typename T>
inline T pgm_read(const T* ptr)
{
    switch (sizeof(T)) {
        case 1: return pgm_read_byte(ptr);
        case 2: return pgm_read_word(ptr);
        case 4: return pgm_read_dword(ptr);
        default: {
            T result;
            memcpy_P(&result, ptr, sizeof(T));
            return result;
        }
    }
}

template<typename T>
struct flash
{
    const T value;

    inline T get() const { return pgm_read(&value); }

    template<typename R = T>
    inline operator R() const { return pgm_read(&value); }
};
