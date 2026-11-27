/*
* OutputSerialI2S.cpp - Serial protocol driver code for ESPixelStick I2S Channel
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2015 - 2026 Shelby Merrick
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
#if (defined (SUPPORT_OutputProtocol_FireGod) || defined (SUPPORT_OutputProtocol_DMX) || defined (SUPPORT_OutputProtocol_Serial) || defined (SUPPORT_OutputProtocol_Renard)) && defined (SUPPORT_I2S)

#include "output/OutputSerialI2S.hpp"

//----------------------------------------------------------------------------
#ifdef SERIAL_I2S_DEBUG_COUNTERS
static uint32_t StartNewFrameBaseCount = 0;
static uint32_t GetDataSlicesToSendBaseCount = 0;
static uint32_t SetDefaultDataValueBaseCount = 0;
#endif // def SERIAL_I2S_DEBUG_COUNTERS

//----------------------------------------------------------------------------
static void IRAM_ATTR StartNewFrameBase (void * arg)
{
    #ifdef SERIAL_I2S_DEBUG_COUNTERS
    StartNewFrameBaseCount++;
    #endif // def SERIAL_I2S_DEBUG_COUNTERS
    
    reinterpret_cast<c_OutputSerialI2S*> (arg)->ISR_StartNewDataFrame ();
} // StartNewFrameBase

//----------------------------------------------------------------------------
// return false if there are no more data slices to send
static bool IRAM_ATTR GetDataSlicesToSendBase (void * arg, c_OutputI2S::I2S_Item_t * DataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef SERIAL_I2S_DEBUG_COUNTERS
    ++GetDataSlicesToSendBaseCount;
    #endif // def SERIAL_I2S_DEBUG_COUNTERS

    return reinterpret_cast<c_OutputSerialI2S*> (arg)->ISR_GetNextDataSlicesToSend (DataToSend, numSlices, DefaultIntensitySliceData);
} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
static void SetDefaultDataValueBase (void * arg, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef SERIAL_I2S_DEBUG_COUNTERS
    ++SetDefaultDataValueBaseCount;
    #endif // def SERIAL_I2S_DEBUG_COUNTERS

    reinterpret_cast<c_OutputSerialI2S*> (arg)->SetDefaultDataValue (DefaultIntensitySliceData);
} // SetDefaultDataValueBase

//----------------------------------------------------------------------------
c_OutputSerialI2S::c_OutputSerialI2S (OM_OutputPortDefinition_t & OutputPortDefinition,
    c_OutputMgr::e_OutputProtocolType outputType) :
    c_OutputSerial (OutputPortDefinition, outputType)
{
    // DEBUG_START;

    CurrentBreakSliceCount = 0;
    CurrentMabSliceCount   = 0;
    CurrentBitSliceCount   = 0;

    #ifdef SERIAL_I2S_DEBUG_COUNTERS
    memset (&SerialI2SDebugCounters, 0, sizeof (SerialI2SDebugCounters));
    #endif // def SERIAL_I2S_DEBUG_COUNTERS

    // DEBUG_END;

} // c_OutputSerialI2S

//----------------------------------------------------------------------------
c_OutputSerialI2S::~c_OutputSerialI2S ()
{
    // DEBUG_START;
    if (HasBeenInitialized)
    {
        I2Sdriver->RemoveSlotDevice (OutputPortDefinition.PortId);
    }

    // DEBUG_END;
} // ~c_OutputSerialI2S

//----------------------------------------------------------------------------
/* Use the current config to set up the output port
*/
void c_OutputSerialI2S::Begin ()
{
    // DEBUG_START;

    I2Sdriver = OutputMgr.GetI2sDriver ();

    c_OutputSerial::Begin ();

    // make sure we are in the paused state
    PauseOutput (true);

    // DEBUG_V (String ("DataPin: ") + String (DataPin));

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
void c_OutputSerialI2S::CalculateFrameBitSlices ()
{
    // DEBUG_START;

    float BitTimeNS = (1.0 / float (CurrentBaudrate)) * NanoSecondsInASecond;
    // DEBUG_V (String (" CurrentBaudrate: ") + String (CurrentBaudrate));
    // DEBUG_V (String ("       BitTimeNS: ") + String (BitTimeNS));

    // DEBUG_V (String ("BitTimeNS: ") + String (BitTimeNS));
    // DEBUG_V (String ("I2S_TickLengthNS: ") + String (I2S_TickLengthNS));

    BreakSliceCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (DMX_BREAK_US * NanoSecondsInAMicroSecond);
    MabSliceCount   = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (DMX_MAB_US * NanoSecondsInAMicroSecond);
    BitSliceCount   = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (BitTimeNS);

    // DEBUG_END;

} // CalculateFrameBits

//----------------------------------------------------------------------------
bool c_OutputSerialI2S::SetConfig (ArduinoJson::JsonObject& jsonConfig)
{
    // DEBUG_START;

    bool response = c_OutputSerial::SetConfig (jsonConfig);
    // DEBUG_V (String ("DataPin: ") + String (DataPin));

    // update the output GPIO
    I2Sdriver->SetGpio (OutputPortDefinition.PortId, OutputPortDefinition.gpios.data);

    // DEBUG_END;
    return response;

} // SetConfig

//----------------------------------------------------------------------------
void c_OutputSerialI2S::SetOutputBufferSize (uint32_t NumChannelsAvailable)
{
    // DEBUG_START;

    c_OutputSerial::SetOutputBufferSize (NumChannelsAvailable);

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
void c_OutputSerialI2S::GetStatus (ArduinoJson::JsonObject& jsonStatus)
{
    // DEBUG_START;

    c_OutputSerial::GetStatus (jsonStatus);

    #ifdef SERIAL_I2S_DEBUG_COUNTERS
    JsonObject JsonCounters = jsonStatus["JsonCounters"].to<JsonObject> ();

    JsonWrite (JsonCounters, "StartNewFrameBaseCount",       StartNewFrameBaseCount);
    JsonWrite (JsonCounters, "GetDataSlicesToSendBaseCount", GetDataSlicesToSendBaseCount);
    JsonWrite (JsonCounters, "SetDefaultDataValueBaseCount", SetDefaultDataValueBaseCount);

    JsonWrite (JsonCounters, "StartNewDataFrame",            SerialI2SDebugCounters.StartNewDataFrame);
    JsonWrite (JsonCounters, "StartNewDataFrameI",           SerialI2SDebugCounters.StartNewDataFrameI);
    JsonWrite (JsonCounters, "StartNewDataFrameII",          SerialI2SDebugCounters.StartNewDataFrameII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSend",      SerialI2SDebugCounters.GetNextDataSlicesToSend);
    JsonWrite (JsonCounters, "BreakBits",                    SerialI2SDebugCounters.BreakBits);
    JsonWrite (JsonCounters, "MabBits",                      SerialI2SDebugCounters.MabBits);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendI",     SerialI2SDebugCounters.GetNextDataSlicesToSendI);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendII",    SerialI2SDebugCounters.GetNextDataSlicesToSendII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendIII",   SerialI2SDebugCounters.GetNextDataSlicesToSendIII);
    JsonWrite (JsonCounters, "SetUpNextDataToSend",          SerialI2SDebugCounters.SetUpNextDataToSend);
    JsonWrite (JsonCounters, "SetUpNextDataToSendI",         SerialI2SDebugCounters.SetUpNextDataToSendI);
    JsonWrite (JsonCounters, "SetUpNextDataToSendII",        SerialI2SDebugCounters.SetUpNextDataToSendII);

    #endif // def SERIAL_I2S_DEBUG_COUNTERS

    // DEBUG_END;
} // GetStatus

//----------------------------------------------------------------------------
void IRAM_ATTR c_OutputSerialI2S::ISR_StartNewDataFrame ()
{
    // DEBUG_START;
    // DEBUG_V (String ("frame started on ") + String (OutputPortDefinition.gpios.data));
    INC_SERIAL_I2S_DEBUG_COUNTERS (StartNewDataFrame);

    c_OutputSerial::ISR_StartNewFrame ();

    #if defined (SUPPORT_OutputProtocol_DMX)
    if (OutputType == c_OutputMgr::e_OutputProtocolType::OutputProtocol_DMX)
    {
        INC_SERIAL_I2S_DEBUG_COUNTERS (StartNewDataFrameI);
        CurrentBreakSliceCount = BreakSliceCount;
        CurrentMabSliceCount   = MabSliceCount;
    }
    else
    {
        INC_SERIAL_I2S_DEBUG_COUNTERS (StartNewDataFrameII);
        CurrentBreakSliceCount = 0;
        CurrentMabSliceCount   = 0;
    }
    #endif //  defined (SUPPORT_OutputProtocol_DMX)

    ISR_SetUpNextDataToSend ();

    // DEBUG_END;
} // StartNewDataFrame

//----------------------------------------------------------------------------
void c_OutputSerialI2S::SetDefaultDataValue (c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    // DEBUG_START;

    // DEBUG_V (String ("     SetDefaultDataValue:0x") + String (DefaultIntensitySliceData, HEX));

    OneBit = DefaultIntensitySliceData | DataBit;
    ZeroBit = DefaultIntensitySliceData & DataBitMask;

    // DEBUG_END;
} // SetDefaultDataValue

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputSerialI2S::ISR_GetNextDataSlicesToSend (c_OutputI2S::I2S_Item_t * pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    INC_SERIAL_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSend);

    bool MoreDataToSend = true;

    #if defined (SUPPORT_OutputProtocol_DMX)
    while (CurrentBreakSliceCount && numSlices)
    {
        INC_SERIAL_I2S_DEBUG_COUNTERS (BreakBits);
        * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend)) ^ 0x2)) = ZeroBit;
        --CurrentBreakSliceCount;
        numSlices -= sizeof (ZeroBit);
        pDataToSend += sizeof (ZeroBit);

    } // while (CurrentBreakSliceCount && numSlices)

    while (CurrentMabSliceCount && numSlices)
    {
        INC_SERIAL_I2S_DEBUG_COUNTERS (MabBits);

        * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend)) ^ 0x2)) = OneBit;
        --CurrentMabSliceCount;
        numSlices -= sizeof (OneBit);
        pDataToSend += sizeof (OneBit);

    } // while (CurrentMabSliceCount && numSlices)
    #endif // defined (SUPPORT_OutputProtocol_DMX)

    // send data bytes.
    while (numSlices && MoreDataToSend && CurrentBitSliceCount)
    {
        INC_SERIAL_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendI);

        * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend)) ^ 0x2)) = (DataPattern & DataPatternMask) ? OneBit : ZeroBit;
        numSlices -= sizeof (OneBit);
        pDataToSend += sizeof (OneBit);
        --CurrentBitSliceCount;

        // have more slices in the current data bit to send?
        if (0 == CurrentBitSliceCount)
        {
            INC_SERIAL_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendII);

            // move to the next bit of data
            DataPatternMask = (DataPatternMask << 1) & 0b11111111111;
            CurrentBitSliceCount = BitSliceCount;

            if (0 == DataPatternMask)
            {
                INC_SERIAL_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendIII);
                MoreDataToSend = ISR_SetUpNextDataToSend ();
            }
        }
    } // while (numSlices && MoreDataToSend && CurrentBitSliceCount)

    return MoreDataToSend;
} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputSerialI2S::ISR_SetUpNextDataToSend ()
{
    // 1 bit of data has completed move to the next bit of data
    bool MoreDataToSend = true;
    INC_SERIAL_I2S_DEBUG_COUNTERS (SetUpNextDataToSend);

    do // once
    {
        // do we need to set up the next data byte to send?

        // is there another data byte to send?
        if (false == c_OutputSerial::ISR_MoreDataToSend ())
        {
            // Frame is complete, set up for the next frame
            INC_SERIAL_I2S_DEBUG_COUNTERS (SetUpNextDataToSendI);

            MoreDataToSend = false;
            DataPatternMask = 0;
            break;
        }
    
        // there is more data to send in the current frame
        INC_SERIAL_I2S_DEBUG_COUNTERS (SetUpNextDataToSendII);

        // set up to output the next data byte
        DataPattern = 0;
        c_OutputSerial::ISR_GetNextIntensityToSend (DataPattern);

        // add start and stop bits to the data byte
        DataPattern = ((DataPattern << 1) & ~0x01) | 0b11000000000; // add start and stop bits to the data byte
        DataPatternMask = 0x1; // send 11 bits: 1 Start + 8 data + 2 stop bits
        CurrentBitSliceCount = BitSliceCount;
    
    } while (false); // do once

    return MoreDataToSend;
} // ISR_SetUpNextDataToSend

//----------------------------------------------------------------------------
void c_OutputSerialI2S::PauseOutput (bool State)
{
    // DEBUG_START;

    // DEBUG_V (String ("PortId: ") + String (OutputPortDefinition.PortId));
    // DEBUG_V (String (" New State: ") + String (State));
    // DEBUG_V (String (" Old State: ") + String (IsPaused ()));
    c_OutputSerial::PauseOutput (State);
    I2Sdriver->SetOutputState (OutputPortDefinition.PortId, !State);

    // DEBUG_END;
} // PauseOutput

#endif // (defined (SUPPORT_OutputProtocol_FireGod) || defined (SUPPORT_OutputProtocol_DMX) || defined (SUPPORT_OutputProtocol_Serial) || defined (SUPPORT_OutputProtocol_Renard)) && defined (SUPPORT_I2S)
