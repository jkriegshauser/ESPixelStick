/*
* OutputUCS8903I2S.cpp - UCS8903 driver code for ESPixelStick I2S Channel
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
*/
#include "ESPixelStick.h"
#if defined (SUPPORT_OutputProtocol_UCS8903) && defined (SUPPORT_I2S)

#include "output/OutputUCS8903I2S.hpp"
#include "output/OutputMgr.hpp"

//----------------------------------------------------------------------------
#ifdef UCS8903_I2S_DEBUG_COUNTERS
static uint32_t StartNewFrameBaseCount = 0;
static uint32_t GetDataSlicesToSendBaseCount = 0;
static uint32_t SetDefaultDataValueBaseCount = 0;
#endif // def UCS8903_I2S_DEBUG_COUNTERS

//----------------------------------------------------------------------------
static void IRAM_ATTR StartNewFrameBase (void * arg)
{
    #ifdef UCS8903_I2S_DEBUG_COUNTERS
    StartNewFrameBaseCount++;
    #endif // def UCS8903_I2S_DEBUG_COUNTERS
    
    reinterpret_cast<c_OutputUCS8903I2S*> (arg)->ISR_StartNewDataFrame ();
} // StartNewFrameBase

//----------------------------------------------------------------------------
// return false if there are no more data slices to send
static bool IRAM_ATTR GetDataSlicesToSendBase (void * arg, c_OutputI2S::I2S_Item_t * DataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef UCS8903_I2S_DEBUG_COUNTERS
    ++GetDataSlicesToSendBaseCount;
    #endif // def UCS8903_I2S_DEBUG_COUNTERS

    return reinterpret_cast<c_OutputUCS8903I2S*> (arg)->ISR_GetNextDataSlicesToSend (DataToSend, numSlices, DefaultIntensitySliceData);
} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
static void SetDefaultDataValueBase (void * arg, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef UCS8903_I2S_DEBUG_COUNTERS
    ++SetDefaultDataValueBaseCount;
    #endif // def UCS8903_I2S_DEBUG_COUNTERS

    reinterpret_cast<c_OutputUCS8903I2S*> (arg)->SetDefaultDataValue (DefaultIntensitySliceData);
} // SetDefaultDataValueBase

//----------------------------------------------------------------------------
c_OutputUCS8903I2S::c_OutputUCS8903I2S (OM_OutputPortDefinition_t & OutputPortDefinition,
                                      c_OutputMgr::e_OutputProtocolType outputType) :
    c_OutputUCS8903 (OutputPortDefinition, outputType)
{
    // DEBUG_START;

    // DEBUG_V (String ("UCS8903_PIXEL_I2S_TICKS_BIT_0_H: 0x") + String (UCS8903_PIXEL_I2S_TICKS_BIT_0_HIGH, HEX));
    // DEBUG_V (String ("UCS8903_PIXEL_I2S_TICKS_BIT_0_L: 0x") + String (UCS8903_PIXEL_I2S_TICKS_BIT_0_LOW,  HEX));
    // DEBUG_V (String ("UCS8903_PIXEL_I2S_TICKS_BIT_1_H: 0x") + String (UCS8903_PIXEL_I2S_TICKS_BIT_1_HIGH, HEX));
    // DEBUG_V (String ("UCS8903_PIXEL_I2S_TICKS_BIT_1_L: 0x") + String (UCS8903_PIXEL_I2S_TICKS_BIT_1_LOW,  HEX));

    OutputUCS8903I2S_FSM_State = OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle;

    // DEBUG_END;

} // c_OutputUCS8903I2S

//----------------------------------------------------------------------------
c_OutputUCS8903I2S::~c_OutputUCS8903I2S ()
{
    // DEBUG_START;
    if (HasBeenInitialized)
    {
        I2Sdriver->RemoveSlotDevice (OutputPortDefinition.PortId);
    }

    // DEBUG_END;
} // ~c_OutputUCS8903I2S

