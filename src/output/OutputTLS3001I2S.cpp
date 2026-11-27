/*
* OutputTLS3001I2S.cpp - TLS3001 driver code for ESPixelStick I2S Channel
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
#if defined (SUPPORT_OutputProtocol_TLS3001) && defined (SUPPORT_I2S)

#include "output/OutputTLS3001I2S.hpp"

//----------------------------------------------------------------------------
#ifdef USE_TLS3001I2S_COUNTERS
static uint32_t StartNewFrameBaseCount = 0;
static uint32_t GetDataSlicesToSendBaseCount = 0;
static uint32_t SetDefaultDataValueBaseCount = 0;
#endif // def USE_TLS3001I2S_COUNTERS

//----------------------------------------------------------------------------
static void IRAM_ATTR StartNewFrameBase (void * arg)
{
    #ifdef USE_TLS3001I2S_COUNTERS
    StartNewFrameBaseCount++;
    #endif // def USE_TLS3001I2S_COUNTERS
    
    reinterpret_cast<c_OutputTLS3001I2S*> (arg)->ISR_StartNewDataFrame ();
} // StartNewFrameBase

//----------------------------------------------------------------------------
// return false if there are no more data slices to send
static bool IRAM_ATTR GetDataSlicesToSendBase (void * arg, c_OutputI2S::I2S_Item_t * DataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef USE_TLS3001I2S_COUNTERS
    ++GetDataSlicesToSendBaseCount;
    #endif // def USE_TLS3001I2S_COUNTERS

    return reinterpret_cast<c_OutputTLS3001I2S*> (arg)->ISR_GetNextDataSlicesToSend (DataToSend, numSlices, DefaultIntensitySliceData);
} // ISR_GetNextBitToSend

//----------------------------------------------------------------------------
static void SetDefaultDataValueBase (void * arg, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    #ifdef USE_TLS3001I2S_COUNTERS
    ++SetDefaultDataValueBaseCount;
    #endif // def USE_TLS3001I2S_COUNTERS

    reinterpret_cast<c_OutputTLS3001I2S*> (arg)->SetDefaultDataValue (DefaultIntensitySliceData);
} // SetDefaultDataValueBase

//----------------------------------------------------------------------------
c_OutputTLS3001I2S::c_OutputTLS3001I2S (OM_OutputPortDefinition_t & OutputPortDefinition,
                                       c_OutputMgr::e_OutputProtocolType outputType) :
    c_OutputTLS3001 (OutputPortDefinition, outputType)
{
    // DEBUG_START;

    // DEBUG_V (String ("TLS3001_PIXEL_I2S_TICKS_BIT: 0x") + String (TLS3001_PIXEL_I2S_TICKS_BIT, HEX));
    fsm_I2S_state_SendSync_imp.SetParent (this);
    fsm_I2S_state_SendReset_imp.SetParent (this);
    fsm_I2S_state_SendDataStart_imp.SetParent (this);
    fsm_I2S_state_SendData_imp.SetParent (this);
    fsm_I2S_state_SendDataIdle_imp.SetParent (this);

    fsm_I2S_state_SendDataIdle_imp.Init ();

    // DEBUG_END;

} // c_OutputTLS3001I2S

//----------------------------------------------------------------------------
c_OutputTLS3001I2S::~c_OutputTLS3001I2S ()
{
    // DEBUG_START;
    if (HasBeenInitialized)
    {
        I2Sdriver->RemoveSlotDevice (OutputPortDefinition.PortId);
    }

    // DEBUG_END;
} // ~c_OutputTLS3001I2S

//----------------------------------------------------------------------------
/* Use the current config to set up the output port
*/
void c_OutputTLS3001I2S::Begin ()
{
    // DEBUG_START;

    I2Sdriver = OutputMgr.GetI2sDriver ();

    c_OutputTLS3001::Begin ();

    // make sure we are in the paused state
    PauseOutput (true);

    // DEBUG_V (String ("DataPin: ") + String (DataPin));

    FrameResetBitCount  = 0;
    NumSyncIdleBits     = 1;

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
    
    ForceFrameReset ();

    HasBeenInitialized = true;

    // DEBUG_END;

} // Begin

