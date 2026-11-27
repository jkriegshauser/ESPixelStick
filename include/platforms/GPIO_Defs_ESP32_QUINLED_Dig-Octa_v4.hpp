#pragma once
/*
 * GPIO_Defs_ESP32_QUINLED_Dig-Octa_v4.hpp - Output Management class
 *
 * Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
 * Copyright (c) 2021 - 2026 Shelby Merrick
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

// QuinLED Dig-Octa Brainboard-32-8L v4 (board marking "32-8L v4r1"). The v2 board keeps GPIO_Defs_ESP32_QUINLED_Dig-Octa.hpp.
// Pins per the QuinLED v4 pinout guide: LED1..LED8 on GPIO 1, 2, 3, 4, 5, 12, 13, 14; relay on GPIO 33; the micro SD card
// on CLK 15, MOSI 16, CS 17, MISO 36; the Ethernet PHY clock comes from the board's own oscillator into GPIO 0.
// GPIO 34 and 35 are the button inputs (input only), GPIO 15, 16, 17 and 36 are shared with the QEXP port, I2C and the PDM
// microphone, so there is no SPI or I2C output port here.

// Same FPP variant name as the v2 environment: xLights matches its controller definition on this string, and the v4 has the
// same eight pixel ports, relay and protocol list, so the existing "QUINLED_Dig-Octa" definition fits it.
 #define FPP_VARIANT_NAME "QUINLED_Dig-Octa"

//Output Manager
// MAX 8 Serial ports on ESP32
const OM_OutputPortDefinition_t OM_OutputPortDefinitions[] =
{
    {OM_PortId_t(0), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_1}},
    {OM_PortId_t(1), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_2}},
    {OM_PortId_t(2), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_3}},
    {OM_PortId_t(3), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_4}},
    {OM_PortId_t(4), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_5}},
    {OM_PortId_t(5), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_12}},
    {OM_PortId_t(6), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_13}},
    {OM_PortId_t(7), OM_PortType_t::OM_SERIAL, {gpio_num_t::GPIO_NUM_14}},
    {OM_PortId_t(8), OM_PortType_t::OM_RELAY,  {gpio_num_t::GPIO_NUM_33}}
};

// File Manager
#define SUPPORT_SD
#define SD_CARD_MISO_PIN        gpio_num_t::GPIO_NUM_36
#define SD_CARD_MOSI_PIN        gpio_num_t::GPIO_NUM_16
#define SD_CARD_CLK_PIN         gpio_num_t::GPIO_NUM_15
#define SD_CARD_CS_PIN          gpio_num_t::GPIO_NUM_17

#define SUPPORT_ETHERNET
#include <ETH.h>

/*
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
// The v4 has its own 50 MHz oscillator feeding GPIO0. GPIO17 is the SD card chip select on this board.
#define DEFAULT_ETH_CLK_MODE ETH_CLOCK_GPIO0_IN

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define DEFAULT_ETH_POWER_PIN          gpio_num_t(-1)
#define DEFAULT_ETH_POWER_PIN_ACTIVE   HIGH

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define DEFAULT_ETH_TYPE    ETH_PHY_LAN8720

// I2C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR_PHY_LAN8720    0
// #define ETH_ADDR_PHY_LAN8720    1
// #define ETH_ADDR_PHY_TLK110     31
#define DEFAULT_ETH_ADDR        ETH_ADDR_PHY_LAN8720
#define DEFAULT_ETH_TXEN        gpio_num_t::GPIO_NUM_21
#define DEFAULT_ETH_TXD0        gpio_num_t::GPIO_NUM_19
#define DEFAULT_ETH_TXD1        gpio_num_t::GPIO_NUM_22
#define DEFAULT_ETH_CRSDV       gpio_num_t::GPIO_NUM_27
#define DEFAULT_ETH_RXD0        gpio_num_t::GPIO_NUM_25
#define DEFAULT_ETH_RXD1        gpio_num_t::GPIO_NUM_26
#define DEFAULT_ETH_MDC_PIN     gpio_num_t::GPIO_NUM_23
#define DEFAULT_ETH_MDIO_PIN    gpio_num_t::GPIO_NUM_18

// Output Types
#define SUPPORT_OutputProtocol_TLS3001             // OM_SERIAL
// #define SUPPORT_OutputProtocol_APA102           // OM_SPI
#define SUPPORT_OutputProtocol_DMX                 // OM_SERIAL
#define SUPPORT_OutputProtocol_GECE                // OM_SERIAL
#define SUPPORT_OutputProtocol_GS8208              // OM_SERIAL
#define SUPPORT_OutputProtocol_Renard              // OM_SERIAL
#define SUPPORT_OutputProtocol_Serial              // OM_SERIAL
#define SUPPORT_OutputProtocol_TM1814              // OM_SERIAL
#define SUPPORT_OutputProtocol_UCS1903             // OM_SERIAL
#define SUPPORT_OutputProtocol_UCS8903             // OM_SERIAL
// #define SUPPORT_OutputProtocol_WS2801           // OM_SPI
#define SUPPORT_OutputProtocol_WS2811           // OM_SERIAL
#define SUPPORT_OutputProtocol_Relay               // OM_RELAY
// #define SUPPORT_OutputProtocol_Servo_PCA9685    // OM_I2C
#define SUPPORT_OutputProtocol_FireGod             // OM_SERIAL
// #define SUPPORT_OutputProtocol_GRINCH           // OM_SPI
