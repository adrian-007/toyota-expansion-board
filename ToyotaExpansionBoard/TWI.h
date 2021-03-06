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

#include <avr/io.h>
#include <util/twi.h>

#include "Serial.h"
#include "InterruptGuard.h"

struct TWI
{
    static constexpr auto TWIFrequency() { return 100000UL; }

    static void init()
    {
        TWSR &= ~((1 << TWPS0) | (1 << TWPS1));
        TWBR = ((F_CPU/TWIFrequency()) - 16) / 2;
    }

    static void start()
    {
        InterruptGuard ig {};
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
        while (!(TWCR & (1 << TWINT)));
        
        auto status { TW_STATUS & TW_STATUS_MASK };
        if (status != TW_START) {
            DEBUG_PRINT("Start failed, status: ", status);
        }
    }

    static void stop()
    {
        InterruptGuard ig {};
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
        while (!(TWCR & (1 << TWSTO)));
    }

    template<typename...Args>
    inline static void write(Args...data)
    {
        InterruptGuard ig {};
        TWI::writeImpl(data...);
    }

private:
    template<typename...Args>
    inline static void writeImpl(uint8_t data, Args...args)
    {
        writeImpl(data);
        writeImpl(args...);
    }
    
    inline static void writeImpl(uint8_t data)
    {
        TWDR = data;
        TWCR = (1 << TWINT) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT)));

        auto status { TW_STATUS & TW_STATUS_MASK };
        if (status != TW_MT_DATA_ACK && status != TW_MT_SLA_ACK) {
            DEBUG_PRINT("Write failed, status: ", status);
        }
    }    
};

struct ScopedTWI
{
    ScopedTWI(uint8_t address)
    {
        TWI::start();
        TWI::write(address << 1);
    }

    ~ScopedTWI()
    {
        TWI::stop();
    }

    template<typename...Args>
    inline void write(Args...data)
    {
        TWI::write(data...);
    }
};
