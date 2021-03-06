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

#include "Serial.h"
#include "MCP42100.h"
#include "Queue.h"

class ADCButtons
{
public:
    enum Button : uint8_t
    {
        None, // Special value indicating that no button has been pressed.
        Select,
        Next,
        Up,
        Prev,
        Down,
        Mute,
        OnOff,
        VolumeUp,
        VolumeDown,
        AnswerCall,
        HangUpCall,
        AddressBook
    };

    static ADCButtons& instance()
    {
        static ADCButtons instance;
        return instance;
    }

    void init()
    {
        MCP42100::init();

        // ADMUX 7:6 = 01 - set voltage reference to AVCC with external capacitor at AREF pin
        // ADMUX 3:0 = 0111 - select ADC7
        ADMUX |= (1 << REFS0) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);

        // 0:2 = 110 - prescaler fcpu/64 = 62,5 kHz
        ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1);
    }

    void poll()
    {
        if (_sampling) {
            _samplingTime += 10;
        }

        if (_eventCoolOffDuration < coolOffDuration()) {
            _eventCoolOffDuration += 10;
            return;
        }

        auto* event = _buttonEventQueue.peek();
        if (!event)
            return;

        if (event->elapsed >= event->duration) {
            // Event expired
            MCP42100::setPOT(POT_TIP_SHUTDOWN | POT_RING_SHUTDOWN, 0);
            _eventCoolOffDuration = 0;
            _buttonEventQueue.pop();

            DEBUG_PRINT("Pot shut down");
        }
        else {
            if (event->elapsed == 0) {
                if (event->pot == POT::Ring) {
                    DEBUG_PRINT("Setting ring POT to 800 ohms");
        
                    MCP42100::setPOT(POT_RING_ADDRESS, resistanceToPotValue(800));
                }
        
                DEBUG_PRINT("Setting tip POT to ", potValueToResistance(event->potValue), " ohms");
        
                MCP42100::setPOT(POT_TIP_ADDRESS, event->potValue);
                event->elapsed = 1;
            }
            else {
                event->elapsed += 10;
            }
        }
    }

    void newSample(uint16_t sample)
    {
        auto noButtonPressed { sample >= maxSampleValue() };

        if (noButtonPressed) {
            if (_sampling) {
                _sampling = false;

                if (_sampleCount < minSampleCount())
                    return;

                // Find out which button was pressed.

                for (const auto& buttonInfo : _buttonInfo) {
                    if (buttonInfo.isInRange(_sampleAvg)) {
                        auto button { _samplingTime < alternateFunctionSamplingTimeThreshold() ? buttonInfo.button : buttonInfo.alternateButton };

                        DEBUG_PRINT("Sample statistics",
                            ": min / avg / max: ", _sampleMin, " / ", _sampleAvg, " / ", _sampleMax,
                            ", sampling time: ", _samplingTime, ", sample count: ", _sampleCount,
                            ", Button: ", buttonName(button)
                        );

                        // Find POT definition for given button
                        for (const auto& buttonPotInfo : _buttonPotInfo) {
                            if (buttonPotInfo.button == button) {
                                auto isLongPress { _samplingTime >= alternateFunctionSamplingTimeThreshold() * 2 };

                                uint16_t duration;
                                if (buttonPotInfo.durationType == POTDurationType::Variable) {
                                    duration = durationTypeToDuration(isLongPress ? POTDurationType::Long : POTDurationType::Short);
                                }
                                else {
                                    duration = durationTypeToDuration(buttonPotInfo.durationType);
                                }

                                if (!_buttonEventQueue.push(ButtonPOTEvent(buttonPotInfo.button, buttonPotInfo.pot, buttonPotInfo.potValue, duration))) {
                                    DEBUG_PRINT("Could not queue POT event for button ", buttonNameString);
                                }
                            }
                        }

                        break;
                    }
                }    
            }
        }
        else
        {
            bool inAnyRange { false };
            for (const auto& button : _buttonInfo) {
                if (button.isInRange(sample)) {
                    inAnyRange = true;
                    break;
                }
            }

            if (!inAnyRange)
                return;

            if (!_sampling) {
                _sampling = true;
                _samplingTime = 0;
                _sampleCount = 0;
                _sampleAvg = _sampleMin = _sampleMax = sample;
            }

            if (++_sampleCount < maxSampleCount()) {
                if (sample < _sampleMin)
                    _sampleMin = sample;
                
                if (sample > _sampleMax)
                    _sampleMax = sample;
            
                _sampleAvg += sample;
                _sampleAvg /= 2;
            }
        }
    }

