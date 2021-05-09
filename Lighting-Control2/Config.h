/*
 * Copyright (c) 20148, Daniel Swann <hs@dswann.co.uk>, Matt Lloyd <dps.lwk@gmail.com>
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

// #define BUILD_IDENT "IDE"

/* Pin assigments */
#define PIN_LED0          A0
#define PIN_LED1          A1
#define PIN_LED2          A2
#define PIN_LED3          A3
//      SDA               A4
//      SCL               A5
#define PIN_INPUT_INT      2 /* Uses interupt0 - do not move */
//      N/C                3
//      N/C                4
//      N/C                5
//      N/C                6
//      N/C                7
//      N/C                8
//      N/C                9
//      Ethernet SS       10
//      MISO (RFID+Eth)   11
//      MOSI (RFID+Eth)   12
//      SCK  (RFID+Eth)   13


// Locations in EEPROM of various settings
#define EEPROM_MAC           0  //  6 bytes
#define EEPROM_IP            6  //  4 bytes
#define EEPROM_BASE_TOPIC   10  // 40 bytes   e.g. "nh/tools/"
#define EEPROM_NAME         50  // 20 bytes   e.g. "laser"
#define EEPROM_SERVER_IP    70  //  4 bytes
#define EEPROM_INPUT_ENABLES 74 //  1 bytes   0bit == enabled, 1 == disabled
#define EEPROM_OVERRIDE_MASKS   75 // 32 bytes   4 bytes per 8 inputs
#define EEPROM_OVERRIDE_STATES  107 // 32 bytes   4 bytes per 8 inputs
#define EEPROM_INPUT_STATEFULL  139 // 1 bytes   0bit == enabled, 1 == disabled

#define MQTT_PORT 1883

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"

const char sON[] PROGMEM = "ON";
const char sOFF[] PROGMEM = "OFF";
const char sTOGGLE[] PROGMEM = "TOGGLE";

// PCF8574
// #define PCF_BASE_ADDRESS 0x20
// PCF8574A (we have these :/)
#define PCF_BASE_ADDRESS 0x38

// Debug output destinations
#define DEBUG_MQTT
#define DEBUG_SERIAL

// buffer size
#define DMSG  50

// How fast the state LED should flash when not connected (lower = faster)
#define STATE_FLASH_FREQ  1000

// Serial/config menu current position
enum serial_state_t
{
  SS_MAIN_MENU,
  SS_SET_MAC,
  SS_SET_IP,
  SS_SET_NAME,
  SS_SET_TOPIC,
  SS_SET_SERVER_IP,
  SS_SET_INPUT_OVERRIDE
};

#endif
