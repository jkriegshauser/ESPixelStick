/*
* OutputGECEI2S.cpp - GECE driver code for ESPixelStick I2S Channel
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
#if defined (SUPPORT_OutputProtocol_GECE) && defined (SUPPORT_I2S) 

#include "output/OutputGECEI2S.hpp"

#ifdef GECE_I2S_DEBUG_COUNTERS
static uint32_t StartNewFrameBaseCount = 0;
static uint32_t GetDataSlicesToSendBaseCount = 0;
static uint32_t SetDefaultDataValueBaseCount = 0;
#endif // def GECE_I2S_DEBUG_COUNTERS

//----------------------------------------------------------------------------
static void IRAM_ATTR StartNewFrameBase (void * arg) 
{
    #ifdef GECE_I2S_DEBUG_COUNTERS
    StartNewFrameBaseCount++;
    #endif // def GECE_I2S_DEBUG_COUNTERS

    reinterpret_cast<c_OutputGECEI2S*> (arg) ->ISR_StartNewDataFrame ();
} // StartNewFrameBase

//----------------------------------------------------------------------------
// return false if there are no more data slices to send
static bool IRAM_ATTR GetDataSlicesToSendBase (void * arg, c_OutputI2S::I2S_Item_t * DataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData) 
{
    #ifdef GECE_I2S_DEBUG_COUNTERS
    ++GetDataSlicesToSendBaseCount;
    #endif // def GECE_I2S_DEBUG_COUNTERS

    return reinterpret_cast<c_OutputGECEI2S*> (arg) ->ISR_GetNextDataSlicesToSend (DataToSend, numSlices, DefaultIntensitySliceData);
} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
static void SetDefaultDataValueBase (void * arg, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData) 
{
    #ifdef GECE_I2S_DEBUG_COUNTERS
    ++SetDefaultDataValueBaseCount;
    #endif // def GECE_I2S_DEBUG_COUNTERS

    reinterpret_cast<c_OutputGECEI2S*> (arg) ->SetDefaultDataValue (DefaultIntensitySliceData);

} // SetDefaultDataValueBase

//----------------------------------------------------------------------------
c_OutputGECEI2S::c_OutputGECEI2S (OM_OutputPortDefinition_t & OutputPortDefinition,
                                 c_OutputMgr::e_OutputProtocolType outputType) :
    c_OutputGECE (OutputPortDefinition, outputType) 
{
    // DEBUG_START;

    I2Sdriver = OutputMgr.GetI2sDriver ();
    NeedToEnumeratePixels = true;
    StartupSliceDelay = 1024 * 2000;

    #ifdef GECE_I2S_DEBUG_COUNTERS
    memset (&I2SDebugCounters, 0, sizeof (I2SDebugCounters) );
    #endif // def GECE_I2S_DEBUG_COUNTERS

    // DEBUG_END;

} // c_OutputGECEI2S

//----------------------------------------------------------------------------
c_OutputGECEI2S::~c_OutputGECEI2S () 
{
    // DEBUG_START;

    if (I2Sdriver) 
    {
        I2Sdriver->RemoveSlotDevice (OutputPortDefinition.PortId);
    }

    // DEBUG_END;
} // ~c_OutputGECEI2S

//----------------------------------------------------------------------------
void c_OutputGECEI2S::Begin () 
{
    // DEBUG_START;

    // make sure we are in the paused state
    PauseOutput (true);

    c_OutputGECE::Begin ();

    // DEBUG_V (String ("DataPin: ") + String (gpio_num_t (OutputPortDefinition.gpios.data) ) );

    c_OutputI2S::OutputI2SChannelConfig_t OutputI2SConfig;
    OutputI2SConfig.I2SChannelId              = uint32_t (OutputPortDefinition.PortId);
    OutputI2SConfig.DataPin                   = gpio_num_t (OutputPortDefinition.gpios.data);
    OutputI2SConfig.arg                       = this;
    OutputI2SConfig.StartNewFrame             = StartNewFrameBase;
    OutputI2SConfig.GetNextIntensityBitSlices = GetDataSlicesToSendBase;
    OutputI2SConfig.SetDefaultDataValue       = SetDefaultDataValueBase;
    OutputI2SConfig.IdleLevelIsHigh           = false; // idle is low
    OutputI2SConfig.IsEnabled                  = false; // do not output data

    I2Sdriver->RegisterSlotDevice (OutputI2SConfig, DataBit, DataBitMask);

    // DEBUG_V (String ("    DataBit: 0x") + String (DataBit, HEX) );
    // DEBUG_V (String ("DataBitMask: 0x") + String (DataBitMask, HEX) );

    c_OutputPixel::ClearGecePixelId ();

    HasBeenInitialized = true;

    // DEBUG_END;

} // Begin

//----------------------------------------------------------------------------
void c_OutputGECEI2S::CalculateFrameBitSlices () 
{
    // DEBUG_START;

    StartBit.first.DataBitDataValueCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (40 * NanoSecondsInAMicroSecond) - 1;
    StartBit.second.DataBitDataValueCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (10 * NanoSecondsInAMicroSecond) - 1;
    StartBit.first.CurrentCount = 0;
    StartBit.second.CurrentCount = 0;

    ZeroBit.first.DataBitDataValueCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (10 * NanoSecondsInAMicroSecond) - 1;
    ZeroBit.second.DataBitDataValueCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (20 * NanoSecondsInAMicroSecond) - 1;
    ZeroBit.first.CurrentCount = 0;
    ZeroBit.second.CurrentCount = 0;

    OneBit.first.DataBitDataValueCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (20 * NanoSecondsInAMicroSecond) - 1;
    OneBit.second.DataBitDataValueCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (10 * NanoSecondsInAMicroSecond) - 1;
    OneBit.first.CurrentCount = 0;
    OneBit.second.CurrentCount = 0;

    // DEBUG_END;

} // CalculateFrameBits

//----------------------------------------------------------------------------
bool c_OutputGECEI2S::SetConfig (ArduinoJson::JsonObject& jsonConfig) 
{
    // DEBUG_START;

    bool response = c_OutputGECE::SetConfig (jsonConfig);

    I2Sdriver->SetGpio (OutputPortDefinition.PortId, OutputPortDefinition.gpios.data);
    // DEBUG_V (String ("DataPin: ") + String (gpio_num_t (OutputPortDefinition.gpios.data) ) );

    // DEBUG_END;
    return response;

} // SetConfig

//----------------------------------------------------------------------------
void c_OutputGECEI2S::SetOutputBufferSize (uint32_t NumChannelsAvailable) 
{
    // DEBUG_START;

    c_OutputGECE::SetOutputBufferSize (NumChannelsAvailable);

    if (OutputBufferSize && HasBeenInitialized) 
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
void c_OutputGECEI2S::GetStatus (ArduinoJson::JsonObject& jsonStatus) 
{
    c_OutputGECE::GetStatus (jsonStatus);
    I2Sdriver->GetStatus (jsonStatus);

    #ifdef GECE_I2S_DEBUG_COUNTERS
    JsonObject JsonCounters = jsonStatus["JsonCounters"].to<JsonObject> ();

    JsonWrite (JsonCounters, "StartNewFrameBaseCount",       StartNewFrameBaseCount);
    JsonWrite (JsonCounters, "GetDataSlicesToSendBaseCount", GetDataSlicesToSendBaseCount);
    JsonWrite (JsonCounters, "SetDefaultDataValueBaseCount", SetDefaultDataValueBaseCount);

    JsonWrite (JsonCounters, "StartNewDataFrame",            I2SDebugCounters.StartNewDataFrame);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSend",      I2SDebugCounters.GetNextDataSlicesToSend);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendNumSlices", I2SDebugCounters.GetNextDataSlicesToSendNumSlices);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendI",     I2SDebugCounters.GetNextDataSlicesToSendI);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendII",    I2SDebugCounters.GetNextDataSlicesToSendII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendIII",   I2SDebugCounters.GetNextDataSlicesToSendIII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendIV",    I2SDebugCounters.GetNextDataSlicesToSendIV);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendV",     I2SDebugCounters.GetNextDataSlicesToSendV);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendVI",    I2SDebugCounters.GetNextDataSlicesToSendVI);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendVII",   I2SDebugCounters.GetNextDataSlicesToSendVII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendVIII",  I2SDebugCounters.GetNextDataSlicesToSendVIII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendIX",    I2SDebugCounters.GetNextDataSlicesToSendIX);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendX",     I2SDebugCounters.GetNextDataSlicesToSendX);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendXI",    I2SDebugCounters.GetNextDataSlicesToSendXI);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendXII",   I2SDebugCounters.GetNextDataSlicesToSendXII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendXIII",  I2SDebugCounters.GetNextDataSlicesToSendXIII);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendXIV",   I2SDebugCounters.GetNextDataSlicesToSendXIV);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendXV",    I2SDebugCounters.GetNextDataSlicesToSendXV);
    JsonWrite (JsonCounters, "GetNextDataSlicesToSendXVI",   I2SDebugCounters.GetNextDataSlicesToSendXVI);
    JsonWrite (JsonCounters, "SetUpNextDataToSend",          I2SDebugCounters.SetUpNextDataToSend);
    JsonWrite (JsonCounters, "SetUpNextDataToSendI",         I2SDebugCounters.SetUpNextDataToSendI);
    JsonWrite (JsonCounters, "SetUpNextDataToSendII",        I2SDebugCounters.SetUpNextDataToSendII);
    JsonWrite (JsonCounters, "SetUpNextDataToSendIII",       I2SDebugCounters.SetUpNextDataToSendIII);
    JsonWrite (JsonCounters, "SetUpNextDataToSendIV",        I2SDebugCounters.SetUpNextDataToSendIV);
    JsonWrite (JsonCounters, "SetUpNextDataToSendV",         I2SDebugCounters.SetUpNextDataToSendV);
    JsonWrite (JsonCounters, "SetUpNextDataToSendVI",        I2SDebugCounters.SetUpNextDataToSendVI);
    JsonWrite (JsonCounters, "SetUpNextDataToSendVII",       I2SDebugCounters.SetUpNextDataToSendVII);

    #endif // def GECE_I2S_DEBUG_COUNTERS

} // GetStatus

//----------------------------------------------------------------------------
void IRAM_ATTR c_OutputGECEI2S::ISR_StartNewDataFrame () 
{
    INC_GECE_I2S_DEBUG_COUNTERS (StartNewDataFrame);

    c_OutputGECE::ISR_StartNewFrame ();
    ISR_SetUpNextDataToSend ();

} // StartNewDataFrame

//----------------------------------------------------------------------------
void c_OutputGECEI2S::SetDefaultDataValue (c_OutputI2S::I2S_Item_t DefaultIntensitySliceData) 
{
    // DEBUG_START;

    // DEBUG_V (String ("     SetDefaultDataValue:0x") + String (DefaultIntensitySliceData, HEX) );
    // DEBUG_V (String ("                 DataBit:0x") + String (DataBit, HEX) );
    // DEBUG_V (String ("             DataBitMask:0x") + String (DataBitMask, HEX) );

    StartBit.first.DataValue = DefaultIntensitySliceData & DataBitMask;
    StartBit.second.DataValue = DefaultIntensitySliceData | DataBit;

    ZeroBit.first.DataValue = DefaultIntensitySliceData & DataBitMask;
    ZeroBit.second.DataValue = DefaultIntensitySliceData | DataBit;

    OneBit.first.DataValue = DefaultIntensitySliceData & DataBitMask;
    OneBit.second.DataValue = DefaultIntensitySliceData | DataBit;

    // DEBUG_END;
} // SetDefaultDataValue

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputGECEI2S::ISR_GetNextDataSlicesToSend (c_OutputI2S::I2S_Item_t * pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData) 
{
    INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSend);
    SET_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendNumSlices, numSlices);

    bool MoreDataToSend = true;

    while (numSlices && MoreDataToSend) 
    {
        while (numSlices && StartupSliceDelay) 
        {
            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendIII);
            
            * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend) ) ^ 0x2) ) = StartBit.second.DataValue;
            pDataToSend += sizeof (StartBit.second.DataValue);
            numSlices-= sizeof (StartBit.second.DataValue);
            StartupSliceDelay--;

            if(0 == StartupSliceDelay)
            {
                ISR_StartNewDataFrame();
            }
        } // while (numSlices && StartupSliceDelay) 

        while (numSlices && StartBit.first.CurrentCount) 
        {
            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendIII);
            
            * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend) ) ^ 0x2) ) = StartBit.first.DataValue;
            pDataToSend += sizeof (StartBit.first.DataValue);
            numSlices-= sizeof (StartBit.first.DataValue);
            StartBit.first.CurrentCount--;
        } // while (numSlices && StartBit.first.CurrentCount) 

        while (numSlices && StartBit.second.CurrentCount) 
        {
            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendIV);
            
            * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend) ) ^ 0x2) ) = StartBit.second.DataValue;
            pDataToSend += sizeof (StartBit.second.DataValue);
            numSlices-= sizeof (StartBit.second.DataValue);
            StartBit.second.CurrentCount--;
        } // while (numSlices && StartBit.second.CurrentCount) 

        while (numSlices && DataPatternMask) 
        {
            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendV);
            
            // do the databit stuff here
            while (numSlices && CurrentBit.first.CurrentCount) 
            {
                INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendVI);
                
                * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend) ) ^ 0x2) ) = CurrentBit.first.DataValue;
                pDataToSend += sizeof (CurrentBit.first.DataValue);
                numSlices-= sizeof (CurrentBit.first.DataValue);
                CurrentBit.first.CurrentCount--;
        
            } // while (numSlices && CurrentBit.first.CurrentCount) 
        
            while (numSlices && CurrentBit.second.CurrentCount) 
            {
                INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendVII);
                
                * ( (c_OutputI2S::I2S_Item_t*) ( ( (uint32_t) (pDataToSend) ) ^ 0x2) ) = CurrentBit.second.DataValue;
                pDataToSend += sizeof (CurrentBit.second.DataValue);
                numSlices-= sizeof (CurrentBit.second.DataValue);
                CurrentBit.second.CurrentCount--;
        
            } // while (numSlices && CurrentBit.second.CurrentCount) 

            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendVIII);

            DataPatternMask = DataPatternMask >> 1;

            if (0 == DataPatternMask) 
            {
                INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendIX);
                // no more bits in this frame
                break;
            }
            else if (DataPattern & DataPatternMask) 
            {
                INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendXI);
                // One Bit
                CurrentBit = OneBit;
            }
            else
            {
                INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendXII);
                // Zero Bit
                CurrentBit = ZeroBit;
            }

            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendXIII);
            CurrentBit.first.CurrentCount = CurrentBit.first.DataBitDataValueCount;
            CurrentBit.second.CurrentCount = CurrentBit.second.DataBitDataValueCount;

        } // while (numSlices && CurrentBit.second.CurrentCount) 

        // did we finish the pixel?
        if (0 == DataPatternMask) 
        {
            INC_GECE_I2S_DEBUG_COUNTERS (GetNextDataSlicesToSendXVI);
            MoreDataToSend = ISR_SetUpNextDataToSend ();
        }

    } // while (numSlices && MoreDataToSend) 
    return MoreDataToSend;

} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputGECEI2S::ISR_SetUpNextDataToSend () 
{
    bool MoreDataToSend = true;
    INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSend);

    do // once
    {
        // do we need to set up the next data byte to send?

        if (NeedToEnumeratePixels)
        {
            INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendI);

            // this functionality does not care about the instensity values and may be sending data from other channels
            if (63 == GetGecePixelId ())
            {
                NeedToEnumeratePixels = false;
            }

        } // NeedToEnumeratePixels

        // is there another data byte to send?
        else if (false == c_OutputPixel::ISR_MoreDataToSend () ) 
        {
            // Frame is complete, set up for the next frame
            INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendII);

            MoreDataToSend = false;
            DataPatternMask = 0;

            StartBit.first.CurrentCount  = 0;
            StartBit.second.CurrentCount = 0;
        
            CurrentBit.first.CurrentCount  = 0;
            CurrentBit.second.CurrentCount = 0;

            break;
        }

        // there is more data to send in the current frame
        INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendIII);

        if (NeedToEnumeratePixels)
        {
            INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendIV);
            // build a GECE intensity frame
            DataPattern = c_OutputPixel::ISR_PixelSendGECEIntensity ();
        }
        else
        {
            INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendV);
            // set up to output the next data byte
            c_OutputPixel::ISR_GetNextIntensityToSend (DataPattern);
        }

        // next bit is MSBit
        DataPatternMask = 0x02000000;

        if (DataPattern & DataPatternMask) 
        {
            INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendVI);
            // One Bit
            CurrentBit = OneBit;
        }
        else
        {
            INC_GECE_I2S_DEBUG_COUNTERS (SetUpNextDataToSendVII);
            CurrentBit = ZeroBit;
        }

        StartBit.first.CurrentCount  = StartBit.first.DataBitDataValueCount;
        StartBit.second.CurrentCount = StartBit.second.DataBitDataValueCount;
    
        CurrentBit.first.CurrentCount  = CurrentBit.first.DataBitDataValueCount;
        CurrentBit.second.CurrentCount = CurrentBit.second.DataBitDataValueCount;

    } while (false); // do once

    return MoreDataToSend;
} // ISR_SetUpNextDataToSend

//----------------------------------------------------------------------------
void c_OutputGECEI2S::PauseOutput (bool State) 
{
    // DEBUG_START;

    // DEBUG_V (String ("PortId: ") + String (OutputPortDefinition.PortId) );
    // DEBUG_V (String (" New State: ") + String (State) );
    // DEBUG_V (String (" Old State: ") + String (IsPaused () ) );
    c_OutputGECE::PauseOutput (State);
    // DEBUG_V (String ("I2Sdriver: 0x") + String (uint32_t (I2Sdriver) , HEX) );

    I2Sdriver->SetOutputState (OutputPortDefinition.PortId, !State);

    // DEBUG_END;
} // PauseOutput

#endif // defined (SUPPORT_OutputProtocol_GECE) && defined (SUPPORT_I2S) 