private:
    struct ButtonInfo
    {
        constexpr ButtonInfo(Button button, uint16_t minSample, uint16_t maxSample)
            : button { button }
            , minSample { minSample }
            , maxSample { maxSample }
            , alternateButton { button }
        { }

        constexpr ButtonInfo(Button button, uint16_t minSample, uint16_t maxSample, Button alternateButton)
            : button { button }
            , minSample { minSample }
            , maxSample { maxSample }
            , alternateButton { alternateButton }
        { }

        inline bool isInRange(uint16_t value) const
        {
            return value >= minSample && value <= maxSample;
        }

        const Button button;
        const uint16_t minSample;
        const uint16_t maxSample;
        const Button alternateButton;
    };

    enum class POT : uint8_t { None, Ring, Tip };
    enum class POTDurationType : uint8_t { Short, Long, Variable };

    struct ButtonPOTInfo
    {
        constexpr ButtonPOTInfo(Button button, POT pot, uint8_t potValue, POTDurationType durationType)
            : button { button }
            , pot { pot }
            , potValue { potValue }
            , durationType { durationType }
        { }

        const Button button;
        const POT pot;
        const uint8_t potValue;
        const POTDurationType durationType;
    };

    struct ButtonPOTEvent
    {
        constexpr ButtonPOTEvent() = default;
        constexpr ButtonPOTEvent(Button button, POT pot, uint8_t potValue, uint16_t duration)
            : button { button }
            , pot { pot }
            , potValue { potValue }
            , duration { duration }
            , elapsed { 0 }
        { }

        ButtonPOTEvent(const ButtonPOTEvent&) = default;
        ButtonPOTEvent& operator=(const ButtonPOTEvent& event) = default;

        inline bool isSet() const { return button != Button::None && pot != POT::None; }

        volatile Button button = Button::None;
        volatile POT pot = POT::None;
        volatile uint8_t potValue = 0;
        volatile uint16_t duration = 0;
        volatile uint16_t elapsed = 0;
    };

    static constexpr uint16_t maxSampleValue() { return 890; }
    static constexpr uint16_t maxSampleCount() { return 600; }
    static constexpr uint16_t minSampleCount() { return 200; }
    static constexpr uint16_t alternateFunctionSamplingTimeThreshold() { return 500; }
    static constexpr uint32_t maxPotResistance() { return 100000; }
    static constexpr uint8_t maxPotValue() { return 255; }
    static constexpr uint8_t coolOffDuration() { return 120; }

    static constexpr uint32_t potValueToResistance(uint8_t potValue)
    {
        return (maxPotValue() - potValue) * (maxPotResistance() / maxPotValue());
    }
    
    static constexpr uint8_t resistanceToPotValue(uint32_t resistance)
    {
        auto result { maxPotValue() - (maxPotValue() * resistance / maxPotResistance()) };
        if (!result)
            result += 1;
        return result;
    }

    static constexpr uint16_t durationTypeToDuration(POTDurationType durationType)
    {
         return durationType == POTDurationType::Long ? 700 : 80;
    }

    static constexpr const char* buttonName(Button button)
    {
        switch (button) {
            case Button::Select:        return "Select";
            case Button::Next:          return "Next";
            case Button::Up:            return "Up";
            case Button::Prev:          return "Previous";
            case Button::Down:          return "Down";
            case Button::Mute:          return "Mute";
            case Button::OnOff:         return "OnOff";
            case Button::VolumeUp:      return "Volume Up";
            case Button::VolumeDown:    return "Volume Down";
            case Button::AnswerCall:    return "Answer Call";
            case Button::HangUpCall:    return "Hang Up Call";
            case Button::AddressBook:   return "Address Book";
            default:                    return "?";
        }
    }

    const ButtonInfo _buttonInfo[6] {
        // Button                Min ADC value     Max ADC value    Alternate Button on long press
        { Button::Mute,          0,                100,             Button::OnOff       },
        { Button::VolumeDown,    160,              220,             Button::HangUpCall  },
        { Button::VolumeUp,      330,              390,             Button::AnswerCall  },
        { Button::Select,        490,              550,             Button::AddressBook },
        { Button::Next,          660,              720,             Button::Up          },
        { Button::Prev,          770,              830,             Button::Down        },
    };

    const ButtonPOTInfo _buttonPotInfo[12] {
        // Button                POT connection type    POT value                         Duration
        { Button::Select,        POT::Tip,              resistanceToPotValue(1200),       POTDurationType::Short    },
        { Button::Next,          POT::Tip,              resistanceToPotValue(8000),       POTDurationType::Short    },
        { Button::Up,            POT::Ring,             resistanceToPotValue(8000),       POTDurationType::Variable },
        { Button::Prev,          POT::Tip,              resistanceToPotValue(11250),      POTDurationType::Short    },
        { Button::Down,          POT::Ring,             resistanceToPotValue(11250),      POTDurationType::Variable },
        { Button::Mute,          POT::Tip,              resistanceToPotValue(3500),       POTDurationType::Short    },
        { Button::OnOff,         POT::Tip,              resistanceToPotValue(60000),      POTDurationType::Short    },
        { Button::VolumeUp,      POT::Tip,              resistanceToPotValue(16000),      POTDurationType::Short    },
        { Button::VolumeDown,    POT::Tip,              resistanceToPotValue(24000),      POTDurationType::Short    },
        { Button::AnswerCall,    POT::Ring,             resistanceToPotValue(3000),       POTDurationType::Short    },
        { Button::HangUpCall,    POT::Ring,             resistanceToPotValue(5500),       POTDurationType::Short    },
        { Button::AddressBook,   POT::Ring,             resistanceToPotValue(1200),       POTDurationType::Short    },
    };

    bool _sampling = false;
    uint16_t _samplingTime = 0;
    uint16_t _sampleCount = 0;
    uint16_t _sampleMax = 0;
    uint16_t _sampleMin = 0;
    uint16_t _sampleAvg = 0;

    Queue<ButtonPOTEvent, 10> _buttonEventQueue;
    uint8_t _eventCoolOffDuration = coolOffDuration();
};
