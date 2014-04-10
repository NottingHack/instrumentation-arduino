/*
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>
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
//      LCD SDA          A4
//      LCD SCL          A5
#define PIN_CANCEL_BUTTON 2
#define PIN_CANCEL_LIGHT  3
//      Ethernet          4
#define PIN_RFID_RST      6
#define PIN_RFID_SS       7
#define PIN_NOTE_TX       8
#define PIN_NOTE_RX       9
//      Ethernet SS      10
//      MISO (RFID+Eth)  11
//      MOSI (RFID+Eth)  12
//      SCK  (RFID+Eth)  13


// Locations in EEPROM of various settings
#define EEPROM_MAC           0 //  6 bytes
#define EEPROM_IP            6 //  4 bytes
#define EEPROM_BASE_TOPIC   10 // 40 bytes
#define EEPROM_NAME         50 // 20 bytes

// IP of MQTT server
byte server[] = { 192, 168, 0, 1 };
#define MQTT_PORT 1883

// Subscribe to topics
#define S_NOTE_RX "nh/note_acc/tx"

// Publish Topics
#define P_NOTE_TX "nh/note_acc/rx"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: note_acceptor"
#define RESTART "Restart: note_acceptor"

// Client id for connecting to MQTT
#define CLIENT_ID "note_acceptor"

// Debug output destinations
#define DEBUG_MQTT
#define DEBUG_SERIAL

// buffer size
#define DMSG  50

#define TIMEOUT_COMS (1000*10) // 10s - timeout waiting for server/note acceptor.
#define TIMEOUT_SES  (1000*30) // 30s - Sesion timout - (STATE_ACCEPT - waiting for user)

enum serial_state_t
{
  SS_MAIN_MENU
};