//----------------------------------------------------------------------------
void c_OutputUCS8903I2S::Begin ()
{
    // DEBUG_START;

    I2Sdriver = OutputMgr.GetI2sDriver ();

    c_OutputUCS8903::Begin ();

    // make sure we are in the paused state
    PauseOutput (true);

    // DEBUG_V (String ("DataPin: ") + String (DataPin));

    FrameResetSliceCount        = 0;
    FrameResetRemainingSliceCount = 0;

    c_OutputI2S::OutputI2SChannelConfig_t OutputI2SConfig;
    OutputI2SConfig.I2SChannelId              = uint32_t (OutputPortDefinition.PortId);
    OutputI2SConfig.DataPin                   = gpio_num_t (OutputPortDefinition.gpios.data);
    OutputI2SConfig.arg                       = this;
    OutputI2SConfig.StartNewFrame             = StartNewFrameBase;
    OutputI2SConfig.GetNextIntensityBitSlices = GetDataSlicesToSendBase;
    OutputI2SConfig.SetDefaultDataValue       = SetDefaultDataValueBase;
    OutputI2SConfig.IdleLevelIsHigh           = true; // idle is low
    OutputI2SConfig.IsEnabled                  = false; // do not output data

    I2Sdriver->RegisterSlotDevice (OutputI2SConfig, DataBit, DataBitMask);

    // DEBUG_V (String ("    DataBit: 0x") + String (DataBit, HEX));
    // DEBUG_V (String ("DataBitMask: 0x") + String (DataBitMask, HEX));

    HasBeenInitialized = true;

    // DEBUG_END;

} // Begin

//----------------------------------------------------------------------------
void c_OutputUCS8903I2S::CalculateFrameBitSlices ()
{
    // DEBUG_START;

    uint32_t MinFrameLenNS = 25 * NanoSecondsInAMilliSecond;
    uint32_t FrameDurationInNS = ActualFrameDurationMicroSec * NanoSecondsInAMicroSecond;
    // DEBUG_V (String (" ActualFrameDurationMicroSec: ") + String (ActualFrameDurationMicroSec));
    // DEBUG_V (String ("           FrameDurationInNS: ") + String (FrameDurationInNS));
    // DEBUG_V (String ("     InterFrameGapInMicroSec: ") + String (InterFrameGapInMicroSec));

    uint32_t InterFrameGapInNS = InterFrameGapInMicroSec * NanoSecondsInAMicroSecond;
    // DEBUG_V (String ("           InterFrameGapInNS: ") + String (InterFrameGapInNS));

    FrameDurationInNS += InterFrameGapInNS;
    // DEBUG_V (String ("       new FrameDurationInNS: ") + String (FrameDurationInNS));
    // DEBUG_V (String ("               MinFrameLenNS: ") + String (MinFrameLenNS));
    
    // uint32_t IdleLenNS = (MinFrameLenNS > FrameDurationInNS) ? MinFrameLenNS - FrameDurationInNS : 2 *NanoSecondsInAMicroSecond; 
    // DEBUG_V (String ("                   IdleLenNS: ") + String (IdleLenNS));
    
    FrameResetSliceCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (InterFrameGapInMicroSec * NanoSecondsInAMicroSecond);
    FrameResetSliceCount = (FrameResetSliceCount + (sizeof(c_OutputI2S::I2SDataAsUint_t) - 1)) & ~(sizeof(c_OutputI2S::I2SDataAsUint_t) - 1); // round up to the nearest multiple of the data type size
    // DEBUG_V (String ("        FrameResetSliceCount: ") + String (FrameResetSliceCount));

    // DEBUG_END;

} // CalculateFrameBits

//----------------------------------------------------------------------------
bool c_OutputUCS8903I2S::SetConfig (ArduinoJson::JsonObject& jsonConfig)
{
    // DEBUG_START;

    PauseOutput (true);

    bool response = c_OutputUCS8903::SetConfig (jsonConfig);

    // update the output GPIO
    I2Sdriver->SetGpio (OutputPortDefinition.PortId, OutputPortDefinition.gpios.data);

    if (OutputBufferSize)
    {
        // DEBUG_V ("start the transmiter");
        CalculateFrameBitSlices ();
        PauseOutput (false);
    }
    else
    {
        // DEBUG_V ("stop the transmiter");
        PauseOutput (true);
    }

    // DEBUG_END;
    return response;

} // SetConfig

