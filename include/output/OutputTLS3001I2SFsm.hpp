#pragma once
/*
* OutputTLS3001I2SFsm.h - TLS3001 driver code for ESPixelStick I2S Channel
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2026 Shelby Merrick
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

// forward declaration
class c_OutputTLS3001I2S;

enum OutputTLS3001I2SFsmStates_t
{
    TLS3001Reset = 0,
    TLS3001Sync,
    TLS3001DataStart,
    TLS3001DataSend,
    TLS3001DataIdle
};

/*****************************************************************************/
/*
 *	Generic fsm base class.
 */
/*****************************************************************************/
class fsm_I2S_state
{
public:
             fsm_I2S_state() {}
    virtual ~fsm_I2S_state() {}
            void SetParent(c_OutputTLS3001I2S *_Parent) {Parent = _Parent;};
    virtual void GetDriverName (String& Name) = 0;

public:
    c_OutputTLS3001I2S  *Parent = nullptr;
    uint32_t NumberOfOneBitsToSend;
    uint32_t CommandCodeMask;
    uint32_t NumberOfZeroBitsToSend;
    uint32_t NumberOfIdleBitsToSend;

    // all commands are 4 bits long
    const uint32_t CommandCodeMaskValue = 0b1000;

    // mask includes the leading zero bit. 
    const uint32_t DataPatternMaskValue = 0b1000000000000;

}; // fsm_I2S_state

/*****************************************************************************/
class fsm_I2S_state_SendSync : public fsm_I2S_state
{
public:
    fsm_I2S_state_SendSync() {}
    virtual ~fsm_I2S_state_SendSync() {}
            void Init();
            bool ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);
    virtual void GetDriverName (String& Name) { Name = F("TLS3001SYN"); }
private:
    const uint8_t CommandCode = 0b0001;
}; // fsm_I2S_state_SendSync

/*****************************************************************************/
class fsm_I2S_state_SendReset : public fsm_I2S_state
{
public:
    fsm_I2S_state_SendReset() {}
    virtual ~fsm_I2S_state_SendReset() {}
            void Init();
            bool ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);
    virtual void GetDriverName (String& Name) { Name = F("TLS3001RST"); }
private:
    const uint8_t CommandCode = 0b0100;
}; // fsm_I2S_state_SendReset

/*****************************************************************************/
class fsm_I2S_state_SendDataStart : public fsm_I2S_state
{
public:
    fsm_I2S_state_SendDataStart() {}
    virtual ~fsm_I2S_state_SendDataStart() {}
            void Init();
            bool ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);
    virtual void GetDriverName (String& Name) { Name = F("TLS3001STR"); }
private:
    const uint8_t CommandCode = 0b0010;

}; // fsm_I2S_state_SendDataStart

/*****************************************************************************/
class fsm_I2S_state_SendData : public fsm_I2S_state
{
public:
    fsm_I2S_state_SendData() {}
    virtual ~fsm_I2S_state_SendData() {}
            void Init();
            bool ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);
            bool ISR_RefreshData ();
    virtual void GetDriverName (String& Name) { Name = F("TLS3001DAT"); }
private:
    uint32_t DataPattern = 0;
    uint32_t DataPatternMask = 0;
    uint32_t NumPostDataIdleBitsToSend = 0;
}; // fsm_I2S_state_SendData

/*****************************************************************************/
class fsm_I2S_state_SendDataIdle : public fsm_I2S_state
{
public:
    fsm_I2S_state_SendDataIdle() {}
    virtual ~fsm_I2S_state_SendDataIdle() {}
            void Init();
            bool ISR_GetNextBitToSend (c_OutputI2S::I2S_Item_t *& pDataToSend, uint32_t & numSlices, c_OutputI2S::I2S_Item_t DefaultIntensitySliceData);
    virtual void GetDriverName (String& Name) { Name = F("TLS3001IDL"); }
private:
}; // fsm_I2S_state_SendData

#endif // defined(SUPPORT_OutputProtocol_TLS3001) && defined (SUPPORT_I2S)
