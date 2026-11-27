#pragma once
/*
* OutputGECEI2S.h - GECE driver code for ESPixelStick I2S Channel
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
#if defined(SUPPORT_OutputProtocol_GECE) && defined(SUPPORT_I2S)

#include "OutputGECE.hpp"
#include "OutputI2S.hpp"

class c_OutputGECEI2S : public c_OutputGECE
{
public:
    // These functions are inherited from c_OutputCommon
    c_OutputGECEI2S (OM_OutputPortDefinition_t & OutputPortDefinition,
                     c_OutputMgr::e_OutputProtocolType outputType);
    virtual ~c_OutputGECEI2S ();

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

    struct DataBitTransition_t
    {
        uint32_t DataBitDataValueCount;
        uint32_t CurrentCount;
        c_OutputI2S::I2S_Item_t DataValue;
    };

    struct DataBitCounts_t
    {
        DataBitTransition_t first;
        DataBitTransition_t second;
    }; // DataBitCounts_t

    DataBitCounts_t StartBit;
    DataBitCounts_t ZeroBit;
    DataBitCounts_t OneBit;
    DataBitCounts_t CurrentBit;

    uint32_t StartupSliceDelay;

    c_OutputI2S     *I2Sdriver;

    uint32_t        DataPattern;
    uint32_t        DataPatternMask;

    c_OutputI2S::I2S_Item_t DataBitMask;
    c_OutputI2S::I2S_Item_t DataBit;

    bool NeedToEnumeratePixels;

    // #define GECE_I2S_DEBUG_COUNTERS
    #ifdef GECE_I2S_DEBUG_COUNTERS
    #define INC_GECE_I2S_DEBUG_COUNTERS(c) (++I2SDebugCounters.c)
    #define SET_GECE_I2S_DEBUG_COUNTERS(c, v) (I2SDebugCounters.c = v)
    struct
    {
        uint32_t StartNewDataFrame;
        uint32_t GetNextDataSlicesToSend;
        uint32_t GetNextDataSlicesToSendNumSlices;
        uint32_t GetNextDataSlicesToSendI;
        uint32_t GetNextDataSlicesToSendII;
        uint32_t GetNextDataSlicesToSendIII;
        uint32_t GetNextDataSlicesToSendIV;
        uint32_t GetNextDataSlicesToSendV;
        uint32_t GetNextDataSlicesToSendVI;
        uint32_t GetNextDataSlicesToSendVII;
        uint32_t GetNextDataSlicesToSendVIII;
        uint32_t GetNextDataSlicesToSendIX;
        uint32_t GetNextDataSlicesToSendX;
        uint32_t GetNextDataSlicesToSendXI;
        uint32_t GetNextDataSlicesToSendXII;
        uint32_t GetNextDataSlicesToSendXIII;
        uint32_t GetNextDataSlicesToSendXIV;
        uint32_t GetNextDataSlicesToSendXV;
        uint32_t GetNextDataSlicesToSendXVI;
        uint32_t SetUpNextDataToSend;
        uint32_t SetUpNextDataToSendI;
        uint32_t SetUpNextDataToSendII;
        uint32_t SetUpNextDataToSendIII;
        uint32_t SetUpNextDataToSendIV;
        uint32_t SetUpNextDataToSendV;
        uint32_t SetUpNextDataToSendVI;
        uint32_t SetUpNextDataToSendVII;

    } I2SDebugCounters;
    #else
    #define INC_GECE_I2S_DEBUG_COUNTERS(c)
    #define SET_GECE_I2S_DEBUG_COUNTERS(c, v)

    #endif // def GECE_I2S_DEBUG_COUNTERS

}; // c_OutputGECEI2S

#endif // defined(SUPPORT_OutputProtocol_GECE) && defined(SUPPORT_I2S)