//----------------------------------------------------------------------------
void c_OutputUCS8903I2S::SetOutputBufferSize (uint32_t NumChannelsToOutput)
{
    // DEBUG_START;

    // DEBUG_V (String ("NumChannelsToOutput: ") + String (NumChannelsToOutput));
    c_OutputUCS8903::SetOutputBufferSize (NumChannelsToOutput);

    if (OutputBufferSize)
    {
        // DEBUG_V ("start the transmiter");
        PauseOutput (true);
        CalculateFrameBitSlices ();
        PauseOutput (false);
    }
    else
    {
        // DEBUG_V ("stop the transmiter");
        PauseOutput (true);
    }

    // DEBUG_END;

} // SetBufferSize

//----------------------------------------------------------------------------
void c_OutputUCS8903I2S::GetStatus (ArduinoJson::JsonObject& jsonStatus)
{
    // // DEBUG_START;
    c_OutputUCS8903::GetStatus (jsonStatus);

    #ifdef USE_I2S_DEBUG_COUNTERS
    jsonStatus[F ("FrameDurationInMicroSec")] = FrameDurationInMicroSec;
    #endif // def USE_I2S_DEBUG_COUNTERS

    #ifdef UCS8903_I2S_DEBUG_COUNTERS
    JsonObject JsonCounters = jsonStatus["UCS8903_I2S_DEBUG_COUNTERS"].to<JsonObject> ();
    
    JsonWrite (JsonCounters, "GetDataSlicesToSendBaseCount",GetDataSlicesToSendBaseCount);
    JsonWrite (JsonCounters, "SetDefaultDataValueBaseCount", SetDefaultDataValueBaseCount);

    JsonWrite (JsonCounters, "StartNewFrameBaseCount",      StartNewFrameBaseCount);
    JsonWrite (JsonCounters, "GetDataSlicesI",              I2SDebugCounters.GetDataSlicesI);
    JsonWrite (JsonCounters, "GetDataSlicesII",             I2SDebugCounters.GetDataSlicesII);
    JsonWrite (JsonCounters, "GetDataSlicesIII",            I2SDebugCounters.GetDataSlicesIII);
    JsonWrite (JsonCounters, "FrameStarts",                 I2SDebugCounters.FrameStarts);
    JsonWrite (JsonCounters, "FrameEnds",                   I2SDebugCounters.FrameEnds);
    JsonWrite (JsonCounters, "FrameReset",                  I2SDebugCounters.FrameReset);
    JsonWrite (JsonCounters, "FrameResetI",                 I2SDebugCounters.FrameResetI);
    JsonWrite (JsonCounters, "FrameResetII",                I2SDebugCounters.FrameResetII);
    JsonWrite (JsonCounters, "FrameResetIII",               I2SDebugCounters.FrameResetIII);
    JsonWrite (JsonCounters, "FrameResetAlignments",        I2SDebugCounters.FrameResetAlignments);
    JsonWrite (JsonCounters, "FrameResetDone",              I2SDebugCounters.FrameResetDone);
    JsonWrite (JsonCounters, "IdleSliceCount",              I2SDebugCounters.IdleSliceCount);
    JsonWrite (JsonCounters, "DataI",                       I2SDebugCounters.DataI);
    JsonWrite (JsonCounters, "DataII",                      I2SDebugCounters.DataII);
    JsonWrite (JsonCounters, "DataIII",                     I2SDebugCounters.DataIII);
    JsonWrite (JsonCounters, "DataIV",                      I2SDebugCounters.DataIV);
    JsonWrite (JsonCounters, "DataByteCount",               I2SDebugCounters.DataByteCount);
    JsonWrite (JsonCounters, "UnKnownFrameState",           I2SDebugCounters.UnKnownFrameState);

    JsonWrite (JsonCounters, "DataBitMask",                 String (DataBitMask, HEX));
    JsonWrite (JsonCounters, "DataBit",                     String (DataBit, HEX));
    JsonWrite (JsonCounters, "FrameResetBitCount",          FrameResetSliceCount);
    JsonWrite (JsonCounters, "FrameResetRemainingBitCount", FrameResetRemainingSliceCount);
    JsonWrite (JsonCounters, "DataPattern",                 String (DataPattern, HEX));
    JsonWrite (JsonCounters, "DataPatternMask",             String (DataPatternMask, HEX));

    #endif // def UCS8903_I2S_DEBUG_COUNTERS

    // // DEBUG_END;
} // GetStatus