//----------------------------------------------------------------------------
bool c_OutputTLS3001I2S::SetConfig (ArduinoJson::JsonObject& jsonConfig)
{
    // DEBUG_START;

    bool response = c_OutputTLS3001::SetConfig (jsonConfig);

    c_OutputI2S::OutputI2SChannelConfig_t OutputI2SConfig;
    OutputI2SConfig.I2SChannelId              = uint32_t (OutputPortDefinition.PortId);
    OutputI2SConfig.DataPin                   = gpio_num_t (OutputPortDefinition.gpios.data);
    OutputI2SConfig.arg                       = this;
    OutputI2SConfig.StartNewFrame             = StartNewFrameBase;
    OutputI2SConfig.GetNextIntensityBitSlices = GetDataSlicesToSendBase;
    OutputI2SConfig.SetDefaultDataValue       = SetDefaultDataValueBase;
    OutputI2SConfig.IdleLevelIsHigh           = true; // idle is low
    OutputI2SConfig.IsEnabled                  = false; // do not output data

    // DEBUG_V ();    

    I2Sdriver->RegisterSlotDevice (OutputI2SConfig, DataBit, DataBitMask);

    // DEBUG_V (String ("    DataBit: 0x") + String (DataBit, HEX));
    // DEBUG_V (String ("DataBitMask: 0x") + String (DataBitMask, HEX));

    // DEBUG_END;
    return response;

} // SetConfig

