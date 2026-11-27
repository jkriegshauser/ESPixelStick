#pragma once
/*
* OutputUCS8903I2S.h - UCS8903 driver code for ESPixelStick I2S Channel
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2015, 2026 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*   This is a derived class that converts data in the output buffer into
*   pixel intensities and then transmits them through the configured serial
*   interface.
*
*/
#include "ESPixelStick.h"
#if defined(SUPPORT_OutputProtocol_UCS8903) && defined(SUPPORT_I2S)

#include "OutputUCS8903.hpp"
#include "OutputI2S.hpp"

class c_OutputUCS8903I2S : public c_OutputUCS8903
{
public:
    // These functions are inherited from c_OutputCommon
    c_OutputUCS8903I2S (OM_OutputPortDefinition_t & OutputPortDefinition,
                       c_OutputMgr::e_OutputProtocolType outputType);
    virtual ~c_OutputUCS8903I2S ();

    // functions to be provided by the derived class
    void    Begin ();                                        ///< set up the operating environment based on the current config (or defaults)
    bool    SetConfig (ArduinoJson::JsonObject& jsonConfig); ///< Set a new config in the driver
    uint32_t Poll () {return 0;}                             ///< Call from loop (),  renders output data
    void    GetStatus (ArduinoJson::JsonObject& jsonStatus);
    void    SetOutputBufferSize (uint32_t NumChannelsAvailable);
    void    PauseOutput (bool State);
    bool    ISR_GetNextDataSlicesToSend (c_OutputI2S::I2S_Item_t * DataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);
    void    ISR_StartNewDataFrame ();
    void    SetDefaultDataValue (c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);

private:
    void    CalculateFrameBitSlices ();
    bool    ISR_SetUpNextDataToSend ();

    c_OutputI2S     *I2Sdriver;

    uint32_t        FrameResetSliceCount;
    uint32_t        FrameResetRemainingSliceCount;

    uint32_t        DataPattern;
    uint32_t        DataPatternMask;

    c_OutputI2S::I2SDataAsUint_t OneBit;
    c_OutputI2S::I2SDataAsUint_t ZeroBit;

    c_OutputI2S::I2S_Item_t DataBitMask;
    c_OutputI2S::I2S_Item_t DataBit;

    // #define UCS8903_I2S_DEBUG_COUNTERS
    #ifdef UCS8903_I2S_DEBUG_COUNTERS
    #define INC_UCS8903_I2S_DEBUG_COUNTER(c) (I2SDebugCounters.c++)
    struct
    {
        uint32_t GetDataSlicesI;
        uint32_t GetDataSlicesII;
        uint32_t GetDataSlicesIII;
        uint32_t FrameStarts;
        uint32_t FrameEnds;
        uint32_t FrameReset;
        uint32_t FrameResetI;
        uint32_t FrameResetII;
        uint32_t FrameResetIII;
        uint32_t FrameResetAlignments;
        uint32_t FrameResetDone;
        uint32_t IdleSliceCount;
        uint32_t DataI;
        uint32_t DataII;
        uint32_t DataIII;
        uint32_t DataIV;
        uint32_t DataByteCount;
        uint32_t UnKnownFrameState;
    } I2SDebugCounters;
    #else
    #define INC_UCS8903_I2S_DEBUG_COUNTER(c)
    #endif // def UCS8903_I2S_DEBUG_COUNTERS

    enum OutputUCS8903I2S_FSM_States
    {
        _OutputUCS8903I2S_FSM_State_FrameReset,
        _OutputUCS8903I2S_FSM_State_Data,
        _OutputUCS8903I2S_FSM_State_Idle
    };
    OutputUCS8903I2S_FSM_States volatile OutputUCS8903I2S_FSM_State = OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle;

}; // c_OutputUCS8903I2S

#endif // defined(SUPPORT_OutputProtocol_UCS8903) && defined(SUPPORT_I2S)
