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
#include <util/delay.h>

#include "InterruptGuard.h"
#include "Serial.h"

#define DS_PORT       PORTD
#define DS_DDR        DDRD
#define DS_PIN        PIND
#define DS_PIN_NUM    PIND3

class DS18B20
{
public:
	static void init()
	{
		if (!oneWireInit()) {
			DEBUG_PRINT(" Sensor not present");
			return;
		}

		uint8_t sp[9] { 0 };
		bool changeResolution { true };

		if (readScratchPad(sp)) {
			auto resolution { sp[4] >> 5 };
			
			switch (resolution) {
				case 0:
					DEBUG_PRINT(" Sensor present, resolution: ", 9, "-bits");
					changeResolution = false;
					break;
				case 1:
					DEBUG_PRINT(" Sensor present, resolution: ", 10, "-bits");
					break;
				case 2:
					DEBUG_PRINT(" Sensor present, resolution: ", 11, "-bits");
					break;
				case 3:
					DEBUG_PRINT(" Sensor present, resolution: ", 12, "-bits");
					break;
				default:
					DEBUG_PRINT(" Sensor present, unknown resolution - ", ", read value: ", resolution);
					break;
			}
			
		}
		else {
			DEBUG_PRINT(" Sensor not present, cannot read scratchpad");
		}

		if (changeResolution) {
			// 9-bit resolution
			writeScratchPad(0xFF, 0xFF, 0x00);
			commitScratchPad();	
		}
	}

	/**
	 * Method should be called every second
	 */
	static void poll()
	{
		constexpr uint8_t samplesPerMinute { 12 };
		constexpr uint8_t ticksThreshold { 60 / samplesPerMinute };

		static volatile bool converting { false };
		static volatile uint8_t ticks { ticksThreshold };

		if (++ticks < ticksThreshold)
			return;

		if (!converting) {
			converting = startConversion();
			
			if (!converting) {
				lastTemperatureValue() = 90;
			}
		}
		else {
			converting = false;
			ticks = 0;

			lastTemperatureValue() = readTemp();
		}
	}

	static volatile int8_t& lastTemperatureValue()
	{
		static volatile int8_t lastTemp = 90;
		return lastTemp;
	}

private:
	enum Commands : uint8_t
	{
		SkipROM            = 0xCC,
		StartConversion    = 0x44,
		ReadScratchPad     = 0xBE,
		WriteScratchPad    = 0x4E,
		CommitScratchPad   = 0x48
	};

	static int8_t readTemp()
	{
		uint8_t sp[9] { 0 };
		if (!readScratchPad(sp))
			return 127;

		int16_t temp = ((int16_t)(sp[1] << 8) + sp[0]) / 16;
		return (int8_t)(temp);
	}

	static bool startConversion()
	{
		if (!oneWireInit()) {
			DEBUG_PRINT("Could not initialize 1-Wire reset pulse prior to ", "starting conversion");
			return false;
		}

		oneWireWrite(Commands::SkipROM, Commands::StartConversion);
		oneWireStrongPullup();
		return true;
	}

	static bool readScratchPad(uint8_t scratchPad[9])
	{
		if (!oneWireInit()) {
			DEBUG_PRINT("Could not initialize 1-Wire reset pulse prior to ", "reading scratch pad data");
			return false;
		}

		oneWireWrite(Commands::SkipROM, Commands::ReadScratchPad);

		scratchPad[0] = oneWireRead();
		scratchPad[1] = oneWireRead();
		scratchPad[2] = oneWireRead();
		scratchPad[3] = oneWireRead();
		scratchPad[4] = oneWireRead();
		scratchPad[5] = oneWireRead();
		scratchPad[6] = oneWireRead();
		scratchPad[7] = oneWireRead();
		scratchPad[8] = oneWireRead();

		return calculateCRC(&scratchPad[0], 8) == scratchPad[8];
	}

	static bool writeScratchPad(uint8_t th, uint8_t tl, uint8_t config)
	{
		if (!oneWireInit()) {
			DEBUG_PRINT("Could not initialize 1-Wire reset pulse prior to ", "writing scratch pad data");
			return false;
		}

		oneWireWrite(Commands::SkipROM, Commands::WriteScratchPad, th, tl, config);
		return true;
	}

	static bool commitScratchPad()
	{
		if (!oneWireInit()) {
			DEBUG_PRINT("Could not initialize 1-Wire reset pulse prior to ", "commiting scratch pad data to EEPROM");
			return false;
		}

		oneWireWrite(Commands::SkipROM, Commands::CommitScratchPad);
		oneWireStrongPullup();
		_delay_ms(15);
		return true;
	}

	inline static void oneWireStrongPullup()
	{
		pullUp();
		tx();
	}

	static bool oneWireInit()
	{
		InterruptGuard ig {};

		pullUp();
		tx();
		pullDown();

		_delay_us(500); // 480 us + 20 us of margin
		rx();
		_delay_us(80); // 60 us + 20 us of margin
		
		bool presence { false };
		uint16_t remainingSleep { 500 };

		while (remainingSleep > 0) {
			if (!presence && !pin()) {
				presence = true;
			}

			--remainingSleep;
			_delay_us(1);
		}

		pullUp();
		tx();

		return presence;
	}
	
	static void oneWireWrite(auto value)
	{
		static_assert(sizeof(value) == 1, "You may write at most one byte at a time");

		constexpr auto writeBit = [](bool bit) {
			InterruptGuard ig {};

			pullDown();
			bit ? _delay_us(8) : _delay_us(80);

			pullUp();
			bit ? _delay_us(80) : _delay_us(8);
		};

		pullUp();
		tx();

		writeBit(value & (1 << 0));
		writeBit(value & (1 << 1));
		writeBit(value & (1 << 2));
		writeBit(value & (1 << 3));
		writeBit(value & (1 << 4));
		writeBit(value & (1 << 5));
		writeBit(value & (1 << 6));
		writeBit(value & (1 << 7));
	}

	template<typename...A>
	inline static void oneWireWrite(auto value, A...args)
	{
		oneWireWrite(value);
		oneWireWrite(args...);
	}

	static uint8_t oneWireRead()
	{
		constexpr auto readBit = [](auto pos) -> uint8_t {
			InterruptGuard ig {};

			pullUp();
			tx();

			pullDown();
			_delay_us(2);

			rx();
			_delay_us(5);
		
			uint8_t bit = 0;

			for (auto i { 0u }; i < 5; ++i) {
				if (pin())
					bit = 1;
				_delay_us(1);
			}

			_delay_us(45);

			return bit << pos;
		};

		uint8_t byte = readBit(0) | readBit(1) | readBit(2) | readBit(3) | readBit(4) | readBit(5) | readBit(6) | readBit(7);
		return byte;
	}

	static uint8_t calculateCRC(uint8_t* data, uint8_t length)
	{
		uint8_t crc = 0;

		for (auto i = 0u; i < length; ++i) {
			auto byte { data[i] };

			for (auto j = 0u; j < 8; ++j) {
				auto mix { ( crc ^ byte ) & 1 };
				crc >>= 1;

				if (mix)
					crc ^= 0x8C;

				byte >>= 1;
			}
		}

		return crc;
	}

	static constexpr auto pinMask() { return 1 << DS_PIN_NUM; }

	inline static void pullUp() { DS_PORT |= pinMask(); }
	inline static void pullDown() { DS_PORT &= ~pinMask(); }
	inline static void tx() { DS_DDR |= pinMask(); }
	inline static void rx() { DS_DDR &= ~pinMask(); }
	inline static bool pin() { return DS_PIN & pinMask(); }
};