//----------------------------------------------------------------------------
void c_OutputTLS3001I2S::SetOutputBufferSize (uint32_t NumChannelsToOutput)
{
    // DEBUG_START;

    // DEBUG_V (String ("NumChannelsToOutput: ") + String (NumChannelsToOutput));
    c_OutputTLS3001::SetOutputBufferSize (NumChannelsToOutput);

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
void c_OutputTLS3001I2S::CalculateFrameBitSlices ()
{
    // DEBUG_START;

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
    
    double NumChannelsAvailable = double (GetNumOutputBufferChannelsServiced ());
    double NumPixels            = NumChannelsAvailable / double (PIXEL_DEFAULT_INTENSITY_BYTES_PER_PIXEL);
    double BaudRateMhz          = 800000.0 / 1000000.0; // 800kHz in MHz
    // calculate the after sync delay based on bit rate and number of pixels on port
    // (Number of connected chips) ÷ (Communication baud rate in MHz) × 30
    double delayUs              = (NumPixels / BaudRateMhz) * 30;
    double delyNS               = NanoSecondsInAMicroSecond * delayUs;

    // DEBUG_V (String ("NumChannelsAvailable: ") + String (NumChannelsAvailable));
    // DEBUG_V (String ("           NumPixels: ") + String (NumPixels));
    // DEBUG_V (String ("         BaudRateMhz: ") + String (BaudRateMhz));
    // DEBUG_V (String ("             delayUs: ") + String (delayUs));
    // DEBUG_V (String ("              delyNS: ") + String (delyNS));

    NumSyncIdleBits = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (delyNS);
    // DEBUG_V (String ("     NumSyncIdleBits: ") + String (NumSyncIdleBits));
    NumSyncIdleBits = (NumSyncIdleBits + (sizeof (c_OutputI2S::I2SDataAsUint_t) - 1)) / sizeof (c_OutputI2S::I2SDataAsUint_t); // round up to the nearest multiple of the data type size
    // DEBUG_V (String ("     NumSyncIdleBits: ") + String (NumSyncIdleBits));

    // DEBUG_V ("SendDataIfg");
    // post frame reset wait time is fixed at 1ms, so we need to calculate the number of slices needed for the post reset time
    uint32_t FrameResetSliceCount = I2Sdriver->GetNumTimeSlicesForTargetTimeNS (1 * NanoSecondsInAMilliSecond);
    FrameResetBitCount = (FrameResetSliceCount + (sizeof (c_OutputI2S::I2SDataAsUint_t) - 1)) / (sizeof (c_OutputI2S::I2SDataAsUint_t)); // round up to the nearest multiple of the data type size
    // DEBUG_V (String ("  FrameResetBitCount: ") + String (FrameResetBitCount));

    // DEBUG_END;

} // CalculateFrameBits

//----------------------------------------------------------------------------
void c_OutputTLS3001I2S::GetStatus (ArduinoJson::JsonObject& jsonStatus)
{
    // // DEBUG_START;
    c_OutputTLS3001::GetStatus (jsonStatus);

    #ifdef USE_I2S_DEBUG_COUNTERS
    // jsonStatus[F ("Can Refresh")] = CanRefresh;
    // jsonStatus[F ("Cannot Refresh")] = CannotRefresh;
    #endif // def USE_I2S_DEBUG_COUNTERS

    #ifdef USE_TLS3001I2S_COUNTERS
    JsonObject CounterStatus = jsonStatus["TLS3001_I2S_DEBUG_COUNTERS"].to<JsonObject> ();

    CounterStatus[F ("StartNewFrameBaseCount")]          = StartNewFrameBaseCount;
    CounterStatus[F ("GetDataSlicesToSendBaseCount")]    = GetDataSlicesToSendBaseCount;
    CounterStatus[F ("SetDefaultDataValueBaseCount")]    = SetDefaultDataValueBaseCount;

    CounterStatus[F ("MinFrameLenMs")]                   = TLS3001I2SCounters.MinFrameLenMs;
    CounterStatus[F ("MaxFrameLenMs")]                   = TLS3001I2SCounters.MaxFrameLenMs;
    CounterStatus[F ("MinPollTimeMs")]                   = TLS3001I2SCounters.MinPollTimeMs;
    CounterStatus[F ("MaxPollTimeMs")]                   = TLS3001I2SCounters.MaxPollTimeMs;
    CounterStatus[F ("NoGpio")]                          = TLS3001I2SCounters.NoGpio;
    CounterStatus[F ("StartNewDataFrame")]               = TLS3001I2SCounters.StartNewDataFrame;
    CounterStatus[F ("StartNewDataFrameI")]              = TLS3001I2SCounters.StartNewDataFrameI;
    CounterStatus[F ("StartNewDataFrameII")]             = TLS3001I2SCounters.StartNewDataFrameII;
    CounterStatus[F ("FrameErrorNotFinished")]           = TLS3001I2SCounters.FrameErrorNotFinished;
    CounterStatus[F ("SendReset_Init")]                  = TLS3001I2SCounters.SendReset_Init;
    CounterStatus[F ("SendReset_GetNextBitToSend")]      = TLS3001I2SCounters.SendReset_GetNextBitToSend;
    CounterStatus[F ("SendReset_GetNextBitToSendI")]     = TLS3001I2SCounters.SendReset_GetNextBitToSendI;
    CounterStatus[F ("SendReset_GetNextBitToSendII")]    = TLS3001I2SCounters.SendReset_GetNextBitToSendII;
    CounterStatus[F ("SendReset_GetNextBitToSendIII")]   = TLS3001I2SCounters.SendReset_GetNextBitToSendIII;
    CounterStatus[F ("SendReset_GetNextBitToSendIV")]    = TLS3001I2SCounters.SendReset_GetNextBitToSendIV;
    CounterStatus[F ("SendReset_GetNextBitToSendV")]     = TLS3001I2SCounters.SendReset_GetNextBitToSendV;
    CounterStatus[F ("SendSync_Init")]                   = TLS3001I2SCounters.SendSync_Init;
    CounterStatus[F ("SendSync_GetNextBitToSend")]       = TLS3001I2SCounters.SendSync_GetNextBitToSend;
    CounterStatus[F ("SendSync_GetNextBitToSendI")]      = TLS3001I2SCounters.SendSync_GetNextBitToSendI;
    CounterStatus[F ("SendSync_GetNextBitToSendII")]     = TLS3001I2SCounters.SendSync_GetNextBitToSendII;
    CounterStatus[F ("SendSync_GetNextBitToSendIII")]    = TLS3001I2SCounters.SendSync_GetNextBitToSendIII;
    CounterStatus[F ("SendSync_GetNextBitToSendIV")]     = TLS3001I2SCounters.SendSync_GetNextBitToSendIV;
    CounterStatus[F ("SendSync_GetNextBitToSendV")]      = TLS3001I2SCounters.SendSync_GetNextBitToSendV;
    CounterStatus[F ("SendSync_GetNextBitToSendVI")]     = TLS3001I2SCounters.SendSync_GetNextBitToSendVI;
    CounterStatus[F ("SendSync_GetNextBitToSendVII")]    = TLS3001I2SCounters.SendSync_GetNextBitToSendVII;
    CounterStatus[F ("SendDataStart_Init")]              = TLS3001I2SCounters.SendDataStart_Init;
    CounterStatus[F ("SendDataStart_GetNextBitToSend")]  = TLS3001I2SCounters.SendDataStart_GetNextBitToSend;
    CounterStatus[F ("SendDataStart_GetNextBitToSendI")] = TLS3001I2SCounters.SendDataStart_GetNextBitToSendI;
    CounterStatus[F ("SendDataStart_GetNextBitToSendII")] = TLS3001I2SCounters.SendDataStart_GetNextBitToSendII;
    CounterStatus[F ("SendDataStart_GetNextBitToSendIII")] = TLS3001I2SCounters.SendDataStart_GetNextBitToSendIII;
    CounterStatus[F ("SendDataStart_GetNextBitToSendIV")] = TLS3001I2SCounters.SendDataStart_GetNextBitToSendIV;
    CounterStatus[F ("SendDataStart_GetNextBitToSendV")] = TLS3001I2SCounters.SendDataStart_GetNextBitToSendV;
    CounterStatus[F ("SendData_Init")]                   = TLS3001I2SCounters.SendData_Init;
    CounterStatus[F ("SendData_GetNextBitToSend")]       = TLS3001I2SCounters.SendData_GetNextBitToSend;
    CounterStatus[F ("SendData_GetNextBitToSendI")]      = TLS3001I2SCounters.SendData_GetNextBitToSendI;
    CounterStatus[F ("SendData_GetNextBitToSendII")]     = TLS3001I2SCounters.SendData_GetNextBitToSendII;
    CounterStatus[F ("SendData_GetNextBitToSendIII")]    = TLS3001I2SCounters.SendData_GetNextBitToSendIII;
    CounterStatus[F ("SendData_GetNextBitToSendIV")]     = TLS3001I2SCounters.SendData_GetNextBitToSendIV;
    CounterStatus[F ("SendData_GetNextBitToSendV")]      = TLS3001I2SCounters.SendData_GetNextBitToSendV;
    CounterStatus[F ("SendData_RefreshData")]            = TLS3001I2SCounters.SendData_RefreshData;
    CounterStatus[F ("SendDataIdle_Init")]               = TLS3001I2SCounters.SendDataIdle_Init;
    CounterStatus[F ("SendDataIdle_GetNextBitToSend")]   = TLS3001I2SCounters.SendDataIdle_GetNextBitToSend;

    #endif // def USE_TLS3001I2S_COUNTERS

    // // DEBUG_END;
} // GetStatus

//----------------------------------------------------------------------------
void IRAM_ATTR c_OutputTLS3001I2S::ISR_StartNewDataFrame ()
{
    // DEBUG_START;
    // DEBUG_V (String ("frame started on ") + String (OutputPortDefinition.gpios.data));
    INCREMENT_TLS3001_COUNTER (StartNewDataFrame);

    if (FrameResetIsNeeded ())
    {
        INCREMENT_TLS3001_COUNTER (StartNewDataFrameI);
        fsm_I2S_state_SendReset_imp.Init ();
    }
    else
    {
        INCREMENT_TLS3001_COUNTER (StartNewDataFrameII);
        fsm_I2S_state_SendDataStart_imp.Init ();
    }

    // DEBUG_END;
} // StartNewDataFrame

//----------------------------------------------------------------------------
void c_OutputTLS3001I2S::SetDefaultDataValue (c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    // DEBUG_START;

    // DEBUG_V (String ("     SetDefaultDataValue:0x") + String (DefaultIntensitySliceData, HEX));
    // DEBUG_V (String ("                 DataBit:0x") + String (DataBit, HEX));
    // DEBUG_V (String ("             DataBitMask:0x") + String (DataBitMask, HEX));

    OneBit.DataAsBytes[2] = DefaultIntensitySliceData & DataBitMask;
    OneBit.DataAsBytes[3] = DefaultIntensitySliceData & DataBitMask;
    OneBit.DataAsBytes[0] = DefaultIntensitySliceData | DataBit;
    OneBit.DataAsBytes[1] = DefaultIntensitySliceData | DataBit;

    ZeroBit.DataAsBytes[2] = DefaultIntensitySliceData | DataBit;
    ZeroBit.DataAsBytes[3] = DefaultIntensitySliceData | DataBit;
    ZeroBit.DataAsBytes[0] = DefaultIntensitySliceData & DataBitMask;
    ZeroBit.DataAsBytes[1] = DefaultIntensitySliceData & DataBitMask;

    memset (IdleBit.DataAsBytes, DefaultIntensitySliceData, sizeof (IdleBit));

    // for testing only
//    IdleBit.DataAsBytes[2] = DefaultIntensitySliceData | DataBit;
//    IdleBit.DataAsBytes[3] = DefaultIntensitySliceData & DataBitMask;
//    IdleBit.DataAsBytes[0] = DefaultIntensitySliceData | DataBit;
//    IdleBit.DataAsBytes[1] = DefaultIntensitySliceData & DataBitMask;

    // DEBUG_END;
} // SetDefaultDataValue

//----------------------------------------------------------------------------
void c_OutputTLS3001I2S::PauseOutput (bool State)
{
    // DEBUG_START;

    c_OutputTLS3001::PauseOutput (State);
    // DEBUG_V (String ("PortId: ") + String (OutputPortDefinition.PortId));
    // DEBUG_V (String (" New State: ") + String (State));
    // DEBUG_V (String (" Old State: ") + String (IsPaused ()));
    if (IsPaused ())
    {
        fsm_I2S_state_SendDataIdle_imp.Init ();
    }
    I2Sdriver->SetOutputState (OutputPortDefinition.PortId, !State);

    // DEBUG_END;
} // PauseOutput

//----------------------------------------------------------------------------
bool IRAM_ATTR c_OutputTLS3001I2S::ISR_GetNextDataSlicesToSend (c_OutputI2S::I2S_Item_t * pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    bool MoreDataToSend = true;
    while (numSlices > sizeof (c_OutputI2S::I2SDataAsUint_t) && MoreDataToSend)
    {
        switch (CurrentFsmState)
        {
            // below states cant use virtual functions because they are called from an ISR context
            case OutputTLS3001I2SFsmStates_t::TLS3001Sync:
            {
                MoreDataToSend = fsm_I2S_state_SendSync_imp.ISR_GetNextBitToSend (pDataToSend, numSlices, DefaultIntensitySliceData);
                break;
            }
            case OutputTLS3001I2SFsmStates_t::TLS3001DataStart:
            {
                MoreDataToSend = fsm_I2S_state_SendDataStart_imp.ISR_GetNextBitToSend (pDataToSend, numSlices, DefaultIntensitySliceData);
                break;
            }
            case OutputTLS3001I2SFsmStates_t::TLS3001DataSend:
            {
                MoreDataToSend = fsm_I2S_state_SendData_imp.ISR_GetNextBitToSend (pDataToSend, numSlices, DefaultIntensitySliceData);
                break;
            }
            case OutputTLS3001I2SFsmStates_t::TLS3001DataIdle:
            {
                MoreDataToSend = fsm_I2S_state_SendDataIdle_imp.ISR_GetNextBitToSend (pDataToSend, numSlices, DefaultIntensitySliceData);
                break;
            }
            default:
            case OutputTLS3001I2SFsmStates_t::TLS3001Reset:
            {
                MoreDataToSend = fsm_I2S_state_SendReset_imp.ISR_GetNextBitToSend (pDataToSend, numSlices, DefaultIntensitySliceData);
                break;
            }
        };
    } // end while (numSlices > sizeof (c_OutputI2S::I2SDataAsUint_t) && MoreDataToSend)
    return MoreDataToSend;
} // ISR_GetNextIntensityToSend

//----------------------------------------------------------------------------
// FSM
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IRAM_ATTR fsm_I2S_state_SendReset::Init ()
{
    INCREMENT_TLS3001_FSM_COUNTER (SendReset_Init);
    // DEBUG_START;

    // Serial.print ('b');

    Parent->CurrentFsmState = OutputTLS3001I2SFsmStates_t::TLS3001Reset;
    Parent->ResetFrameCounter ();

    NumberOfOneBitsToSend = TLS3001_NumLeadingOnes;
    CommandCodeMask =CommandCodeMaskValue; // reset
    NumberOfIdleBitsToSend = Parent->FrameResetBitCount;

    // DEBUG_END;
} // fsm_I2S_state_SendReset

//----------------------------------------------------------------------------
bool IRAM_ATTR fsm_I2S_state_SendReset::ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    bool MoreDataToSend = true;

    INCREMENT_TLS3001_FSM_COUNTER (SendReset_GetNextBitToSend);

    // Serial.print ('2');

    while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))
    {
        if (NumberOfOneBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendReset_GetNextBitToSendII);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->OneBit;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);
            --NumberOfOneBitsToSend;

        } // end if (NumberOfOneBitsToSend)

        else if (CommandCodeMask)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendReset_GetNextBitToSendIII);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = (CommandCode & CommandCodeMask) ? Parent->OneBit : Parent->ZeroBit;
            CommandCodeMask = CommandCodeMask >> 1;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

        } // end if (CommandCodeMask)

        else if (NumberOfIdleBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendReset_GetNextBitToSendIV);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->IdleBit;

            --NumberOfIdleBitsToSend;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

        } // end if (NumberOfIdleBitsToSend)

        else
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendReset_GetNextBitToSendV);

            Parent->fsm_I2S_state_SendSync_imp.Init ();
            break;
        }
    } // end while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))

    return MoreDataToSend;

} // fsm_I2S_state_SendReset

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IRAM_ATTR fsm_I2S_state_SendSync::Init ()
{
    INCREMENT_TLS3001_FSM_COUNTER (SendSync_Init);
    // DEBUG_START;

    // Serial.print ('c');

    Parent->CurrentFsmState = OutputTLS3001I2SFsmStates_t::TLS3001Sync;

    NumberOfOneBitsToSend = TLS3001_NumLeadingOnes;
    CommandCodeMask = CommandCodeMaskValue;
    NumberOfZeroBitsToSend = 11;
    NumberOfIdleBitsToSend = Parent->NumSyncIdleBits;

    // DEBUG_END;
} // fsm_I2S_state_SendSync

