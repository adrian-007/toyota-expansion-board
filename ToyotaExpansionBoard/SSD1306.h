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

#include "TWI.h"
#include "ThermometerFont.h"

#include <util/delay.h>

#define RESET_PIN  PIND0
#define RESET_PORT PORTD
#define RESET_DDR  DDRD

class SSD1306
{
    struct Commands
    {
        static constexpr uint8_t CommandTag                  { 0x00 };
        static constexpr uint8_t DataTag                     { 0x40 };

        static constexpr uint8_t DisplayOff                  { 0xAE };
        static constexpr uint8_t SetDisplayClockDiv          { 0xD5 };
        static constexpr uint8_t SetMultiplex                { 0xA8 };
        static constexpr uint8_t SetDisplayOffset            { 0xD3 };
        static constexpr uint8_t SetStartLine                { 0x40 };
        static constexpr uint8_t ChargePump                  { 0x8D };
        static constexpr uint8_t MemoryMode                  { 0x20 };
        static constexpr uint8_t SegRemap                    { 0xA0 };
        static constexpr uint8_t COMOutputScanDirNormal      { 0xC0 };
        static constexpr uint8_t COMPinsHWConfig             { 0xDA };
        static constexpr uint8_t SetContrast                 { 0x81 };
        static constexpr uint8_t SetPrecharge                { 0xD9 };
        static constexpr uint8_t SetVComDetect               { 0xDB };
        static constexpr uint8_t DisplayAllOnResume          { 0xA4 };
        static constexpr uint8_t NormalDisplay               { 0xA6 };
        static constexpr uint8_t InvertDisplay               { 0xA7 };
        static constexpr uint8_t DisplayOn                   { 0xAF };
        static constexpr uint8_t SetPageStartAddress         { 0xB0 };
        static constexpr uint8_t SetColumnLSB                { 0x00 };
        static constexpr uint8_t SetColumnMSB                { 0x10 };
        static constexpr uint8_t DeactivateScroll            { 0x2E };
        static constexpr uint8_t SetColumnAddress            { 0x21 };
        static constexpr uint8_t SetPageAddress              { 0x22 };

        struct MemoryMode
        {
            static constexpr uint8_t Horizontal { 0x00 };
            static constexpr uint8_t Vertical { 0x01 };
            static constexpr uint8_t Page { 0x02 };
        };
    };

    static constexpr uint8_t _displayHeight { 64 };
    static constexpr uint8_t _displayWidth { 128 };
    static constexpr uint8_t _pageCount { _displayHeight / 8 };
    static constexpr uint8_t _address { 0x3C };

public:
    static void init()
    {
        // Set up RESET pin as output
        RESET_DDR &= ~(1 << RESET_PIN);

        // Pull it down and keep for 10 us
        RESET_PORT &= ~(1 << RESET_PIN);
        _delay_us(10);
        
        // Pull high to enable the display
        RESET_PORT |= (1 << RESET_PIN);

        sendCommand(
            Commands::DisplayOff,
            Commands::SetMultiplex, 0x3F,
            Commands::SetDisplayOffset, 0x00,
            Commands::SetStartLine,
            Commands::SegRemap,
            Commands::SetDisplayClockDiv, 0x80,
            Commands::COMOutputScanDirNormal,
            Commands::COMPinsHWConfig, 0x12,
            Commands::SetContrast, 0x80,
            Commands::DisplayAllOnResume,
            Commands::NormalDisplay,
            Commands::ChargePump, 0x14,
            Commands::SetPrecharge, 0xF1,
            Commands::MemoryMode, Commands::MemoryMode::Vertical,
            Commands::SetVComDetect, 0x20,
            Commands::DeactivateScroll
        );

        clearDisplay();

        sendCommand(Commands::DisplayOn);

        drawTemp(127);
    }

    static void clearDisplay() {
        setDrawRect(0, _displayWidth - 1, 0, _pageCount - 1);

        ScopedTWI twi { _address };
        twi.write(Commands::DataTag);

        for (auto w { 0u }; w < _displayWidth / 32; ++w) {
            for (auto h { 0u }; h < _displayHeight; ++h) {
                twi.write(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
        }
    }

    static void drawTemp(int8_t temp)
    {
        bool negative { temp < 0 };
        if (negative)
            temp *= -1;

        int8_t chars[5] = { '-', '-', '-', '-', '-' };
        static int8_t prevChars[sizeof(chars)] { 0 };
    
        if (temp < 70) {
            int8_t tempDigits[2] { int8_t(temp / 10), int8_t(temp % 10) };

            int pos = sizeof(chars);
            chars[--pos] = 'C';
            chars[--pos] = '*';

            chars[--pos] = '0' + tempDigits[1];

            if (tempDigits[0] > 0)
                chars[--pos] = '0' + tempDigits[0];

            if (negative)
                chars[--pos] = '-';
            
            while (pos > 0)
                chars[--pos] = ' ';

            bool redraw { false };
            for (auto i { 0u }; i < sizeof(chars); ++i) {
                if (prevChars[i] != chars[i]) {
                    redraw = true;
                    break;
                }
            }

            if (!redraw) {
                return;
            }
        }

        const uint8_t pageOffset { 1u };
        uint8_t columnOffset { 0u };

        for (auto i = 0u; i < sizeof(chars); ++i) {
            if (chars[i] != prevChars[i]) {
                drawChar<ThermometerFont::Handler>(chars[i], pageOffset, columnOffset);
                prevChars[i] = chars[i];
            } 

            columnOffset += ThermometerFont::Handler::width(chars[i]);
        }
    }

private:
    template<typename FontHandler, typename SymbolType>
    static void drawChar(SymbolType symbol, uint8_t pageStart = 0u, uint8_t offset = 0u)
    {
        auto* charData = FontHandler::dataForSymbol(symbol);
        
        if (charData == nullptr)
            return;
        
        const auto pages { FontHandler::height() / 8 };
        const auto width { FontHandler::width() };

        setDrawRect(offset, offset + width, pageStart, pageStart + pages - 1);

        ScopedTWI twi { _address };
        twi.write(Commands::DataTag);

        for (auto p { 0u }; p < pages; ++p) {
            for (auto w { 0u }; w < width; ++w) {
                twi.write((++charData)->get());
            }
        }
    }

    static void setDrawRect(uint8_t columnBegin, uint8_t columnEnd, uint8_t pageBegin, uint8_t pageEnd)
    {
        sendCommand(
            Commands::SetColumnAddress, columnBegin, columnEnd,
            Commands::SetPageAddress, pageBegin, pageEnd
        );
    }

    template<typename...Args>
    inline static void sendCommand(Args...commands)
    {
        ScopedTWI twi { _address };
        twi.write(Commands::CommandTag, commands...);
    }
};
