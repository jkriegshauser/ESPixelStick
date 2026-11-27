#pragma once
/******************************************************************
*
*       Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel (And Serial!) driver
*       Orginal ESPixelStickproject by copyright 2015 - 2026 Shelby Merrick
*
*       Brought to you by:
*              Bill Porter
*              www.billporter.info
*
*       See Readme for other info and version history
*
*
*This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*
*This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
*To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or
*send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
******************************************************************
*
*   This is a derived class that converts data in the output buffer into
*   Relay States and then outputs on a GPIO
*
*/
#include "ESPixelStick.h"

#ifdef SUPPORT_OutputProtocol_Relay

#include "OutputCommon.hpp"

class c_OutputRelay : public c_OutputCommon
{
public:
    // These functions are inherited from c_OutputCommon
    c_OutputRelay (OM_OutputPortDefinition_t & OutputPortDefinition,
                   c_OutputMgr::e_OutputProtocolType outputType);
    virtual ~c_OutputRelay ();

    // functions to be provided by the derived class
    void        Begin ();                                         ///< set up the operating environment based on the current config (or defaults)
    bool        SetConfig (ArduinoJson::JsonObject & jsonConfig); ///< Set a new config in the driver
    void        GetConfig (ArduinoJson::JsonObject & jsonConfig); ///< Get the current config used by the driver
    uint32_t    Poll ();                                        ///< Call from loop(),  renders output data
    #if defined(SUPPORT_RMT)
    bool        RmtPoll () {return false;}
    #endif // defined(SUPPORT_RMT)
    void        GetDriverName (String& sDriverName);
    void        GetStatus (ArduinoJson::JsonObject & jsonStatus);
    uint32_t    GetNumOutputBufferBytesNeeded () { return 1; }
    uint32_t    GetNumOutputBufferChannelsServiced () { return 1; }
    bool        ValidateGpio (gpio_num_t ConsoleTxGpio, gpio_num_t ConsoleRxGpio);
    void        RelayUpdate (String & NewValue, String & Response);

private:
#   define OM_RELAY_UPDATE_INTERVAL_NAME    CN_updateinterval
#   define OM_RELAY_CHANNEL_INVERT_NAME     CN_inv
#   define OM_RELAY_CHANNEL_PWM_NAME        CN_pwm

    bool    validate ();
    void    OutputValue (uint8_t NewValue);

    // config data
    bool        httpEnabled;
    bool        InvertOutput;
    bool        Pwm;
    uint8_t     OnOffTriggerLevel;
    gpio_num_t  GpioId;
    uint8_t     OnValue;
    uint8_t     OffValue;
    uint8_t     previousValue;
#if defined(ARDUINO_ARCH_ESP32)
    uint16_t    PwmFrequency;
#endif // defined(ARDUINO_ARCH_ESP32)
    uint16_t    UpdateInterval = 0;

}; // c_OutputRelay

#endif // def SUPPORT_OutputProtocol_Relay
