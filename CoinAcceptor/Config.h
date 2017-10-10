/*
 * Copyright (c) 2017, Daniel Swann <hs@dswann.co.uk>, Robert Hunt <rob.hunt@nottinghack.org.uk>
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
//      I2C SDA (LCD)     A4
//      I2C SCL (LCD)     A5
#define PIN_COIN_PULSE    2 // requires interrupt
#define PIN_COIN_ENABLE   3
//      SD Card SS        4
#define PIN_RFID_RST      6
#define PIN_RFID_SS       7 // labeled SDA on RFID
#define PIN_CANCEL_BUTTON 8
#define PIN_CANCEL_LIGHT  9
//      Ethernet SS      10
//      MISO (RFID+Eth)  11
//      MOSI (RFID+Eth)  12
//      SCK  (RFID+Eth)  13

// MAC / IP address of device
byte mac[]    = { 0xDE, 0xED, 0xBA, 0xFE, 0x65, 0xF1 };
byte ip[]     = { 192, 168, 1, 171 };

// IP of MQTT server
byte server[] = { 192, 168, 1, 65 };
#define MQTT_PORT 1883

// Subscribe to topics
#define S_COIN_RX "nh/coin_acc/tx"

// Publish Topics
#define P_COIN_TX "nh/coin_acc/rx"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: coin_acceptor"
#define RESTART "Restart: coin_acceptor"

// Client id for connecting to MQTT
#define CLIENT_ID "coin_acceptor"

// Debug output destinations
#define DEBUG_MQTT
#define DEBUG_SERIAL

// buffer size
#define DMSG 100

#define COIN_PULSE_VALUE 5 // how many pence each pulse from the coin acceptor represents
#define COIN_PULSE_TIMEOUT_MS 200 // In fast mode, each pulse is 30ms, with an interval of 100ms (130ms total)
                                  // so when we see a gap of 200ms or more we use that as a signal to stop counting

#define TIMEOUT_COMS (1000*10) // 10s - timeout waiting for server
#define TIMEOUT_SES  (1000*20) // 20s - Sesion timout - (STATE_ACCEPT - waiting for user)

enum state_t
{
  STATE_CONN_WAIT,           // Waiting for MQTT connection
  STATE_READY,               // RFID enabled, coin acceptor disabled.
  STATE_AUTH_WAIT,           // After RFID card read - waiting on response from server
  STATE_ACCEPT,              // Server responded ok, RFID reader disabled, coin acceptor enabled
  STATE_ACCEPT_COIN_COUNT,   // Coins taken in, waiting for them to be counted
  STATE_ACCEPT_NET_WAIT,     // Coins counted, waiting for server to give go-ahead to accept
  STATE_ACCEPT_CONFIRM_WAIT, // Server given go-ahead waiting to confirm transaction
};
