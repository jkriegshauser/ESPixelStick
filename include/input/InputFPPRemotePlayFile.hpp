#pragma once
/*
* InputFPPRemotePlayFile.hpp
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2021, 2025 Shelby Merrick
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
*   PlayFile object used to parse and play an FSEQ File
*/

#include "ESPixelStick.h"
#include "InputFPPRemotePlayItem.hpp"
#include "InputFPPRemotePlayFileFsm.hpp"
#include "service/fseq.h"

#ifdef ARDUINO_ARCH_ESP32
#include <esp_task.h>
#endif // def ARDUINO_ARCH_ESP32

class c_InputFPPRemotePlayFile : public c_InputFPPRemotePlayItem
{
public:
    c_InputFPPRemotePlayFile (c_InputMgr::e_InputChannelIds InputChannelId);
    virtual ~c_InputFPPRemotePlayFile ();

    virtual void Start (String & FileName, float SecondsElapsed, uint32_t RemainingPlayCount);
    virtual void Stop ();
    virtual void Sync (String& FileName, float SecondsElapsed);
    virtual bool Poll ();
    virtual void GetStatus (JsonObject & jsonStatus);
    virtual void ClearStatistics ();
    virtual bool IsIdle () { return (pCurrentFsmState == &fsm_PlayFile_state_Idle_imp); }
    virtual void ProcessDeferredFileWork ();

#ifdef ARDUINO_ARCH_ESP32
    TaskHandle_t GetTaskHandle () { return TimerPollTaskHandle; }
    volatile bool TimerPollInProgress = false;
#endif // def ARDUINO_ARCH_ESP32

private:
#define ELAPSED_PLAY_TIMER_INTERVAL_MS  10

    void ClearControlFileInfo ();

    // On the ESP8266 the input manager runs from a Ticker, in the SDK's timer context, where a wait on the
    // SD card holds up the network stack and loop() (#20). There the state machine only decides what has
    // to happen; the file open and the frame reads are done from loop() by ProcessDeferredFileWork().
    // The ESP32 runs the input manager in its own task and does that work in place.
#ifdef ARDUINO_ARCH_ESP8266
    static constexpr bool DeferFileWork = true;
#else
    static constexpr bool DeferFileWork = false;
#endif // def ARDUINO_ARCH_ESP8266
    volatile bool     StartRequested     = false; ///< Starting: the control block is set up, open and parse the file
    volatile bool     FrameRequested     = false; ///< PlayingFile: read RequestedFrameId
    volatile uint32_t RequestedFrameId   = 0;
    volatile bool     FileWorkInProgress = false; ///< loop() is inside FinishStart or ReadFrame
    void FinishStart ();
    void ReadFrame (uint32_t FrameId);

    friend class fsm_PlayFile_state_Idle;
    friend class fsm_PlayFile_state_Starting;
    friend class fsm_PlayFile_state_PlayingFile;
    friend class fsm_PlayFile_state_Stopping;
    friend class fsm_PlayFile_state_Error;
    friend class fsm_PlayFile_state;

    fsm_PlayFile_state_Idle        fsm_PlayFile_state_Idle_imp;
    fsm_PlayFile_state_Starting    fsm_PlayFile_state_Starting_imp;
    fsm_PlayFile_state_PlayingFile fsm_PlayFile_state_PlayingFile_imp;
    fsm_PlayFile_state_Stopping    fsm_PlayFile_state_Stopping_imp;
    fsm_PlayFile_state_Error       fsm_PlayFile_state_Error_imp;

    fsm_PlayFile_state * pCurrentFsmState = &fsm_PlayFile_state_Idle_imp;

    struct SyncControl_t
    {
        uint32_t          SyncCount = 0;
        uint32_t          SyncAdjustmentCount = 0;
        float             LastRcvdElapsedSeconds = 0.0;
    } SyncControl;

#define MAX_NUM_SPARSE_RANGES 5
    FSEQParsedRangeEntry SparseRanges[MAX_NUM_SPARSE_RANGES];

    void        UpdateElapsedPlayTimeMS ();
    uint32_t    CalculateFrameId (uint32_t ElapsedMS, int32_t SyncOffsetMS);
    bool        ParseFseqFile ();
    uint64_t    ReadFile(uint64_t DestinationIntensityId, uint64_t NumBytesToRead, uint64_t FileOffset);

    char      LastFailedPlayStatusMsg[128];
    char      LastFailedFilename[65];

#ifdef ARDUINO_ARCH_ESP32
    TaskHandle_t TimerPollTaskHandle = NULL;
#   define TimerPollHandlerTaskStack 3000
// #   define TimerPollHandlerTaskStack 6000
#endif // def ARDUINO_ARCH_ESP32

}; // c_InputFPPRemotePlayFile
