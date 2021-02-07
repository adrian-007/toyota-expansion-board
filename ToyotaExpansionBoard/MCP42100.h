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

#define MCP42100_CS_PORT    PORTB
#define MCP42100_CS_DDR     DDRB
#define MCP42100_CS_PIN     PINB2

#define POT_RING_ADDRESS    0x11
#define POT_TIP_ADDRESS     0x12

#define POT_RING_SHUTDOWN   0x21
#define POT_TIP_SHUTDOWN    0x22

#define SPI_PORT            PORTB
#define SPI_DDR             DDRB
#define MOSI_PIN            PINB3
#define MISO_PIN            PINB4
#define SCK_PIN             PINB5

class MCP42100
{
public:
	MCP42100() = delete;

	static void init()
	{
		MCP42100_CS_DDR |= (1 << MCP42100_CS_PIN);

		// Set MOSI and SCK output, all others input
		SPI_DDR |= (1 << MOSI_PIN) | (1 << SCK_PIN);

		// Enable SPI, master, set clock rate fck/16
		SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

		setPOT(POT_TIP_ADDRESS | POT_RING_ADDRESS, 0);
		setPOT(POT_TIP_SHUTDOWN | POT_RING_SHUTDOWN, 0);
	}

	static void setPOT(uint8_t address, uint8_t value)
	{
		MCP42100_CS_PORT &= ~(1 << MCP42100_CS_PIN);

		SPDR = address;
		while (!(SPSR & (1 << SPIF)));
		
		SPDR = value;
		while (!(SPSR & (1 << SPIF)));

		MCP42100_CS_PORT |= (1 << MCP42100_CS_PIN);
	}
};