//----------------------------------------------------------------------------
bool IRAM_ATTR fsm_I2S_state_SendSync::ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    // Serial.print ('3');
    bool MoreDataToSend = true;

    INCREMENT_TLS3001_FSM_COUNTER (SendSync_GetNextBitToSend);

    while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))
    {
        if (NumberOfOneBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendSync_GetNextBitToSendIII);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->OneBit;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);
            --NumberOfOneBitsToSend;

        } // end if (NumberOfOneBitsToSend)

        else if (CommandCodeMask)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendSync_GetNextBitToSendIV);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = (CommandCode & CommandCodeMask) ? Parent->OneBit : Parent->ZeroBit;
            CommandCodeMask = CommandCodeMask >> 1;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

        } // end if (CommandCodeMask)

        else if (NumberOfZeroBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendSync_GetNextBitToSendV);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->ZeroBit;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);
            --NumberOfZeroBitsToSend;

        } // end if (NumberOfZeroBitsToSend)

        else if (NumberOfIdleBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendSync_GetNextBitToSendVI);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->IdleBit;

            --NumberOfIdleBitsToSend;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

        } // end if (NumberOfIdleBitsToSend)

        else
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendSync_GetNextBitToSendVII);

            Parent->fsm_I2S_state_SendDataStart_imp.Init ();
            break;
        }
    } // end while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))

    return MoreDataToSend;
} // fsm_I2S_state_SendSync

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IRAM_ATTR fsm_I2S_state_SendDataStart::Init ()
{
    INCREMENT_TLS3001_FSM_COUNTER (SendDataStart_Init);
    // DEBUG_START;
    // Serial.print ('d');

    Parent->IncrementFrameCounter ();
    Parent->c_OutputTLS3001::ISR_StartNewFrame ();

    NumberOfOneBitsToSend = TLS3001_NumLeadingOnes;
    CommandCodeMask = CommandCodeMaskValue;

    Parent->CurrentFsmState = OutputTLS3001I2SFsmStates_t::TLS3001DataStart;

    // DEBUG_END;
} // fsm_I2S_state_SendDataStart::Init

