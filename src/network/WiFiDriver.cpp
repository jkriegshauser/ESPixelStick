/*
* WiFiDriver.cpp - Output Management class
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
*/

#include "ESPixelStick.h"

#ifdef ARDUINO_ARCH_ESP8266
#   include <eagle_soc.h>
#   include <ets_sys.h>
#else
#   include <esp_wifi.h>
#endif // def ARDUINO_ARCH_ESP8266

#include "network/WiFiDriver.hpp"
#include "network/NetworkMgr.hpp"
#include "FileMgr.hpp"

//-----------------------------------------------------------------------------
/*
    There are three ways to define the default Network Name and PassPhrase

    1) Create a secrets.h file and place it in the network directory with the WiFiDriver.cpp file
    2) Use platformio_user.ini to define the WiFi Credentials for your platform
    3) Edit the strings below directly
*/

#if __has_include("secrets.h")
#   include "secrets.h"
#endif //  __has_include("secrets.h")

#if !defined(SECRETS_SSID)
#   define SECRETS_SSID DEFAULT_SSID_NOT_SET
#endif // SECRETS_SSID
#if !defined(SECRETS_PASS)
#   define SECRETS_PASS "DEFAULT_PASSPHRASE_NOT_SET"
#endif // SECRETS_PASS

/* Fallback configuration if config.json is empty or fails */
const String default_ssid       = SECRETS_SSID;
const String default_passphrase = SECRETS_PASS;

/// Radio configuration
/** ESP8266 radio configuration routines that are executed at startup. */
/* Disabled for now, possible flash wear issue. Need to research further
RF_PRE_INIT() {
#ifdef ARDUINO_ARCH_ESP8266
    system_phy_set_powerup_option(3);   // Do full RF calibration on power-up
    system_phy_set_max_tpw(82);         // Set max TX power
#else
    esp_phy_erase_cal_data_in_nvs(); // Do full RF calibration on power-up
    esp_wifi_set_max_tx_power (78);  // Set max TX power
#endif
}
*/

/*****************************************************************************/
/* FSM                                                                       */
/*****************************************************************************/
fsm_WiFi_state_Boot                    fsm_WiFi_state_Boot_imp;
fsm_WiFi_state_ConnectingUsingConfig   fsm_WiFi_state_ConnectingUsingConfig_imp;
fsm_WiFi_state_ConnectingUsingDefaults fsm_WiFi_state_ConnectingUsingDefaults_imp;
fsm_WiFi_state_ConnectedToAP           fsm_WiFi_state_ConnectedToAP_imp;
fsm_WiFi_state_Roaming                 fsm_WiFi_state_Roaming_imp;
fsm_WiFi_state_ConnectingAsAP          fsm_WiFi_state_ConnectingAsAP_imp;
fsm_WiFi_state_ConnectedToSta          fsm_WiFi_state_ConnectedToSta_imp;
fsm_WiFi_state_ConnectionFailed        fsm_WiFi_state_ConnectionFailed_imp;
fsm_WiFi_state_Disabled                fsm_WiFi_state_Disabled_imp;

//-----------------------------------------------------------------------------
///< Start up the driver and put it into a safe mode
c_WiFiDriver::c_WiFiDriver ()
{
    fsm_WiFi_state_Boot_imp.SetParent (this);
    fsm_WiFi_state_ConnectingUsingConfig_imp.SetParent (this);
    fsm_WiFi_state_ConnectingUsingDefaults_imp.SetParent (this);
    fsm_WiFi_state_ConnectedToAP_imp.SetParent (this);
    fsm_WiFi_state_Roaming_imp.SetParent (this);
    fsm_WiFi_state_ConnectingAsAP_imp.SetParent (this);
    fsm_WiFi_state_ConnectedToSta_imp.SetParent (this);
    fsm_WiFi_state_ConnectionFailed_imp.SetParent (this);
    fsm_WiFi_state_Disabled_imp.SetParent (this);

    fsm_WiFi_state_Boot_imp.Init ();
} // c_WiFiDriver

//-----------------------------------------------------------------------------
///< deallocate any resources and put the output channels into a safe state
c_WiFiDriver::~c_WiFiDriver()
{
    // DEBUG_START;

    // DEBUG_END;

} // ~c_WiFiDriver

//-----------------------------------------------------------------------------
void c_WiFiDriver::AnnounceState ()
{
    // DEBUG_START;

    String StateName;
    pCurrentFsmState->GetStateName (StateName);
    logcon (String (F ("WiFi Entering State: ")) + StateName);

    // DEBUG_END;

} // AnnounceState

