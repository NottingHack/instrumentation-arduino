/*
 * Copyright (c) 2014, Daniel Swann <hs@dswann.co.uk>
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

/* Pin assigments */
//      RFID IRQ         A0
//      Screw terminal   A1
//      Card Detect      A2
#define TOOL_POWER_LED   A3
//      LCD SDA          A4
//      LCD SCL          A5
#define PIN_SIGNOFF_BUTTON 2 /* Uses interupt0 - do not move */
#define PIN_INDUCT_BUTTON  3 /* Uses interupt1 - do not move */
#define PIN_RELAY_RESET    4 /* Note: shared with the (unused) sd card reader on the Ethernet shield */
#define PIN_RELAY_SET      5 /* (bistable relay) */
#define PIN_RFID_RST       6 /* RST */
#define PIN_RFID_SS        7 /* SDA */
#define PIN_STATE_LED      8 /* State indicator */
#define PIN_INDUCT_LED     9
//      Ethernet SS       10
//      MISO (RFID+Eth)   11
//      MOSI (RFID+Eth)   12
//      SCK  (RFID+Eth)   13


// Locations in EEPROM of various settings
#define EEPROM_MAC           0 //  6 bytes
#define EEPROM_IP            6 //  4 bytes 
#define EEPROM_BASE_TOPIC   10 // 40 bytes   e.g. "nh/tools/"
#define EEPROM_NAME         50 // 20 bytes   e.g. "laser"
#define EEPROM_SERVER_IP    70 //  4 bytes


#define MQTT_PORT 1883

#define ACTIVE_POLL_FREQ 1000 // Frequency (in ms) to poll for RFID card when device active (i.e. how often to recheck it's present)
#define RFID_RESET_INTERVAL 1000

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"


// Debug output destinations
#define DEBUG_MQTT
#define DEBUG_SERIAL

// buffer size
#define DMSG  50

#define TIMEOUT_SES  (1000*15) // 15s - Session timout - how long to wait without seeing the card before disabling tool

#define INDUCT_TIMEOUT 30     // How long in seconds to wait for card when in the induct state
#define INDUCT_FLASH_FREQ 250 // how fast the induct led flashed after induct button pushed (lower is faster)
#define STATE_FLASH_FREQ  100 // Same again but for state LED

// Serial/config menu current position
enum serial_state_t
{
  SS_MAIN_MENU,
  SS_SET_MAC,
  SS_SET_IP,
  SS_SET_NAME,
  SS_SET_TOPIC,
  SS_SET_SERVER_IP
};