//----------------------------------------------------------------------------
void IRAM_ATTR c_OutputUCS8903I2S::ISR_StartNewDataFrame ()
{
    // DEBUG_START;

    c_OutputUCS8903::ISR_StartNewFrame ();

    // DEBUG_V (String ("frame started on ") + String (OutputPortDefinition.gpios.data));
    INC_UCS8903_I2S_DEBUG_COUNTER (FrameStarts);

    FrameResetRemainingSliceCount  = FrameResetSliceCount;
    OutputUCS8903I2S_FSM_State = c_OutputUCS8903I2S::OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_FrameReset;

    // DEBUG_END;
} // StartNewDataFrame

//----------------------------------------------------------------------------
void c_OutputUCS8903I2S::SetDefaultDataValue (c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    // DEBUG_START;

    // DEBUG_V (String ("     SetDefaultDataValue:0x") + String (DefaultIntensitySliceData, HEX));
    // DEBUG_V (String ("                 DataBit:0x") + String (DataBit, HEX));
    // DEBUG_V (String ("             DataBitMask:0x") + String (DataBitMask, HEX));

    OneBit.DataAsBytes[2] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[3] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[0] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[1] = DefaultIntensitySliceData & DataBitMask;

    ZeroBit.DataAsBytes[2] = DefaultIntensitySliceData | DataBit;
    ZeroBit.DataAsBytes[3] = DefaultIntensitySliceData & DataBitMask;
    ZeroBit.DataAsBytes[0] = DefaultIntensitySliceData & DataBitMask;
    ZeroBit.DataAsBytes[1] = DefaultIntensitySliceData & DataBitMask;

    // DEBUG_END;
} // SetDefaultDataValue

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputUCS8903I2S::ISR_GetNextDataSlicesToSend (c_OutputI2S::I2S_Item_t * pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    bool MoreDataToSend = true;
    INC_UCS8903_I2S_DEBUG_COUNTER (GetDataSlicesI);

    // NOTE: The data request is always uint32 aligned and is a multiple of uint32s long

    INC_UCS8903_I2S_DEBUG_COUNTER (GetDataSlicesII);

    switch (OutputUCS8903I2S_FSM_State)
    {
        case OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_FrameReset:
        {
            INC_UCS8903_I2S_DEBUG_COUNTER (FrameReset);
            
            // Send as many slices as we can to fill the output buffer with the default data value.  The output buffer is always aligned to a I2SDataAsUint_t boundary and the number of slices requested is always a multiple of the I2SDataAsUint_t size.  The FrameResetRemainingSliceCount is also a multiple of the I2SDataAsUint_t size, so we can fill the output buffer with a memset call.
            uint NumSlicesToSend = min (FrameResetRemainingSliceCount, numSlices);
            memset(pDataToSend, (DefaultIntensitySliceData & DataBitMask), NumSlicesToSend);
            numSlices -= NumSlicesToSend;
            pDataToSend += NumSlicesToSend;
            FrameResetRemainingSliceCount -= NumSlicesToSend;

            // do we need to send more slices?
            if(FrameResetRemainingSliceCount > 0)
            {
                // come back to finish in the next buffer
                INC_UCS8903_I2S_DEBUG_COUNTER (FrameResetI);
                break;
            } // end if(FrameResetRemainingSliceCount > 0)

            INC_UCS8903_I2S_DEBUG_COUNTER (FrameResetII);

            // align output to a bit boundary
            uint NumEntriesToSendToAlignOutputToUint = min (uint(pDataToSend) & (sizeof(c_OutputI2S::I2SDataAsUint_t)-1), numSlices);
            if(NumEntriesToSendToAlignOutputToUint)
            {
                INC_UCS8903_I2S_DEBUG_COUNTER (FrameResetAlignments);

                memset(pDataToSend, (DefaultIntensitySliceData & DataBitMask), NumEntriesToSendToAlignOutputToUint);
                numSlices -= NumEntriesToSendToAlignOutputToUint;
                pDataToSend += NumEntriesToSendToAlignOutputToUint;
            } // end if(NumEntriesToSendToAlignOutputToUint)

            // at this point, the output buffer pointer is aligned to a I2SDataAsUint_t boundary.

            INC_UCS8903_I2S_DEBUG_COUNTER (FrameResetDone);

            // fall through to the send data state to send the first data pattern
            OutputUCS8903I2S_FSM_State = OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Data;

        } // end case OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_FrameReset

        case OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Data:
        {
            INC_UCS8903_I2S_DEBUG_COUNTER (DataI);

            uint NumBitsToSend = numSlices / sizeof(c_OutputI2S::I2SDataAsUint_t);

            while (NumBitsToSend > 0)
            {
                INC_UCS8903_I2S_DEBUG_COUNTER (DataII);

                * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = (DataPattern & DataPatternMask) ? OneBit : ZeroBit;

                --NumBitsToSend;
                numSlices -= sizeof(c_OutputI2S::I2SDataAsUint_t);
                pDataToSend += sizeof(c_OutputI2S::I2SDataAsUint_t);

                DataPatternMask = DataPatternMask >> 1;

                // did we just send the last bit of the current data pattern?
                if (0 == DataPatternMask)
                {
                    INC_UCS8903_I2S_DEBUG_COUNTER (DataIII);

                    // try to get another data pattern to send
                    if(!ISR_SetUpNextDataToSend())
                    {
                        INC_UCS8903_I2S_DEBUG_COUNTER (DataIV);
                        // stop sending data bits
                        OutputUCS8903I2S_FSM_State = c_OutputUCS8903I2S::OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle;
                        MoreDataToSend = false;
                        break;
                    } // end if(!ISR_SetUpNextDataToSend())
                } // end new data pattern needed
            } // end while (NumBitsToSend > 0)

            // come back when we get a new buffer to fill
            break;

        } // end case OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Data

        case OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle:
        default:
        {
            // nothing to do. Just leave.
            INC_UCS8903_I2S_DEBUG_COUNTER (IdleSliceCount);
            OutputUCS8903I2S_FSM_State = c_OutputUCS8903I2S::OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle;
            MoreDataToSend = false;
            break;
        } // end default case
    } // switch (CurrentFsmState)

    INC_UCS8903_I2S_DEBUG_COUNTER (GetDataSlicesIII);

    return MoreDataToSend;
} // ISR_GetNextDataSlicesToSend

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputUCS8903I2S::ISR_SetUpNextDataToSend ()
{
    // 1 bit of data has completed move to the next bit of data
    bool MoreDataToSend = true;

    do // once
    {
        // do we need to set up the next data byte to send?

        // is there another data byte to send?
        if (false == c_OutputPixel::ISR_MoreDataToSend ())
        {
            // Frame is complete, set up for the next frame
            INC_UCS8903_I2S_DEBUG_COUNTER (FrameEnds);
            MoreDataToSend = false;

            OutputUCS8903I2S_FSM_State = OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle;
            break;
        }
    
        // there is more data to send in the current frame
        INC_UCS8903_I2S_DEBUG_COUNTER (DataByteCount);

        // set up to output the next data byte
        c_OutputPixel::ISR_GetNextIntensityToSend (DataPattern);
        // next bit is MSBit first, so set the mask to the MSB of the 16 bit data pattern
        DataPatternMask = 0x8000;

    } while (false); // do once

    return MoreDataToSend;
} // ISR_SetUpNextDataToSend

//----------------------------------------------------------------------------
void c_OutputUCS8903I2S::PauseOutput (bool State)
{
    // DEBUG_START;

    // DEBUG_V (String ("PortId: ") + String (OutputPortDefinition.PortId));
    // DEBUG_V (String (" New State: ") + String (State));
    // DEBUG_V (String (" Old State: ") + String (IsPaused ()));
    if (IsPaused ())
    {
        // DEBUG_V ("Currently Paused, Make sure we are ready to start at the beginning of the next frame");
        OutputUCS8903I2S_FSM_State = OutputUCS8903I2S_FSM_States::_OutputUCS8903I2S_FSM_State_Idle;
    }
    c_OutputUCS8903::PauseOutput (State);
    I2Sdriver->SetOutputState (OutputPortDefinition.PortId, !State);

    // DEBUG_END;
} // PauseOutput

#endif // defined (SUPPORT_OutputProtocol_UCS8903) && defined (SUPPORT_I2S)