//-----------------------------------------------------------------------------
///< Start the module
void c_WiFiDriver::Begin ()
{
    // DEBUG_START;

    // save the pointer to the config
    // config = NewConfig;

    // DEBUG_V ("      default_ssid: '" + default_ssid + "'");
    // DEBUG_V ("default_passphrase: '" + default_passphrase + "'");

    if (FileMgr.SdCardIsInstalled())
    {
        JsonDocument jsonConfigDoc;
        jsonConfigDoc.to<JsonObject>();
        // DEBUG_V ("read the sdcard config");
        if (FileMgr.ReadSdFile (F("wificonfig.json"), jsonConfigDoc))
        {
            // DEBUG_V ("Process the sdcard config");
            JsonObject jsonConfig = jsonConfigDoc.as<JsonObject> ();
            ApCredentials.SetConfig(jsonConfig);

            // copy the fields of interest into the local structure
            setFromJSON (ap_ssid,       jsonConfig, CN_ap_ssid);
            setFromJSON (ap_passphrase, jsonConfig, CN_ap_passphrase);

            ConfigSaveNeeded = true;

            FileMgr.DeleteSdFile (F ("wificonfig.json"));
        }
        else
        {
            // DEBUG_V ("ERROR: Could not read SD card config");
        }
    }

    // Disable persistant credential storage and configure SDK params
    WiFi.persistent (false);

    if(String(ap_ssid).isEmpty())
    {
        String Hostname;
        NetworkMgr.GetHostname (Hostname);
        SafeStrncpy(ap_ssid, (String(F("ESPixelStick-AP-")) + Hostname).c_str(), sizeof(ap_ssid));
    }

#ifdef ARDUINO_ARCH_ESP8266
    wifi_set_sleep_type (NONE_SLEEP_T);
    // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html#setoutputpower
    // https://github.com/esp8266/Arduino/issues/6366
    // AI Thinker FCC certification performed at 17dBm
    WiFi.setOutputPower (16);

#elif defined(ARDUINO_ARCH_ESP32)
    esp_wifi_set_ps (WIFI_PS_NONE);
#endif

    // DEBUG_V ("");

    // Setup WiFi Handlers
#ifdef ARDUINO_ARCH_ESP8266
    wifiConnectHandler    = WiFi.onStationModeGotIP        ([this](const WiFiEventStationModeGotIP& event) {this->onWiFiConnect (event); });
    wifiDisconnectHandler = WiFi.onStationModeDisconnected ([this](const WiFiEventStationModeDisconnected& event) {this->onWiFiDisconnect (event); });
#else
    WiFi.onEvent ([this](WiFiEvent_t event, arduino_event_info_t info) {this->onWiFiStaConn (event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent ([this](WiFiEvent_t event, arduino_event_info_t info) {this->onWiFiStaDisc (event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFi.onEvent ([this](WiFiEvent_t event, arduino_event_info_t info) {this->onWiFiConnect    (event, info);}, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent ([this](WiFiEvent_t event, arduino_event_info_t info) {this->onWiFiDisconnect (event, info);}, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif

    // set up the poll interval
    NextPoll.StartTimer(PollInterval, false);

    // Main loop should start polling for us
    // pCurrentFsmState->Poll ();

    // DEBUG_END;

} // begin

//-----------------------------------------------------------------------------
void c_WiFiDriver::connectWifi (const String & current_ssid, const String & current_passphrase)
{
    // DEBUG_START;

    do // once
    {
        // WiFi reset flag is set which will be handled in the next iteration of the main loop.
        // Ignore connect request to lessen boot spam.
        if (ResetWiFi)
        {
            // DEBUG_V ("WiFi Reset Requested");
            break;
        }

        SetUpIp();

        String Hostname;
        NetworkMgr.GetHostname(Hostname);
        // DEBUG_V (String("Hostname: ") + Hostname);

        // Hostname must be set after the mode on ESP8266 and before on ESP32
#ifdef ARDUINO_ARCH_ESP8266
        WiFi.disconnect();
        // DEBUG_V ("");

        // Switch to station mode
        WiFi.mode(WIFI_STA);
        // DEBUG_V ("");

        if (0 != Hostname.length())
        {
            // DEBUG_V (String ("Setting WiFi hostname: ") + Hostname);
            WiFi.hostname(Hostname);
        }
        // DEBUG_V ("");
#else
        WiFi.persistent(false);
        // DEBUG_V ("");
        WiFi.disconnect(true);
        // DEBUG_V ("");

        if (0 != Hostname.length())
        {
            // DEBUG_V (String("Setting WiFi hostname: ") + Hostname);
            WiFi.hostname(Hostname);
        }

        // DEBUG_V ("Setting WiFi Mode to STA");
        WiFi.enableAP(false);
        // DEBUG_V ();
        WiFi.enableSTA(true);

#endif
        // DEBUG_V (String ("      ssid: ") + current_ssid);
        // DEBUG_V (String ("passphrase: ") + current_passphrase);
        // DEBUG_V (String ("  hostname: ") + Hostname);

    	logcon (String(F ("Connecting to '")) +
               current_ssid +
               String(F("' as ")) +
               Hostname);

        WiFi.setSleep(false);
        // DEBUG_V ("");

        // remember the credentials for the scan completion and for roaming
        SafeStrncpy (CurrentSsid,       current_ssid.c_str (),       sizeof (CurrentSsid));
        SafeStrncpy (CurrentPassphrase, current_passphrase.c_str (), sizeof (CurrentPassphrase));
        ConnectionIsPinned = false;

#ifdef ARDUINO_ARCH_ESP8266
        // The SDK connects to the first AP it hears advertising the SSID, which is not
        // necessarily the closest one when several APs share the SSID. Scan for the SSID
        // first and connect to the AP with the strongest signal (see issue #806).
        fsm_WiFi_state * StateAtStart = pCurrentFsmState;
        StartApScan (false);
        if (!ApScanInProgress && (StateAtStart == pCurrentFsmState))
        {
            // DEBUG_V ("Could not start a scan. Connect using the SSID only");
            WiFi.begin(current_ssid.c_str(), current_passphrase.c_str());
        }
#else
        // Scan all channels and connect to the AP with the strongest signal instead
        // of the first AP found advertising the SSID (see issue #806).
        WiFi.setScanMethod (WIFI_ALL_CHANNEL_SCAN);
        WiFi.setSortMethod (WIFI_CONNECT_AP_BY_SIGNAL);
        WiFi.begin(current_ssid.c_str(), current_passphrase.c_str());
#endif // def ARDUINO_ARCH_ESP8266
    } while (false);

    // DEBUG_END;

} // connectWifi

//-----------------------------------------------------------------------------
// Start an asynchronous scan for the APs that advertise CurrentSsid. The flags are
// set before the call: scanNetworks yields to the SDK, and a state change during
// that yield (SetFsmState) must be able to cancel the scan.
void c_WiFiDriver::StartApScan (bool IsRoamCheck)
{
    // DEBUG_START;

    ScanIsRoamCheck  = IsRoamCheck;
    ApScanInProgress = true;
    ApScanTimer.StartTimer (WIFI_AP_SCAN_TIMEOUT_MS, false);

#ifdef ARDUINO_ARCH_ESP8266
    int8_t  ScanResult = WiFi.scanNetworks (true, false, 0, reinterpret_cast<uint8_t*>(CurrentSsid));
#else
    int16_t ScanResult = WiFi.scanNetworks (true, false, false, 300, 0, CurrentSsid);
#endif // def ARDUINO_ARCH_ESP8266

    if (WIFI_SCAN_FAILED == ScanResult)
    {
        ApScanInProgress = false;
        ScanIsRoamCheck  = false;
    }

    // DEBUG_END;
} // StartApScan

//-----------------------------------------------------------------------------
// Called from Poll while a scan for CurrentSsid is in progress. Once the scan
// completes, connect to the AP with the strongest signal (initial connection on
// the ESP8266) or move to it if it is clearly stronger than the current AP (roam check).
void c_WiFiDriver::ProcessApScanResults ()
{
    /// DEBUG_START;

    do // once
    {
        if (!ApScanInProgress)
        {
            break;
        }

        if (ResetWiFi)
        {
            // the pending reset starts over, including a new scan
            ApScanInProgress = false;
            ScanIsRoamCheck  = false;
            break;
        }

        int16_t NumNetworksFound = WiFi.scanComplete ();
        if ((WIFI_SCAN_RUNNING == NumNetworksFound) && !ApScanTimer.IsExpired ())
        {
            // still waiting for the scan to finish
            break;
        }
        ApScanInProgress = false;
        bool IsRoamCheck = ScanIsRoamCheck;
        ScanIsRoamCheck  = false;

        // While connected, the AP we are using is not a candidate
        uint8_t * CurrentBssid = (IsRoamCheck) ? WiFi.BSSID () : nullptr;
        int32_t   CurrentRssi  = WiFi.RSSI ();
        uint32_t  OtherApCount = 0;
        bool      FoundAp      = false;
        int32_t   BestRssi     = 0;
        int32_t   BestChannel  = 0;
        uint8_t   BestBssid[6];
        String    BestBssidStr;
        String    Line;
        Line.reserve (64);

        for (int16_t index = 0; index < NumNetworksFound; ++index)
        {
            if (!WiFi.SSID (index).equals (CurrentSsid))
            {
                continue;
            }

            Line  = F ("Found AP ");
            Line += WiFi.BSSIDstr (index);
            Line += F (" on channel ");
            Line += WiFi.channel (index);
            Line += F (" with RSSI ");
            Line += WiFi.RSSI (index);
            logcon (Line);

            if ((nullptr != CurrentBssid) && (0 == memcmp (CurrentBssid, WiFi.BSSID (index), sizeof (BestBssid))))
            {
                // compare the candidates against the scan reading of the current AP
                CurrentRssi = WiFi.RSSI (index);
                continue;
            }

            ++OtherApCount;
            if (!FoundAp || (WiFi.RSSI (index) > BestRssi))
            {
                FoundAp      = true;
                BestRssi     = WiFi.RSSI (index);
                BestChannel  = WiFi.channel (index);
                BestBssidStr = WiFi.BSSIDstr (index);
                memcpy (BestBssid, WiFi.BSSID (index), sizeof (BestBssid));
            }
        }
        WiFi.scanDelete ();

        if (IsRoamCheck)
        {
            if (FoundAp && (BestRssi >= (CurrentRssi + WIFI_ROAM_MIN_IMPROVEMENT)))
            {
                Line  = F ("Moving to AP ");
                Line += BestBssidStr;
                Line += F (" (");
                Line += BestRssi;
                Line += F (" dBm) from AP ");
                Line += WiFi.BSSIDstr ();
                Line += F (" (");
                Line += CurrentRssi;
                Line += F (" dBm)");
                logcon (Line);
                memcpy (RoamTargetBssid, BestBssid, sizeof (RoamTargetBssid));
                RoamTargetChannel = BestChannel;
                fsm_WiFi_state_Roaming_imp.Init ();
            }
            else if ((0 == OtherApCount) && (NumNetworksFound >= 0))
            {
                // Nothing to move to. The set of APs does not change while we stay connected.
                logcon (F ("No other AP advertises the SSID. Roam checks stop until the next connection"));
                RoamChecksAllowed = false;
            }
            else
            {
                // Check again later, waiting longer each time the result is the same
                logcon (F ("No stronger AP found. Staying on the current AP"));
                RoamCheckTimer.StartTimer (RoamCheckWaitMs, false);
                RoamCheckWaitMs = RoamCheckWaitMs * 2;
                if (RoamCheckWaitMs > WIFI_ROAM_MAX_RETRY_TIME_MS)
                {
                    RoamCheckWaitMs = WIFI_ROAM_MAX_RETRY_TIME_MS;
                }
            }
            break;
        }

        // Initial connection. The ESP32 SDK picks the AP itself, so only the ESP8266 gets here.
        if (FoundAp)
        {
            logcon (String (F ("Connecting to AP ")) + BestBssidStr);
            ConnectionIsPinned = true;
            WiFi.begin (CurrentSsid, CurrentPassphrase, BestChannel, BestBssid);
        }
        else
        {
            logcon (F ("Scan did not find the SSID. Connecting using the SSID only"));
            WiFi.begin (CurrentSsid, CurrentPassphrase);
        }
    } while (false);

    /// DEBUG_END;
} // ProcessApScanResults

//-----------------------------------------------------------------------------
// Called once a second while connected. After the signal has been weak for a
// while, scan for a stronger AP that advertises the same SSID.
void c_WiFiDriver::CheckSignalStrength ()
{
    /// DEBUG_START;

    do // once
    {
        if (!RoamChecksAllowed || ApScanInProgress || !RoamCheckTimer.IsExpired ())
        {
            break;
        }

        int32_t CurrentRssi = WiFi.RSSI ();
        if (CurrentRssi >= WIFI_ROAM_RSSI_THRESHOLD)
        {
            WeakSignalCount = 0;
            break;
        }

        if (++WeakSignalCount < WIFI_ROAM_WEAK_POLLS)
        {
            break;
        }
        WeakSignalCount = 0;

        String Line;
        Line.reserve (64);
        Line  = F ("Weak signal (");
        Line += CurrentRssi;
        Line += F (" dBm). Scanning for a stronger AP");
        logcon (Line);

        StartApScan (true);
        if (!ApScanInProgress)
        {
            // DEBUG_V ("Could not start a scan. Try again later");
            RoamCheckTimer.StartTimer (RoamCheckWaitMs, false);
        }
    } while (false);

    /// DEBUG_END;
} // CheckSignalStrength

//-----------------------------------------------------------------------------
// A connection attempt to a specific AP timed out. Try once more the old way,
// letting the SDK pick any AP that advertises the SSID.
bool c_WiFiDriver::RetryWithoutPinnedAp ()
{
    // DEBUG_START;

    bool Retrying = false;
    if (ConnectionIsPinned)
    {
        ConnectionIsPinned = false;
        logcon (F ("The selected AP did not answer. Connecting using the SSID only"));
        FsmTimer.StartTimer (MilliSecondsInASecond * sta_timeout, false);
        WiFi.disconnect ();
        WiFi.begin (CurrentSsid, CurrentPassphrase);
        Retrying = true;
    }

    // DEBUG_END;
    return Retrying;
} // RetryWithoutPinnedAp

//-----------------------------------------------------------------------------
void c_WiFiDriver::Disable ()
{
    // DEBUG_START;

    if (pCurrentFsmState != &fsm_WiFi_state_Disabled_imp)
    {
        // DEBUG_V ();
        WiFi.enableSTA (false);
        WiFi.enableAP (false);
        fsm_WiFi_state_Disabled_imp.Init ();
    }

    // DEBUG_END;

} // Disable

//-----------------------------------------------------------------------------
void c_WiFiDriver::Enable ()
{
    // DEBUG_START;

    if (pCurrentFsmState == &fsm_WiFi_state_Disabled_imp)
    {
        // DEBUG_V ();
        WiFi.enableSTA (true);
        WiFi.enableAP (false);
        fsm_WiFi_state_ConnectionFailed_imp.Init ();
    }
    else
    {
        // DEBUG_V (String ("WiFi is not disabled"));
    }

    // DEBUG_END;
} // Enable

//-----------------------------------------------------------------------------
void c_WiFiDriver::GetConfig (JsonObject& json)
{
    // DEBUG_START;

    ApCredentials.GetConfig(json);
    JsonWrite(json, CN_ap_ssid,       ap_ssid);
    JsonWrite(json, CN_ap_passphrase, ap_passphrase);

#ifdef ARDUINO_ARCH_ESP8266
    IPAddress Temp = ip;
    JsonWrite(json, CN_ip,           Temp.toString ());
    Temp = netmask;
    JsonWrite(json, CN_netmask,      Temp.toString ());
    Temp = gateway;
    JsonWrite(json, CN_gateway,      Temp.toString ());
    Temp = primaryDns;
    JsonWrite(json, CN_dnsp,         Temp.toString ());
    Temp = secondaryDns;
    JsonWrite(json, CN_dnss,         Temp.toString ());
#else
    JsonWrite(json, CN_ip,           ip.toString ());
    JsonWrite(json, CN_netmask,      netmask.toString ());
    JsonWrite(json, CN_gateway,      gateway.toString ());
    JsonWrite(json, CN_dnsp,         primaryDns.toString ());
    JsonWrite(json, CN_dnss,         secondaryDns.toString ());
#endif // !def ARDUINO_ARCH_ESP8266

    JsonWrite(json, CN_StayInApMode, StayInApMode);
    JsonWrite(json, CN_dhcp,         UseDhcp);
    JsonWrite(json, CN_sta_timeout,  sta_timeout);
    JsonWrite(json, CN_ap_channel,   ap_channelNumber);
    JsonWrite(json, CN_ap_fallback,  ap_fallbackIsEnabled);
    JsonWrite(json, CN_ap_timeout,   ap_timeout);
    JsonWrite(json, CN_ap_reboot,    RebootOnWiFiFailureToConnect);

    // PrettyPrint(json, "WiFi Config");

    // DEBUG_END;

} // GetConfig

//-----------------------------------------------------------------------------
void c_WiFiDriver::GetHostname (String & name)
{
#ifdef ARDUINO_ARCH_ESP8266
    name = WiFi.hostname ();
#else
    name = WiFi.getHostname ();
#endif // def ARDUINO_ARCH_ESP8266

} // GetWiFiHostName

//-----------------------------------------------------------------------------
void c_WiFiDriver::GetStatus (JsonObject& jsonStatus)
{
    // DEBUG_START;
    String Hostname;
    GetHostname (Hostname);
    JsonWrite(jsonStatus, CN_hostname,  Hostname);
    JsonWrite(jsonStatus, CN_rssi,      WiFi.RSSI ());
    JsonWrite(jsonStatus, CN_ip,        getIpAddress ().toString ());
    JsonWrite(jsonStatus, CN_subnet,    getIpSubNetMask ().toString ());
    JsonWrite(jsonStatus, CN_mac,       WiFi.macAddress ());
    JsonWrite(jsonStatus, CN_ssid,      WiFi.SSID ());
    JsonWrite(jsonStatus, CN_bssid,     IsWiFiConnected () ? WiFi.BSSIDstr () : emptyString);
    JsonWrite(jsonStatus, CN_connected, IsWiFiConnected ());

    // DEBUG_END;
} // GetStatus

//-----------------------------------------------------------------------------
#ifdef ARDUINO_ARCH_ESP32

//-----------------------------------------------------------------------------
void c_WiFiDriver::onWiFiStaConn (const WiFiEvent_t event, const WiFiEventInfo_t info)
{
    // DEBUG_V ("ESP has associated with the AP");
} // onWiFiStaConn

//-----------------------------------------------------------------------------
void c_WiFiDriver::onWiFiStaDisc (const WiFiEvent_t event, const WiFiEventInfo_t info)
{
    // DEBUG_V ("ESP has disconnected from the AP");
} // onWiFiStaDisc

#endif // def ARDUINO_ARCH_ESP32

//-----------------------------------------------------------------------------
#ifdef ARDUINO_ARCH_ESP8266
void c_WiFiDriver::onWiFiConnect (const WiFiEventStationModeGotIP& event)
{
#else
void c_WiFiDriver::onWiFiConnect (const WiFiEvent_t event, const WiFiEventInfo_t info)
{
#endif
    // DEBUG_START;

    pCurrentFsmState->OnConnect ();

    // DEBUG_END;
} // onWiFiConnect

//-----------------------------------------------------------------------------
/// WiFi Disconnect Handler
#ifdef ARDUINO_ARCH_ESP8266
/** Attempt to re-connect every 2 seconds */
void c_WiFiDriver::onWiFiDisconnect (const WiFiEventStationModeDisconnected & event)
{
#else
void c_WiFiDriver::onWiFiDisconnect (const WiFiEvent_t event, const WiFiEventInfo_t info)
{
#endif
    // DEBUG_START;

    pCurrentFsmState->OnDisconnect ();

    // DEBUG_END;

} // onWiFiDisconnect

//-----------------------------------------------------------------------------
void c_WiFiDriver::Poll ()
{
    // DEBUG_START;

    ProcessApScanResults ();

    if (NextPoll.IsExpired())
    {
        // DEBUG_V ("Start Poll");
        NextPoll.StartTimer(PollInterval, false);
        // displayFsmState ();
        pCurrentFsmState->Poll ();
        // displayFsmState ();
        // DEBUG_V ("End Poll");
    }

    if (ResetWiFi)
    {
        ResetWiFi = false;
        reset ();
    }

    // DEBUG_END;

} // Poll

//-----------------------------------------------------------------------------
void c_WiFiDriver::reset ()
{
    // DEBUG_START;

    // Reset address in case we're switching from static to dhcp
    // WiFi.config (0u, 0u, 0u);
    // DEBUG_V ("");

    if (IsWiFiConnected ())
    {
        // DEBUG_V ("");
        NetworkMgr.SetWiFiIsConnected (false);
    }
    // DEBUG_V ("");

    fsm_WiFi_state_Boot_imp.Init ();

    // DEBUG_END;
} // reset

//-----------------------------------------------------------------------------
bool c_WiFiDriver::SetConfig (JsonObject & json)
{
    // DEBUG_START;

    // DEBUG_V (String("ap_channelNumber: ") + String(ap_channelNumber));
    bool ConfigChanged = false;

    String sIp = ip.toString ();
    String sGateway = gateway.toString ();
    String sNetmask = netmask.toString ();
    String sdnsp = primaryDns.toString ();
    String sdnss = secondaryDns.toString ();

    ConfigChanged |= ApCredentials.SetConfig(json);

    ConfigChanged |= setFromJSON (sIp, json, CN_ip);
    ConfigChanged |= setFromJSON (sNetmask, json, CN_netmask);
    ConfigChanged |= setFromJSON (sGateway, json, CN_gateway);
    ConfigChanged |= setFromJSON (sdnsp, json, CN_dnsp);
    ConfigChanged |= setFromJSON (sdnss, json, CN_dnss);
    ConfigChanged |= setFromJSON (UseDhcp, json, CN_dhcp);
    ConfigChanged |= setFromJSON (ap_ssid, json, CN_ap_ssid);
    ConfigChanged |= setFromJSON (ap_passphrase, json, CN_ap_passphrase);
    ConfigChanged |= setFromJSON (ap_channelNumber, json, CN_ap_channel);
    ConfigChanged |= setFromJSON (sta_timeout, json, CN_sta_timeout);
    ConfigChanged |= setFromJSON (ap_fallbackIsEnabled, json, CN_ap_fallback);
    ConfigChanged |= setFromJSON (ap_timeout, json, CN_ap_timeout);
    ConfigChanged |= setFromJSON (RebootOnWiFiFailureToConnect, json, CN_ap_reboot);
    ConfigChanged |= setFromJSON (StayInApMode, json, CN_StayInApMode);

    // DEBUG_V ("                     ip: " + ip);
    // DEBUG_V (                "gateway: " + gateway);
    // DEBUG_V (                "netmask: " + netmask);
    // DEBUG_V (String("ap_channelNumber: ") + String(ap_channelNumber));
    // String StateName;
    // pCurrentFsmState->GetStateName(StateName);
    // DEBUG_V (String("       CurrState: ") + StateName);

    ip.fromString (sIp);
    gateway.fromString (sGateway);
    netmask.fromString (sNetmask);
    primaryDns.fromString (sdnsp);
    secondaryDns.fromString (sdnss);

    if(ConfigChanged)
    {
        // DEBUG_V ("WiFi Settings changed");
        if(pCurrentFsmState == &fsm_WiFi_state_ConnectedToAP_imp)
        {
            // DEBUG_V ("need to cycle the WiFi to move to new STA settings");
            WiFi.disconnect();
        }
        else if(pCurrentFsmState == &fsm_WiFi_state_Roaming_imp)
        {
            // apply the new settings with a fresh connection once the main loop runs
            ResetWiFi = true;
        }
        else if(pCurrentFsmState == &fsm_WiFi_state_ConnectedToSta_imp)
        {
            // DEBUG_V ("need to cycle the WiFi to move to new AP settings");
            WiFi.softAPdisconnect();
        }
    }

    // DEBUG_V (String("ConfigChanged: ") + String(ConfigChanged));
    // DEBUG_END;
    return ConfigChanged;

} // SetConfig

//-----------------------------------------------------------------------------
void c_WiFiDriver::SetFsmState (fsm_WiFi_state* NewState)
{
    // DEBUG_START;
    // DEBUG_V (String ("pCurrentFsmState: ") + String (uint32_t (pCurrentFsmState), HEX));
    // DEBUG_V (String ("        NewState: ") + String (uint32_t (NewState), HEX));
    pCurrentFsmState = NewState;
    // A state change makes the result of any scan still in flight irrelevant
    ApScanInProgress = false;
    ScanIsRoamCheck  = false;
    // displayFsmState ();
    // DEBUG_END;

} // SetFsmState

//-----------------------------------------------------------------------------
void c_WiFiDriver::SetHostname (String & )
{
    // DEBUG_START;

    // Need to reset the WiFi sub system if the host name changes
    reset ();

    // DEBUG_END;
} // SetHostname

//-----------------------------------------------------------------------------
void c_WiFiDriver::SetUpIp ()
{
    // DEBUG_START;

    do // once
    {
        if (true == UseDhcp)
        {
            logcon (F ("Using DHCP"));
            break;
        }

        IPAddress temp = (uint32_t)0;
        // DEBUG_V ("   temp: " + temp.toString ());
        // DEBUG_V ("     ip: " + ip.toString());
        // DEBUG_V ("netmask: " + netmask.toString ());
        // DEBUG_V ("gateway: " + gateway.toString ());

        if (temp == ip)
        {
            logcon (F ("ERROR: STATIC SELECTED WITHOUT IP. Using DHCP assigned address"));
            break;
        }

        if ((ip == WiFi.localIP ()) &&
            (netmask == WiFi.subnetMask ()) &&
            (gateway == WiFi.gatewayIP ()))
        {
            // DEBUG_V ();
            // correct IP is already set
            break;
        }

        // use gateway if primary DNS is not defined
        if(primaryDns == INADDR_NONE)
        {
            primaryDns = gateway;
        }

        // We didn't use DNS, so just set it to our configured gateway
        WiFi.config (ip, gateway, netmask, primaryDns, secondaryDns);

        logcon (F ("Using Static IP"));

    } while (false);

    // DEBUG_END;

} // SetUpIp

//-----------------------------------------------------------------------------
int c_WiFiDriver::ValidateConfig ()
{
    // DEBUG_START;

    int response = 0;

    if (sta_timeout < 5)
    {
        sta_timeout = CLIENT_TIMEOUT;
        // DEBUG_V ();
        response++;
    }

    if (ap_timeout < 15)
    {
        ap_timeout = AP_TIMEOUT;
        // DEBUG_V ();
        response++;
    }

    // DEBUG_END;

    return response;

} // ValidateConfig

/*****************************************************************************/
//  FSM Code
/*****************************************************************************/
/*****************************************************************************/
// Waiting for polling to start
void fsm_WiFi_state_Boot::Poll ()
{
    /// DEBUG_START;

    // Start trying to connect to the AP
    /// DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));
    pWiFiDriver->ApCredentials.ResetCurrentCredentials();
    fsm_WiFi_state_ConnectingUsingConfig_imp.Init ();
    // pWiFiDriver->displayFsmState ();

    /// DEBUG_END;
} // fsm_WiFi_state_boot

/*****************************************************************************/
// Waiting for polling to start
void fsm_WiFi_state_Boot::Init ()
{
    // DEBUG_START;

    // DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));
    pWiFiDriver->SetFsmState (this);

    // This can get called before the system is up and running.
    // No log port available yet
    // pWiFiDriver->AnnounceState ();

    // DEBUG_END;
} // fsm_WiFi_state_Boot::Init

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingUsingConfig::Poll ()
{
    /// DEBUG_START;

    // wait for the connection to complete via the callback function

    if (WiFi.status () != WL_CONNECTED)
    {
        if (pWiFiDriver->GetFsmTimer().IsExpired())
        {
            /// DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));
            if (!pWiFiDriver->RetryWithoutPinnedAp ())
            {
                logcon (F ("WiFi Failed to connect using Configured Credentials"));
                // DEBUG_V ("Try the next SSID");
                pWiFiDriver->ApCredentials.GetNextCredentials();
                Init ();
            }
        }
    }

    /// DEBUG_END;
} // fsm_WiFi_state_ConnectingUsingConfig::Poll

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingUsingConfig::Init ()
{
    // DEBUG_START;
    c_ApCredentials::ApCredentials credentials;

    do
    {
        if(!pWiFiDriver->ApCredentials.GetCurrentCredentials(credentials))
        {
            // DEBUG_V ("No additional SSIDs to try");
            pWiFiDriver->ApCredentials.ResetCurrentCredentials();
            fsm_WiFi_state_ConnectingUsingDefaults_imp.Init ();
            break;
        }

        String ssid = String(credentials.ssid);
        String pw = String(credentials.passphrase);

        // DEBUG_V (String("ssid: ") + ssid);
        // DEBUG_V (String("  pw: ") + pw);

        if (!ssid.isEmpty())
        {
            // DEBUG_V ("Try this ssid");
            pWiFiDriver->SetFsmState (this);
            pWiFiDriver->AnnounceState ();
            pWiFiDriver->GetFsmTimer().StartTimer(MilliSecondsInASecond * pWiFiDriver->Get_sta_timeout(), false);

            pWiFiDriver->connectWifi (ssid, pw);
            break;
        }

        // DEBUG_V ("Try next ssid");
        pWiFiDriver->ApCredentials.GetNextCredentials();

    } while(true);

    // pWiFiDriver->displayFsmState ();

    // DEBUG_END;

} // fsm_WiFi_state_ConnectingUsingConfig::Init

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingUsingConfig::OnConnect ()
{
    // DEBUG_START;

    fsm_WiFi_state_ConnectedToAP_imp.Init ();

    // DEBUG_END;

} // fsm_WiFi_state_ConnectingUsingConfig::OnConnect

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingUsingDefaults::Poll ()
{
    /// DEBUG_START;

    // wait for the connection to complete via the callback function
    if (WiFi.status () != WL_CONNECTED)
    {
        if (pWiFiDriver->GetFsmTimer().IsExpired())
        {
            // DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));
            if (!pWiFiDriver->RetryWithoutPinnedAp ())
            {
                logcon (F ("WiFi Failed to connect using default Credentials"));
                fsm_WiFi_state_ConnectingAsAP_imp.Init ();
            }
        }
    }

    /// DEBUG_END;
} // fsm_WiFi_state_ConnectingUsingDefaults::Poll

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingUsingDefaults::Init ()
{
    // DEBUG_START;

    // DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));

    if(!default_ssid.equals(DEFAULT_SSID_NOT_SET))
    {
        pWiFiDriver->SetFsmState (this);
        pWiFiDriver->AnnounceState ();
        pWiFiDriver->GetFsmTimer().StartTimer(MilliSecondsInASecond * pWiFiDriver->Get_sta_timeout (), false);
        // DEBUG_V ("      default_ssid: '" + default_ssid + "'");
        // DEBUG_V ("default_passphrase: '" + default_passphrase + "'");
        pWiFiDriver->connectWifi (default_ssid, default_passphrase);
        // DEBUG_V ("      default_ssid: '" + default_ssid + "'");
        // DEBUG_V ("default_passphrase: '" + default_passphrase + "'");
    }
    else
    {
        // no defauult ssid was set at compile time. Just move on to AP mode
        fsm_WiFi_state_ConnectingAsAP_imp.Init ();
    }
    // pWiFiDriver->displayFsmState ();

    // DEBUG_END;
} // fsm_WiFi_state_ConnectingUsingDefaults::Init

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingUsingDefaults::OnConnect ()
{
    // DEBUG_START;

    // DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));
    fsm_WiFi_state_ConnectedToAP_imp.Init ();

    // DEBUG_END;

} // fsm_WiFi_state_ConnectingUsingDefaults::OnConnect

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingAsAP::Poll ()
{
    /// DEBUG_START;

    if (0 != WiFi.softAPgetStationNum ())
    {
        fsm_WiFi_state_ConnectedToSta_imp.Init();
    }
    else
    {
        if (pWiFiDriver->GetFsmTimer().IsExpired())
        {
            if( false == pWiFiDriver->Get_ap_StayInApMode())
            {
                logcon (F ("WiFi STA Failed to connect"));
                fsm_WiFi_state_ConnectionFailed_imp.Init ();
            }
        }
    }

    /// DEBUG_END;
} // fsm_WiFi_state_ConnectingAsAP::Poll

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingAsAP::Init ()
{
    // DEBUG_START;

    pWiFiDriver->SetFsmState (this);
    pWiFiDriver->AnnounceState ();
    pWiFiDriver->GetFsmTimer ().StartTimer(MilliSecondsInASecond * pWiFiDriver->Get_ap_timeout (), false);

    if (true == pWiFiDriver->Get_ap_fallbackIsEnabled() || pWiFiDriver->Get_ap_StayInApMode())
    {
        WiFi.enableSTA(false);
        WiFi.enableAP(true);
        if(String(pWiFiDriver->ap_ssid).isEmpty())
        {
            String Hostname;
            NetworkMgr.GetHostname (Hostname);
            SafeStrncpy(pWiFiDriver->ap_ssid, (String(F("ESPixelStick-")) + Hostname).c_str(), sizeof(pWiFiDriver->ap_ssid));
        }
        // DEBUG_V (String("ap_channelNumber: ") + String(pWiFiDriver->ap_channelNumber));
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        WiFi.softAP (pWiFiDriver->ap_ssid, pWiFiDriver->ap_passphrase, int(pWiFiDriver->ap_channelNumber));

        pWiFiDriver->setIpAddress (IPAddress(192,168,4,1));
        pWiFiDriver->setIpSubNetMask (WiFi.subnetMask ());

        logcon (String (F ("WiFi SOFTAP:       ssid: '")) + String(pWiFiDriver->ap_ssid));
        logcon (String (F ("WiFi SOFTAP: IP Address: '")) + pWiFiDriver->getIpAddress ().toString ());
    }
    else
    {
        logcon (String (F ("WiFi SOFTAP: Not enabled")));
        fsm_WiFi_state_ConnectionFailed_imp.Init ();
    }

    // DEBUG_END;

} // fsm_WiFi_state_ConnectingAsAP::Init

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectingAsAP::OnConnect ()
{
    // DEBUG_START;

    fsm_WiFi_state_ConnectedToSta_imp.Init ();

    // DEBUG_END;

} // fsm_WiFi_state_ConnectingAsAP::OnConnect

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectedToAP::Poll ()
{
    /// DEBUG_START;

    // did we get silently disconnected?
    if (WiFi.status () != WL_CONNECTED)
    {
        // DEBUG_V ("WiFi Handle Silent Disconnect");
        WiFi.reconnect ();
    }
    else
    {
        pWiFiDriver->CheckSignalStrength ();
    }

    /// DEBUG_END;
} // fsm_WiFi_state_ConnectedToAP::Poll

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectedToAP::Init ()
{
    // DEBUG_START;

    // DEBUG_V (String ("this: ") + String (uint32_t (this), HEX));
    pWiFiDriver->SetFsmState (this);
    pWiFiDriver->AnnounceState ();

    // WiFiDriver.SetUpIp ();

    pWiFiDriver->setIpAddress( WiFi.localIP () );
    pWiFiDriver->setIpSubNetMask( WiFi.subnetMask () );

    logcon (String (F ("Connected with IP: ")) + pWiFiDriver->getIpAddress ().toString ());

    // give the link time to settle before looking for a stronger AP
    pWiFiDriver->WeakSignalCount   = 0;
    pWiFiDriver->RoamChecksAllowed = true;
    pWiFiDriver->RoamCheckWaitMs   = WIFI_ROAM_RETRY_TIME_MS;
    pWiFiDriver->RoamCheckTimer.StartTimer (WIFI_ROAM_SETTLE_TIME_MS, false);

    pWiFiDriver->SetIsWiFiConnected (true);
    NetworkMgr.SetWiFiIsConnected (true);

    // DEBUG_END;
} // fsm_WiFi_state_ConnectedToAP::Init

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectedToAP::OnDisconnect ()
{
    // DEBUG_START;

    logcon (F ("WiFi Lost the connection to the AP"));
    fsm_WiFi_state_ConnectionFailed_imp.Init ();

    // DEBUG_END;

} // fsm_WiFi_state_ConnectedToAP::OnDisconnect

/*****************************************************************************/
/*****************************************************************************/
// Move to a stronger AP that advertises the same SSID
void fsm_WiFi_state_Roaming::Init ()
{
    // DEBUG_START;

    pWiFiDriver->SetFsmState (this);
    pWiFiDriver->AnnounceState ();
    pWiFiDriver->GetFsmTimer ().StartTimer (MilliSecondsInASecond * pWiFiDriver->Get_sta_timeout (), false);

    // the network services see this as a normal disconnect / reconnect cycle
    if (pWiFiDriver->IsWiFiConnected ())
    {
        pWiFiDriver->SetIsWiFiConnected (false);
        NetworkMgr.SetWiFiIsConnected (false);
    }

    pWiFiDriver->ConnectionIsPinned = true;
    WiFi.disconnect ();
    WiFi.begin (pWiFiDriver->CurrentSsid, pWiFiDriver->CurrentPassphrase, pWiFiDriver->RoamTargetChannel, pWiFiDriver->RoamTargetBssid);

    // DEBUG_END;
} // fsm_WiFi_state_Roaming::Init

/*****************************************************************************/
// Wait for the connection to the new AP
void fsm_WiFi_state_Roaming::Poll ()
{
    /// DEBUG_START;

    // wait for the connection to complete via the callback function
    if (WiFi.status () != WL_CONNECTED)
    {
        if (pWiFiDriver->GetFsmTimer ().IsExpired ())
        {
            if (!pWiFiDriver->RetryWithoutPinnedAp ())
            {
                logcon (F ("WiFi Failed to connect to the stronger AP"));
                fsm_WiFi_state_ConnectionFailed_imp.Init ();
            }
        }
    }

    /// DEBUG_END;
} // fsm_WiFi_state_Roaming::Poll

/*****************************************************************************/
void fsm_WiFi_state_Roaming::OnConnect ()
{
    // DEBUG_START;
    fsm_WiFi_state_ConnectedToAP_imp.Init ();
    // DEBUG_END;
} // fsm_WiFi_state_Roaming::OnConnect

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectedToSta::Poll ()
{
    /// DEBUG_START;

    // did we get silently disconnected?
    if (0 == WiFi.softAPgetStationNum ())
    {
        logcon (F ("WiFi Lost the connection to the STA"));
        if(pWiFiDriver->Get_ap_StayInApMode())
        {
            fsm_WiFi_state_ConnectingAsAP_imp.Init ();
        }
        else
        {
            fsm_WiFi_state_ConnectionFailed_imp.Init ();
        }
    }

    /// DEBUG_END;
} // fsm_WiFi_state_ConnectedToSta::Poll

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectedToSta::Init ()
{
    // DEBUG_START;

    pWiFiDriver->SetFsmState (this);
    pWiFiDriver->AnnounceState ();

    // WiFiDriver.SetUpIp ();

    pWiFiDriver->setIpAddress (WiFi.softAPIP ());
    pWiFiDriver->setIpSubNetMask (IPAddress (255, 255, 255, 0));

    logcon (String (F ("Connected to STA with IP: ")) + pWiFiDriver->getIpAddress ().toString ());

    pWiFiDriver->SetIsWiFiConnected (true);
    NetworkMgr.SetWiFiIsConnected (true);

    // DEBUG_END;
} // fsm_WiFi_state_ConnectedToSta::Init

/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectedToSta::OnDisconnect ()
{
    // DEBUG_START;

    logcon (F ("WiFi STA Disconnected"));
    if(pWiFiDriver->Get_ap_StayInApMode())
    {
        fsm_WiFi_state_ConnectingAsAP_imp.Init ();
    }
    else
    {
        fsm_WiFi_state_ConnectionFailed_imp.Init ();
    }

    // DEBUG_END;

} // fsm_WiFi_state_ConnectedToSta::OnDisconnect

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_ConnectionFailed::Init ()
{
    // DEBUG_START;

    pWiFiDriver->SetFsmState (this);
    pWiFiDriver->AnnounceState ();

    if (pWiFiDriver->IsWiFiConnected())
    {
        pWiFiDriver->SetIsWiFiConnected (false);
        NetworkMgr.SetWiFiIsConnected (false);
    }

    if (true == pWiFiDriver->Get_RebootOnWiFiFailureToConnect())
    {
        String Reason = (F ("WiFi Requesting Reboot"));
        RequestReboot(Reason, 100000);
    }
    else
    {
        // DEBUG_V ("WiFi Reboot Disabled. Try Again");

        // start over
        fsm_WiFi_state_Boot_imp.Init ();
    }

    // DEBUG_END;

} // fsm_WiFi_state_ConnectionFailed::Init

/*****************************************************************************/
/*****************************************************************************/
// Wait for events
void fsm_WiFi_state_Disabled::Init ()
{
    // DEBUG_START;

    pWiFiDriver->SetFsmState (this);
    pWiFiDriver->AnnounceState ();

    if (pWiFiDriver->IsWiFiConnected ())
    {
        pWiFiDriver->SetIsWiFiConnected (false);
        NetworkMgr.SetWiFiIsConnected (false);
    }

    // DEBUG_END;

} // fsm_WiFi_state_Disabled::Init

//-----------------------------------------------------------------------------
