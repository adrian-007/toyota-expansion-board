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

#ifndef F_CPU
# error "F_CPU not defined"
#endif

#define SERIAL_BAUD_RATE 19200UL

#include <avr/io.h>
#include <stdint.h>

class Serial
{
public:
    Serial() = delete;

    enum class Base { Bin, Oct, Dec, Hex };    

    static void init()
    {
        UBRR0 = ((F_CPU / (8UL * SERIAL_BAUD_RATE)) - 1UL);
        UCSR0A |= (1 << U2X0);
        UCSR0B |= (1 << TXEN0);
        UCSR0C |= (3 << UCSZ00);
    }

    static inline void print(uint8_t value, Base base = Base::Dec)  { print_number(value, base); }
    static inline void print(uint16_t value, Base base = Base::Dec) { print_number(value, base); }
    static inline void print(uint32_t value, Base base = Base::Dec) { print_number(value, base); }
    static inline void print(uint64_t value, Base base = Base::Dec) { print_number(value, base); }
    static inline void print(int16_t value, Base base = Base::Dec)  { print_number(value, base); }
    static inline void print(int32_t value, Base base = Base::Dec)  { print_number(value, base); }
    static inline void print(int64_t value, Base base = Base::Dec)  { print_number(value, base); }

    static inline void print(char value)
    {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = value;
    }

    static void print(const char* string)
    {
        while (*string) {
            print(*string);
            ++string;
        }
    }

    template<typename...A>
    static inline void print(auto value, A...args)
    {
        print(value);
        print(args...);
    }

    template<typename...A>
    static void println(A...args)
    {
        print(args..., "\r\n");
    }

private:
    static constexpr auto baseDiv(Base base)
    {
        switch (base) {
            case Base::Bin: return 2;
            case Base::Oct: return 8;
            case Base::Dec: return 10;
            case Base::Hex: return 16;
            default: return 10;
        }
    }

    static char digitToChar(uint8_t digit)
    {
        static const char digits[16] {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        };

        return digits[digit];
    }

    static void print_number(auto number, Base base) {
        if (number < 0) {
            print('-');
            number *= -1;
        }

        auto div { baseDiv(base) };

        auto d { number / div };
        auto r { number % div };
        if (d != 0) {
            print_number(d, base);
        }

        print(digitToChar(r));
    }    
};

#ifdef ENABLE_UART_LOGGING
# define DEBUG_PRINT(...) Serial::println(__VA_ARGS__)
#else
# define DEBUG_PRINT(...)
#endif