//----------------------------------------------------------------------------
bool IRAM_ATTR fsm_I2S_state_SendDataStart::ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    bool MoreDataToSend = true;

    INCREMENT_TLS3001_FSM_COUNTER (SendDataStart_GetNextBitToSend);
    // Serial.print ('4');

    while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))
    {
        if (NumberOfOneBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendDataStart_GetNextBitToSendIII);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->OneBit;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);
            --NumberOfOneBitsToSend;

        } // end if (NumberOfOneBitsToSend)

        else if (CommandCodeMask)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendDataStart_GetNextBitToSendIV);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = (CommandCode & CommandCodeMask) ? Parent->OneBit : Parent->ZeroBit;
            CommandCodeMask = CommandCodeMask >> 1;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

        } // end if (CommandCodeMask)

        else
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendDataStart_GetNextBitToSendV);

            Parent->fsm_I2S_state_SendData_imp.Init ();
            break;
        }
    } // end while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))

    return MoreDataToSend;
} // fsm_I2S_state_SendDataStart

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IRAM_ATTR fsm_I2S_state_SendData::Init ()
{
    INCREMENT_TLS3001_FSM_COUNTER (SendData_Init);
    // DEBUG_START;
    // Serial.print ('e');

    Parent->CurrentFsmState = OutputTLS3001I2SFsmStates_t::TLS3001DataSend;

    ISR_RefreshData ();
    NumPostDataIdleBitsToSend = 5;

    // DEBUG_END;
} // fsm_I2S_state_SendData

