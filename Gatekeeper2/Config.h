/*
 * Copyright (c) 2017, Daniel Swann <hs@dswann.co.uk>
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

#ifndef GKCONFIG_H
#define GKCONFIG_H

/* Door side a (outside) */
#define SIDEA_LCD_ADDR        0x3E
#define SIDEA_DOORBELL_BUTTON 2
#define SIDEA_BUZZER          A2
#define SIDEA_RFID_SS         7
#define SIDEA_RFID_RST        6
#define SIDEA_STATUS_LED      3

/* Door side b (inside) */
#define SIDEB_LCD_ADDR        0x3F
#define SIDEB_DOORBELL_BUTTON 0xFF
#define SIDEB_BUZZER          A3
#define SIDEB_RFID_SS         8
#define SIDEB_RFID_RST        6
#define SIDEB_STATUS_LED      5

/* Door general */
#define DOOR_CONTACT          A0
#define DOOR_SENSE            A1

#define DOOR_SENSE_SECURE     1
#define DOOR_SENSE_UNSECURE   0

#define PORT_EXPANDER_ADDR    0x38
#define PORT_EXPANDER_RELAY   0    // Bit/port position relay is on (0-7)

// does a high or low on the relay pin unlock the door? 
#define DOOR_RELAY_LOCKED     0

// does a high or low on the DOOR_CONTACT pin signify an open or closed door?
#define DOOR_CONTACT_OPEN     HIGH
#define DOOR_CONTACT_CLOSED   LOW

// timeout in mills for how often the same card can be reported to the server
#define CARD_TIMEOUT 1000

// How long does the door bell button have to pressed for before it counts (ms)
#define DOOR_BUTTON_DELAY 10

// How long to wait after the door is opened before locking (ms)
#define DOOR_LOCK_TIME_AFTER_OPEN 5000

// After receiving a door unlock message, how long (at most) should the door be kept unlocked for (ms)
#define DOOR_RELAY_TIMEOUT 5000

// Minimum interval between door bell button presses (ms)
#define DOOR_BUTTON_INTERVAL 5000

// How fast the state LED should flash when not connected (lower = faster)
#define STATE_FLASH_FREQ  1000

// (retained) topic that has the default message for both sides of the door
#define DOOR_DEFAULT_MESSAGE_A "nh/gk/DefaultMessage/A"
#define DOOR_DEFAULT_MESSAGE_B "nh/gk/DefaultMessage/B"

// Port the MQTT broker is on
#define MQTT_PORT 1883

// Locations in EEPROM of various settings
#define EEPROM_MAC           0 //  6 bytes
#define EEPROM_IP            6 //  4 bytes 
#define EEPROM_BASE_TOPIC   10 // 40 bytes   e.g. "nh/gk"
#define EEPROM_DOOR_ID      50 //  1 byte
#define EEPROM_SERVER_IP    51 //  4 bytes

// Status Topic
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"

// Debug output destinations
#define DEBUG_MQTT
#define DEBUG_SERIAL

// buffer size
#define DMSG  50


// Serial/config menu current position
enum serial_state_t
{
  SS_MAIN_MENU,
  SS_SET_MAC,
  SS_SET_IP,
  SS_SET_DOOR_ID,
  SS_SET_TOPIC,
  SS_SET_SERVER_IP
};

enum door_relay_state_t
{
  DR_LOCKED,
  DR_UNLOCKED
};

enum door_state_t
{
   DS_UNKNOWN,
   DS_OPEN,
   DS_CLOSED,
   DS_LOCKED,
   DS_FAULT
};


#endif
