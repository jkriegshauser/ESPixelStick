/*
* OutputWS2811I2S.cpp - WS2811 driver code for ESPixelStick I2S Channel
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
#if defined (SUPPORT_OutputProtocol_WS2811) && defined (SUPPORT_I2S)

#include "output/OutputWS2811I2S.hpp"
#include "output/OutputMgr.hpp"

//----------------------------------------------------------------------------
#ifdef WS2811_I2S_DEBUG_COUNTERS
uint32_t StartNewFrameBaseCount = 0;
#endif // def WS2811_I2S_DEBUG_COUNTERS
// return false if there are no more data slices to send
static void IRAM_ATTR StartNewFrameBase (void * arg)
{
    #ifdef WS2811_I2S_DEBUG_COUNTERS
    StartNewFrameBaseCount++;
    #endif // def WS2811_I2S_DEBUG_COUNTERS
    
    reinterpret_cast<c_OutputWS2811I2S*> (arg)->ISR_StartNewDataFrame ();
} // StartNewFrameBase

//----------------------------------------------------------------------------
#ifdef WS2811_I2S_DEBUG_COUNTERS
uint32_t GetDataSlicesToSendBaseCount = 0;
#endif // def WS2811_I2S_DEBUG_COUNTERS

//----------------------------------------------------------------------------
// return false if there are no more data slices to send
static bool IRAM_ATTR GetDataSlicesToSendBase (void * arg, c_OutputI2S::I2S_Item_t * DataToSend, uint32_t numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef WS2811_I2S_DEBUG_COUNTERS
    ++GetDataSlicesToSendBaseCount;
    #endif // def WS2811_I2S_DEBUG_COUNTERS

    return reinterpret_cast<c_OutputWS2811I2S*> (arg)->ISR_GetNextDataSlicesToSend (DataToSend, numSlices, DefaultIntensitySliceData);
} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
c_OutputWS2811I2S::c_OutputWS2811I2S (OM_OutputPortDefinition_t & OutputPortDefinition,
                                      c_OutputMgr::e_OutputProtocolType outputType) :
    c_OutputWS2811 (OutputPortDefinition, outputType)
{
    // DEBUG_START;

    // DEBUG_V (String ("WS2811_PIXEL_I2S_TICKS_BIT_0_H: 0x") + String (WS2811_PIXEL_I2S_TICKS_BIT_0_HIGH, HEX));
    // DEBUG_V (String ("WS2811_PIXEL_I2S_TICKS_BIT_0_L: 0x") + String (WS2811_PIXEL_I2S_TICKS_BIT_0_LOW,  HEX));
    // DEBUG_V (String ("WS2811_PIXEL_I2S_TICKS_BIT_1_H: 0x") + String (WS2811_PIXEL_I2S_TICKS_BIT_1_HIGH, HEX));
    // DEBUG_V (String ("WS2811_PIXEL_I2S_TICKS_BIT_1_L: 0x") + String (WS2811_PIXEL_I2S_TICKS_BIT_1_LOW,  HEX));

    OutputWS2811I2S_FSM_State = OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Idle;

    // DEBUG_END;

} // c_OutputWS2811I2S

//----------------------------------------------------------------------------
c_OutputWS2811I2S::~c_OutputWS2811I2S ()
{
    // DEBUG_START;
    if (HasBeenInitialized)
    {
        I2Sdriver->RemoveSlotDevice (OutputPortDefinition.PortId);
    }

    // DEBUG_END;
} // ~c_OutputWS2811I2S

//----------------------------------------------------------------------------
void c_OutputWS2811I2S::Begin ()
{
    // DEBUG_START;

    I2Sdriver = OutputMgr.GetI2sDriver ();

    c_OutputWS2811::Begin ();

    // make sure we are in the paused state
    PauseOutput (true);

    // DEBUG_V (String ("DataPin: ") + String (DataPin));

    FrameResetWordCount        = 0;
    FrameResetRemainingWordCount = 0;

    c_OutputI2S::OutputI2SChannelConfig_t OutputI2SConfig;
    OutputI2SConfig.I2SChannelId              = uint32_t (OutputPortDefinition.PortId);
    OutputI2SConfig.DataPin                   = gpio_num_t (OutputPortDefinition.gpios.data);
    OutputI2SConfig.arg                       = this;
    OutputI2SConfig.StartNewFrame             = StartNewFrameBase;
    OutputI2SConfig.GetNextIntensityBitSlices = GetDataSlicesToSendBase;
    OutputI2SConfig.IdleLevelIsHigh           = true; // idle is low
    OutputI2SConfig.IsActive                  = false; // do not output data

    I2Sdriver->RegisterSlotDevice (OutputI2SConfig, &DataBit, &DataBitMask);

    // DEBUG_V (String ("    DataBit: 0x") + String (DataBit, HEX));
    // DEBUG_V (String ("DataBitMask: 0x") + String (DataBitMask, HEX));

    HasBeenInitialized = true;

    // DEBUG_END;

} // Begin

//----------------------------------------------------------------------------
void c_OutputWS2811I2S::CalculateFrameBitSlices ()
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
    
    uint32_t IdleLenNS = (MinFrameLenNS > FrameDurationInNS) ? MinFrameLenNS - FrameDurationInNS : 2 *NanoSecondsInAMicroSecond; 
    // DEBUG_V (String ("                   IdleLenNS: ") + String (IdleLenNS));
    
    FrameResetWordCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (InterFrameGapInMicroSec * NanoSecondsInAMicroSecond);
    FrameResetWordCount = (FrameResetWordCount + 3) & ~3; // round up to the nearest multiple of 4
    // DEBUG_V (String ("        FrameResetSliceCount: ") + String (FrameResetSliceCount));

    // DEBUG_END;

} // CalculateFrameBits

//----------------------------------------------------------------------------
bool c_OutputWS2811I2S::SetConfig (ArduinoJson::JsonObject& jsonConfig)
{
    // DEBUG_START;

    PauseOutput (true);

    bool response = c_OutputWS2811::SetConfig (jsonConfig);

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
void c_OutputWS2811I2S::SetOutputBufferSize (uint32_t NumChannelsToOutput)
{
    // DEBUG_START;

    // DEBUG_V (String ("NumChannelsToOutput: ") + String (NumChannelsToOutput));
    c_OutputWS2811::SetOutputBufferSize (NumChannelsToOutput);

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
void c_OutputWS2811I2S::GetStatus (ArduinoJson::JsonObject& jsonStatus)
{
    // // DEBUG_START;
    c_OutputWS2811::GetStatus (jsonStatus);

    #ifdef USE_I2S_DEBUG_COUNTERS
    jsonStatus[F ("FrameDurationInMicroSec")] = FrameDurationInMicroSec;
    #endif // def USE_I2S_DEBUG_COUNTERS

    #ifdef WS2811_I2S_DEBUG_COUNTERS
    JsonObject JsonCounters = jsonStatus["WS2811_I2S_DEBUG_COUNTERS"].to<JsonObject> ();
    
    JsonWrite (JsonCounters, "GetDataSlicesToSendBaseCount",GetDataSlicesToSendBaseCount);
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
    JsonWrite (JsonCounters, "FrameResetWords",             I2SDebugCounters.FrameResetWords);
    JsonWrite (JsonCounters, "FrameResetDone",              I2SDebugCounters.FrameResetDone);
    JsonWrite (JsonCounters, "IdleSliceCount",              I2SDebugCounters.IdleSliceCount);
    JsonWrite (JsonCounters, "DataI",                       I2SDebugCounters.DataI);
    JsonWrite (JsonCounters, "DataII",                      I2SDebugCounters.DataII);
    JsonWrite (JsonCounters, "DataIII",                     I2SDebugCounters.DataIII);
    JsonWrite (JsonCounters, "DataByteCount",               I2SDebugCounters.DataByteCount);
    JsonWrite (JsonCounters, "UnKnownFrameState",           I2SDebugCounters.UnKnownFrameState);

    JsonWrite (JsonCounters, "DataBitMask",                 String (DataBitMask, HEX));
    JsonWrite (JsonCounters, "DataBit",                     String (DataBit, HEX));
    JsonWrite (JsonCounters, "FrameResetWordCount",         FrameResetWordCount);
    JsonWrite (JsonCounters, "FrameResetCurrentWordCount",  FrameResetRemainingWordCount);
    JsonWrite (JsonCounters, "DataPattern",                 String (DataPattern, HEX));
    JsonWrite (JsonCounters, "DataPatternMask",             String (DataPatternMask, HEX));

    #endif // def WS2811_I2S_DEBUG_COUNTERS

    // // DEBUG_END;
} // GetStatus

//----------------------------------------------------------------------------
void IRAM_ATTR c_OutputWS2811I2S::ISR_StartNewDataFrame ()
{
    // DEBUG_START;

    c_OutputWS2811::ISR_StartNewFrame ();

    // DEBUG_V (String ("frame started on ") + String (OutputPortDefinition.gpios.data));
    INC_WS2811_I2S_DEBUG_COUNTER (FrameStarts);

    FrameResetRemainingWordCount  = FrameResetWordCount;
    OutputWS2811I2S_FSM_State = c_OutputWS2811I2S::OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_FrameReset;

    // DEBUG_END;
} // StartNewDataFrame

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputWS2811I2S::ISR_GetNextDataSlicesToSend (c_OutputI2S::I2S_Item_t * pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    bool MoreDataToSend = true;
    INC_WS2811_I2S_DEBUG_COUNTER (GetDataSlicesI);

    // NOTE: The data request is always uint32 aligned and is a multiple of uint32s long

    // Place to build the next data slices to send
    memset(&ResetBit, (DefaultIntensitySliceData & DataBitMask), sizeof(ResetBit));

    OneBit.DataAsBytes[2] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[3] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[0] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[1] = DefaultIntensitySliceData & DataBitMask;

    ZeroBit.DataAsBytes[2] = DefaultIntensitySliceData | DataBit;
    ZeroBit.DataAsBytes[3] = DefaultIntensitySliceData & DataBitMask;
    ZeroBit.DataAsBytes[0] = DefaultIntensitySliceData & DataBitMask;
    ZeroBit.DataAsBytes[1] = DefaultIntensitySliceData & DataBitMask;

    while (numSlices > sizeof(uint32_t) - 1)
    {
        INC_WS2811_I2S_DEBUG_COUNTER (GetDataSlicesII);

        switch (OutputWS2811I2S_FSM_State)
        {
            case OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_FrameReset:
            {
                INC_WS2811_I2S_DEBUG_COUNTER (FrameReset);
                
                // align output to a word boundary
                uint NumEntriesToSendToAlignOutput = min (uint(pDataToSend) & 0x02, numSlices);
                while(NumEntriesToSendToAlignOutput > 0)
                {
                    INC_WS2811_I2S_DEBUG_COUNTER (FrameResetAlignments);

                    * pDataToSend = ResetBit.DataAsBytes[0];
                    --numSlices;
                    ++pDataToSend;
                    --NumEntriesToSendToAlignOutput;
                }
                INC_WS2811_I2S_DEBUG_COUNTER (FrameResetI);

                // at this point, the output buffer pointer is aligned to a uint32 boundary

                // determine the number of words to send for the frame reset
                uint NumWordsToSend = min(FrameResetRemainingWordCount, numSlices / sizeof(uint32_t));
                while(NumWordsToSend > 0)
                {
                    INC_WS2811_I2S_DEBUG_COUNTER (FrameResetWords);

                    // write the data to the buffer
                    * ((uint32_t*) (pDataToSend)) = ResetBit.AsUint32;

                    // accounting
                    numSlices -= sizeof(ResetBit.AsUint32);
                    pDataToSend += sizeof(ResetBit.AsUint32);
                    --NumWordsToSend;
                    FrameResetRemainingWordCount -= sizeof(ResetBit.AsUint32);
                }

                INC_WS2811_I2S_DEBUG_COUNTER (FrameResetII);

                // did the counter wrap?
                if (FrameResetRemainingWordCount > FrameResetWordCount)
                {
                    INC_WS2811_I2S_DEBUG_COUNTER (FrameResetDone);

                    ISR_SetUpNextDataToSend ();
                    OutputWS2811I2S_FSM_State = c_OutputWS2811I2S::OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Data;
                }

                INC_WS2811_I2S_DEBUG_COUNTER (FrameResetIII);

                break;
            }

            case OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Data:
            {
                INC_WS2811_I2S_DEBUG_COUNTER (DataI);

                * ((uint32_t*) (pDataToSend)) = (DataPattern & DataBitMask) ? OneBit.AsUint32 : ZeroBit.AsUint32;

                numSlices -= sizeof(OneBit.AsUint32);
                pDataToSend += sizeof(OneBit.AsUint32);

                DataPatternMask = DataPatternMask >> 1;
                if (0 == DataPatternMask)
                {
                    INC_WS2811_I2S_DEBUG_COUNTER (DataII);

                    if(!ISR_SetUpNextDataToSend())
                    {
                        INC_WS2811_I2S_DEBUG_COUNTER (DataIII);
                        OutputWS2811I2S_FSM_State = c_OutputWS2811I2S::OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Idle;
                    }
                }

                break;
            }

            case OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Idle:
            {
                INC_WS2811_I2S_DEBUG_COUNTER (IdleSliceCount);
                MoreDataToSend = false;
                // byte order in the data buffer is 2 3 0 1
                * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend)) ^ 0x2)) = DefaultIntensitySliceData;
        
                --numSlices;
                ++pDataToSend;
                break;
            }

            default:
            {
                OutputWS2811I2S_FSM_State = c_OutputWS2811I2S::OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Idle;

                INC_WS2811_I2S_DEBUG_COUNTER (UnKnownFrameState);

                // byte order in the data buffer is 2 3 0 1
                * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend)) ^ 0x2)) = DefaultIntensitySliceData;
        
                --numSlices;
                ++pDataToSend;
        
                break;
            }
        } // switch (CurrentFsmState)

        INC_WS2811_I2S_DEBUG_COUNTER (GetDataSlicesIII);

    } // while (numSlices > 0)

    return MoreDataToSend;
} // ISR_GetNextDataSlicesToSend

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputWS2811I2S::ISR_SetUpNextDataToSend ()
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
            INC_WS2811_I2S_DEBUG_COUNTER (FrameEnds);
            MoreDataToSend = false;

            OutputWS2811I2S_FSM_State = OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Idle;
            break;
        }
    
        // there is more data to send in the current frame
        INC_WS2811_I2S_DEBUG_COUNTER (DataByteCount);

        // set up to output the next data byte
        c_OutputPixel::ISR_GetNextIntensityToSend (DataPattern);
        // next bit is MSBit
        DataPatternMask = 0x80;

    } while (false); // do once

    return MoreDataToSend;
} // ISR_SetUpNextDataToSend

//----------------------------------------------------------------------------
void c_OutputWS2811I2S::PauseOutput (bool State)
{
    // DEBUG_START;

    // DEBUG_V (String ("PortId: ") + String (OutputPortDefinition.PortId));
    // DEBUG_V (String (" New State: ") + String (State));
    // DEBUG_V (String (" Old State: ") + String (IsPaused ()));
    if (IsPaused ())
    {
        // DEBUG_V ("Currently Paused, Make sure we are ready to start at the beginning of the next frame");
        OutputWS2811I2S_FSM_State = OutputWS2811I2S_FSM_States::_OutputWS2811I2S_FSM_State_Idle;
    }
    c_OutputWS2811::PauseOutput (State);
    I2Sdriver->SetOutputState (OutputPortDefinition.PortId, !State);

    // DEBUG_END;
} // PauseOutput

#endif // defined (SUPPORT_OutputProtocol_WS2811) && defined (SUPPORT_I2S)
