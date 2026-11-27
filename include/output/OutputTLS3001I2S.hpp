#pragma once
/*
* OutputTLS3001I2S.h - TLS3001 driver code for ESPixelStick I2S Channel
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
#if defined(SUPPORT_OutputProtocol_TLS3001) && defined (SUPPORT_I2S)

#include "OutputTLS3001.hpp"
#include "OutputI2S.hpp"
#include "OutputTLS3001I2SFsm.hpp"

class c_OutputTLS3001I2S : public c_OutputTLS3001
{
public:
    // These functions are inherited from c_OutputCommon
    c_OutputTLS3001I2S(OM_OutputPortDefinition_t & OutputPortDefinition, c_OutputMgr::e_OutputProtocolType outputType);
    virtual ~c_OutputTLS3001I2S ();

    // functions to be provided by the derived class
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

protected:
    friend class fsm_I2S_state_SendSync;
    friend class fsm_I2S_state_SendReset;
    friend class fsm_I2S_state_SendDataStart;
    friend class fsm_I2S_state_SendData;
    friend class fsm_I2S_state_SendDataIdle;
    friend class fsm_I2S_state;
    OutputTLS3001I2SFsmStates_t CurrentFsmState = OutputTLS3001I2SFsmStates_t::TLS3001DataIdle;

    fsm_I2S_state_SendSync      fsm_I2S_state_SendSync_imp;
    fsm_I2S_state_SendReset     fsm_I2S_state_SendReset_imp;
    fsm_I2S_state_SendDataStart fsm_I2S_state_SendDataStart_imp;
    fsm_I2S_state_SendData      fsm_I2S_state_SendData_imp;
    fsm_I2S_state_SendDataIdle  fsm_I2S_state_SendDataIdle_imp;

    c_OutputI2S     *I2Sdriver;

    uint32_t        FrameResetBitCount; // must be 1ms

    uint32_t        DataPattern;
    uint32_t        DataPatternMask;

    uint32_t        NumSyncIdleBits;

    c_OutputI2S::I2SDataAsUint_t OneBit;
    c_OutputI2S::I2SDataAsUint_t ZeroBit;
    c_OutputI2S::I2SDataAsUint_t IdleBit;

    c_OutputI2S::I2S_Item_t DataBitMask;
    c_OutputI2S::I2S_Item_t DataBit;

    // #define USE_TLS3001I2S_COUNTERS
    #ifdef USE_TLS3001I2S_COUNTERS
        #define INCREMENT_TLS3001_COUNTER(c) (++TLS3001I2SCounters.c)
        #define INCREMENT_TLS3001_FSM_COUNTER(c) (++Parent->TLS3001I2SCounters.c)
    struct TLS3001I2SCounters_t
    {
        uint32_t StartNewDataFrame  = 0;
        uint32_t StartNewDataFrameI = 0;
        uint32_t StartNewDataFrameII = 0;
        uint32_t FrameErrorNotFinished = 0;
        uint32_t MinFrameLenMs = uint32_t(-1);
        uint32_t MaxFrameLenMs = 0;
        uint32_t MinPollTimeMs = uint32_t(-1);
        uint32_t MaxPollTimeMs = 0;
        uint32_t NoGpio        = 0;
        uint32_t SendReset_Init = 0;
        uint32_t SendReset_GetNextBitToSend = 0;
        uint32_t SendReset_GetNextBitToSendI = 0;
        uint32_t SendReset_GetNextBitToSendII = 0;
        uint32_t SendReset_GetNextBitToSendIII = 0;
        uint32_t SendReset_GetNextBitToSendIV = 0;
        uint32_t SendReset_GetNextBitToSendV = 0;
        uint32_t SendSync_Init = 0;
        uint32_t SendSync_GetNextBitToSend = 0;
        uint32_t SendSync_GetNextBitToSendI = 0;
        uint32_t SendSync_GetNextBitToSendII = 0;
        uint32_t SendSync_GetNextBitToSendIII = 0;
        uint32_t SendSync_GetNextBitToSendIV = 0;
        uint32_t SendSync_GetNextBitToSendV = 0;
        uint32_t SendSync_GetNextBitToSendVI = 0;
        uint32_t SendSync_GetNextBitToSendVII = 0;
        uint32_t SendDataStart_Init = 0;
        uint32_t SendDataStart_GetNextBitToSend = 0;
        uint32_t SendDataStart_GetNextBitToSendI = 0;
        uint32_t SendDataStart_GetNextBitToSendII = 0;
        uint32_t SendDataStart_GetNextBitToSendIII = 0;
        uint32_t SendDataStart_GetNextBitToSendIV = 0;
        uint32_t SendDataStart_GetNextBitToSendV = 0;
        uint32_t SendData_Init = 0;
        uint32_t SendData_GetNextBitToSend = 0;
        uint32_t SendData_GetNextBitToSendI = 0;
        uint32_t SendData_GetNextBitToSendII = 0;
        uint32_t SendData_GetNextBitToSendIII = 0;
        uint32_t SendData_GetNextBitToSendIV = 0;
        uint32_t SendData_GetNextBitToSendV = 0;
        uint32_t SendData_RefreshData = 0;
        uint32_t SendDataIdle_Init = 0;
        uint32_t SendDataIdle_GetNextBitToSend = 0;
    } TLS3001I2SCounters;

    #else
        #define INCREMENT_TLS3001_COUNTER(c)
        #define INCREMENT_TLS3001_FSM_COUNTER(c)
    #endif // def USE_TLS3001I2S_COUNTERS

}; // c_OutputTLS3001I2S

#endif // defined(SUPPORT_OutputProtocol_TLS3001) && defined (SUPPORT_I2S)