//----------------------------------------------------------------------------
bool IRAM_ATTR fsm_I2S_state_SendData::ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    INCREMENT_TLS3001_FSM_COUNTER (SendData_GetNextBitToSend);
    bool MoreDataToSend = true;
    // Serial.print ('5');

    while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t) && MoreDataToSend)
    {
        // send 13 bits of data (1 leading zero + 12 data bits)
        if (DataPatternMask)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendData_GetNextBitToSendII);

            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = (DataPattern & DataPatternMask) ? Parent->OneBit : Parent->ZeroBit;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

            DataPatternMask = DataPatternMask >> 1;
    
            // are there more data bits to send?
            if (0 == DataPatternMask)
            {
                INCREMENT_TLS3001_FSM_COUNTER (SendData_GetNextBitToSendV);

                // set up next data value.
                ISR_RefreshData ();
            }

        } // end if (DataPatternMask != 0)

        else if (NumPostDataIdleBitsToSend)
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendData_GetNextBitToSendIII);

            --NumPostDataIdleBitsToSend;
            * ((c_OutputI2S::I2SDataAsUint_t*) pDataToSend) = Parent->IdleBit;
            numSlices -= sizeof (c_OutputI2S::I2SDataAsUint_t);
            pDataToSend += sizeof (c_OutputI2S::I2SDataAsUint_t);

        } // end if (NumPostDataIdleBitsToSend)

        else
        {
            INCREMENT_TLS3001_FSM_COUNTER (SendData_GetNextBitToSendIV);

            Parent->fsm_I2S_state_SendDataIdle_imp.Init ();
            break;
        }
    } // end while (numSlices >= sizeof (c_OutputI2S::I2SDataAsUint_t))

    return MoreDataToSend;
} // fsm_I2S_state_SendData

