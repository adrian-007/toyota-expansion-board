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

#include "Serial.h"
#include "MCP42100.h"
#include "SSD1306.h"
#include "DS18B20.h"
#include "ADCButtons.h"

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

ISR(ADC_vect)
{
    ADCButtons::instance().newSample(ADC);
}

ISR(TIMER0_COMPA_vect)
{
    ADCButtons::instance().poll();
}

ISR(TIMER1_COMPA_vect)
{
    DS18B20::poll();
}

void disable_wdt() __attribute__((naked, used, section(".init3")));

void disable_wdt()
{
    MCUSR = 0;
    wdt_disable();
}

void initTimer0()
{
    // CTC mode
    TCCR0A |= (1 << WGM01); 
    // Prescaler = 1024
    TCCR0B |= (1 << CS02) | (0 << CS01) | (1 << CS00);

    OCR0A = 32;
    // Enable COMPA interrupt
    TIMSK0 |= (1 << OCIE0A);
}

void initTimer1()
{
    // CTC mode, prescaler = 256
    TCCR1B |= (1 << WGM12) | (1 << CS12);
    // F_CPU / 256 => interrupt every second
    OCR1A = 15625;
    // Enable COMPA interrupt
    TIMSK1 |= (1 << OCIE1A);    
}

int main(void)
{
    Serial::init();
    
    DEBUG_PRINT("Initializing modules");

    TWI::init();
    SSD1306::init();
    DS18B20::init();
    ADCButtons::instance().init();

    initTimer0();
    initTimer1();

    // Enable Watchdog
    wdt_enable(WDTO_4S);

    // Enable interrupts
    sei();

    Serial::println("Entering main loop.");
    Serial::println();

    int8_t temp { 90 };
    while (true) {
        {
            InterruptGuard ig {};
            wdt_reset();
            temp = DS18B20::lastTemperatureValue();
        }

        SSD1306::drawTemp(temp);

        _delay_ms(200);
    }
}
