/*
 * Copyright (c) 2021, Daniel Swann <hs@dswann.co.uk>, Matt Lloyd <dps.lwk@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIGHTING_CONTROL_CONFIG_H
#define LIGHTING_CONTROL_CONFIG_H

#include "Arduino.h"

// #define BUILD_IDENT "IDE"

// Serial EM RS485
// Serial1 RS485IO

/* Pin assigments */
#define PIN_LED_0          A0
#define PIN_LED_1          A1
#define PIN_LED_2          A2
#define PIN_LED_3          A3
//      N/C               A4
//      N/C               A5
#define RS485_RE           2
#define SPI_EEPROM_SS      3
#define RS485_DE           4
#define EM_SO              5
#define EM_RS485_RE        6
#define EM_RS485_DE        7
#define PIN_INPUT_INT      8
//      N/C                9
//      Ethernet SS       10
//      MISO (EEPROM+Eth) 11
//      MOSI (EEPROM+Eth) 12
//      SCK  (EEPROM+Eth) 13


// Locations in EEPROM of various settings
#define EEPROM_MAC                      0UL   //   6 bytes
#define EEPROM_IP                       6UL   //   4 bytes
#define EEPROM_BASE_TOPIC               10UL  //  40 bytes   e.g. "nh/tools/"
#define EEPROM_NAME                     50UL  //  20 bytes   e.g. "laser"
#define EEPROM_SERVER_IP                70UL  //   4 bytes
#define EEPROM_INPUT_ENABLES            74UL  //   1 bytes   0bit == enabled, 1 == disabled
#define EEPROM_OVERRIDE_MASKS           75UL  //  32 bytes   4 bytes per 8 inputs
#define EEPROM_OVERRIDE_STATES          107UL //  32 bytes   4 bytes per 8 inputs
#define EEPROM_INPUT_STATEFULL          139UL //   1 bytes   0bit == enabled, 1 == disabled
#define EEPROM_ENERGY_MONITOR_ENABLE    140UL //   1 bytes
#define EEPROM_RS458_IO_COUNT           141UL //   1 bytes
#define EEPROM_RS485_INPUT_ENABLES      142UL //  20 bytes   2 bytes per io modules (x10) 0bit == enabled, 1 == disabled
#define EEPROM_RS485_OVERRIDE_MASKS     162UL // 640 bytes   4 bytes per 16 inputs per io module (x10)
#define EEPROM_RS485_OVERRIDE_STATES    802UL // 640 bytes   4 bytes per 16 inputs per io module (x10)
#define EEPROM_RS485_INPUT_STATEFULL    1442UL //  20 bytes   2 bytes per io modules (x10) 0bit == enabled, 1 == disabled
#define EEPROM_VALID                    1462UL //  1 bytes
// #define EEPROM_                         1463 // 585 left @ 2K, 2681 @ 4K


/***************
 *** Network ***
 ***************/

// Network config defaults - can be overridden using serial menu
uint8_t const _default_mac[6]       = {0xB8, 0xFC, 0xBF, 0x87, 0x52, 0x68};
uint8_t const _default_ip[4]        = {192,168,15,230}; // C0 A8 0f E6
uint8_t const _default_server_ip[4] = {192,168,15,170}; // C0 A8 0f AA
#define       DEFAULT_BASE_TOPIC   "nh/li" // 6e 68 2f 6c 69
#define       DEFAULT_NAME         "test" // 74 65 73 74

// MQTT stuff
#define MQTT_PORT 1883

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"

const char sON[] = "ON";
const char sOFF[] = "OFF";
const char sTOGGLE[] = "TOGGLE";

// PCF8574
// #define PCF_BASE_ADDRESS 0x20
// PCF8574A (we have these :/)
#define PCF_BASE_ADDRESS 0x38
#define PCF_INPUT_ADDRESS (PCF_BASE_ADDRESS | 7)

// How fast the state LED should flash when not connected (lower = faster)
#define STATE_FLASH_FREQ  1000

#define MENU_TIMEOUT 5000

#define RS485_BAUD 19200
#define RS485_SERIAL_CONFIG SERIAL_8E1
#define RS485_READ_INTERVAL 20

#endif