//----------------------------------------------------------------------------
bool IRAM_ATTR fsm_I2S_state_SendData::ISR_RefreshData ()
{
    INCREMENT_TLS3001_FSM_COUNTER (SendData_RefreshData);
    bool MoreDataAvailable = Parent->ISR_MoreDataToSend ();

    if (MoreDataAvailable)
    {
        uint32_t NewData = 0;
        Parent->c_OutputTLS3001::ISR_GetNextIntensityToSend (NewData);

        // mask includes the leading zero bit. 
        DataPatternMask = DataPatternMaskValue;

        // convert 8 bit data to 12 bit data
        DataPattern = (NewData * 4095) / 255;
        DataPattern = DataPattern & ~DataPatternMask; // clear the leading zero bit
    }
    else
    {
        DataPatternMask = 0;
    }

    return MoreDataAvailable;
} // ISR_RefreshData

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void IRAM_ATTR fsm_I2S_state_SendDataIdle::Init ()
{
    INCREMENT_TLS3001_FSM_COUNTER (SendDataIdle_Init);
    // DEBUG_START;
    // Serial.print ('f');

    Parent->CurrentFsmState = OutputTLS3001I2SFsmStates_t::TLS3001DataIdle;
    // DEBUG_END;
} // fsm_I2S_state_SendData

//----------------------------------------------------------------------------
bool IRAM_ATTR fsm_I2S_state_SendDataIdle::ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData)
{
    INCREMENT_TLS3001_FSM_COUNTER (SendDataIdle_GetNextBitToSend);
    // Serial.print ('6');
    // no more data to send
    return false;
} // fsm_I2S_state_SendDataIdle

#endif // defined (SUPPORT_OutputProtocol_TLS3001) && defined (SUPPORT_I2S)
