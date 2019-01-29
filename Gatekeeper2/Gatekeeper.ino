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


/* Arduino tool access control for Nottingham Hackspace
 *
 * Target board = Arduino Uno, Arduino version = 1.0.1
 *
 * Expects to be connected to:
 *   - I2C 16x2 LCD (HCARDU0023)) - OPTIONAL
 *   - SPI RFID reader (HCMODU0016)
 *   - Wiznet W5100 based Ethernet shield
 * Pin assignments are in Config.h
 *
 * Configuration (IP, MAC, etc) is done using serial @ 9600. Reset the Arduino after changing the config.
 *
 * Additional required libraries:
 *   - PubSubClient                       - https://github.com/knolleary/pubsubclient
 *   - HCARDU0023_LiquidCrystal_I2C_V2_1  - http://forum.hobbycomponents.com/viewtopic.php?f=39&t=1125
 *   - rfid                               - https://github.com/miguelbalboa/rfid
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <avr/wdt.h> 
#include "Config.h"
#include "Gatekeeper.h"
#include "Menu.h"

DoorSide *_SideA;
DoorSide *_SideB;

EthernetClient _ethClient;
PubSubClient *_client;
bool _mqtt_connected = false;
door_relay_state_t _door_relay_state;
char _pmsg[DMSG];
bool _door_contact_state;
unsigned long _door_contact_change_time;
char _door_topic[20+BASE_TOPIC_LEN+10+4]; // name + _base_topic + action + delimiters + terminator e.g. "nh/gk/1/"
unsigned long _door_unlocked_time = 0;
char _door_state_topic[50]="";

// set to millis() on first startup, and then everytime the connection is confirmed good 
// If this is > RESTART_TIMEOUT away from the current time, don't reset the WDT.
unsigned long _wdt_connected_timer = 0; 

void setup()
{
  char *rfid_serial;
  wdt_reset();
  wdt_enable(WDTO_8S);

  // for I2C port expander
  Wire.begin();

  Serial.begin(9600);
  dbg_println(F("Start!"));
  _serial_state = SS_MAIN_MENU;

  lock_door();

  dbg_println(F("Init SPI"));
  SPI.begin();

  // Read settings from eeprom
  for (int i = 0; i < 6; i++)
    _mac[i] = EEPROM.read(EEPROM_MAC+i);

  for (int i = 0; i < 4; i++)
    _ip[i] = EEPROM.read(EEPROM_IP+i);

  for (int i = 0; i < 4; i++)
    _server[i] = EEPROM.read(EEPROM_SERVER_IP+i);

  for (int i = 0; i < BASE_TOPIC_LEN-1; i++)
    _base_topic[i] = EEPROM.read(EEPROM_BASE_TOPIC+i);
  _base_topic[BASE_TOPIC_LEN-1] = '\0';

  for (int i = 0; i < 21; i++)
    _rfid_override[i] = EEPROM.read(EEPROM_EMGCY_RFID+i);
  _rfid_override[EEPROM_EMGCY_RFID-1] = '\0';
 
  _door_id = EEPROM.read(EEPROM_DOOR_ID);

  sprintf(_dev_name,         "Gatekeeper-%d", _door_id);
  sprintf(_door_topic,       "%s/%d/"       , _base_topic, _door_id);
  sprintf(_door_state_topic, "%sDoorState"  , _door_topic);

  _SideA = new DoorSide(SIDEA_LCD_ADDR, SIDEA_DOORBELL_BUTTON, SIDEA_BUZZER, SIDEA_RFID_SS, SIDEA_RFID_RST, SIDEA_STATUS_LED, 'A', &_mqtt_connected);
  _SideB = new DoorSide(SIDEB_LCD_ADDR, SIDEB_DOORBELL_BUTTON, SIDEB_BUZZER, SIDEB_RFID_SS, SIDEB_RFID_RST, SIDEB_STATUS_LED, 'B', &_mqtt_connected);
  _SideA->init();
  _SideB->init();

  // Check if the RFID in EEPROM is already present - if so, unlock door before starting network
  rfid_serial = _SideA->poll_rfid();
  check_for_override_rfid(rfid_serial);
  rfid_serial = _SideB->poll_rfid();
  check_for_override_rfid(rfid_serial);

  dbg_println(F("Start Ethernet"));
  Ethernet.begin(_mac, _ip);

  _ethClient.setTimeout(2);
  _client = new PubSubClient(_server, MQTT_PORT, callbackMQTT, _ethClient);

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  wdt_reset();
  checkMQTT();
  wdt_reset();

  Serial.println();
  serial_show_main_menu();

  pinMode(DOOR_CONTACT, INPUT);
  digitalWrite(DOOR_CONTACT, LOW);
  _door_contact_state = digitalRead(DOOR_CONTACT);
  _door_contact_change_time = millis();
  
  pinMode(DOOR_SENSE, INPUT);
  digitalWrite(DOOR_SENSE, LOW);
  dbg_println(F("Setup done"));
  _wdt_connected_timer = millis();
  
} // end void setup()


void loop()
{
  if (millis() - _wdt_connected_timer < RESTART_TIMEOUT)
    wdt_reset();

  // Check for door beel button, rfid reads, etc
  door_side_loop(_SideA);
  door_side_loop(_SideB);

  // should cause callback if there's a new message
  _client->loop();

  // are we still connected to MQTT
  checkMQTT();

  // Do serial menu
  serial_menu();

  // Check if door needs locking again
  check_door_relay();

  //timeout_loop();
  check_door();

  // If the door has changed state, send an update
  update_door_state();
} // end void loop()


void door_side_loop(DoorSide *side)
{
  // Check if a door bell button has been pushed
  check_buttons(side);
  
  // Check for RFID cards
  poll_rfid(side);

  // update status LEDs
  side->status_led();

  // Update LCDs if nessesary
  side->lcd_loop();
}

void poll_rfid(DoorSide *side)
{
  char *rfid_serial;

  rfid_serial = side->poll_rfid();
  if (rfid_serial != NULL)
  {
    check_for_override_rfid(rfid_serial);
    send_door_side_msg(side->get_side(), "RFID", rfid_serial);
  }
}

void check_for_override_rfid(char *rfid_serial)
{
  if (rfid_serial != NULL)
  {
    if (!strncmp(_rfid_override, rfid_serial, 20))
    {
      dbg_println(F("RFID override serial seen")); 
      unlock_door();
    }
  }
}

void send_door_side_msg(char side_id, char *topic, char *msg)
// take "nh/gk/<door_id>/" in _door_topic, change to:
//      "nh/gk/<door_id>/<side_id>/<topic>"
// send message over MQTT using it, then reset _door_topic
// to how it was.
{
  uint8_t dt_len = strlen(_door_topic);

  _door_topic[dt_len] = side_id;        // add side
  _door_topic[dt_len+1] = '/';          // add seperator
  strcpy(_door_topic+dt_len+2, topic);  // add topic

  _client->publish(_door_topic, msg);   // send message
  _door_topic[dt_len] = '\0';           // reset
}


void check_buttons(DoorSide *side)
{
  if (side->check_button())
  {
    //dbg_println(F("Doorbell: "));
    //Serial.println(side->get_side());
    send_door_side_msg(side->get_side(), "DoorButton", "1");
  }
}

void check_door_relay()
{
  if (_door_relay_state == DR_LOCKED)
    return; // already locked, nothing to do

  if (millis() - _door_unlocked_time < DOOR_RELAY_TIMEOUT)
    return;

  // Time to lock the foor
  lock_door();
}


void check_door()
{
 
  bool new_door_contact_state = digitalRead(DOOR_CONTACT);

  // Ignore state change if it's already changed within the last 50ms
  if (new_door_contact_state != _door_contact_state && (millis()-_door_contact_change_time < 50))
    return;

  // If the door has been unlocked for at least 1 second...
  if ((_door_relay_state == DR_UNLOCKED) && (millis()-_door_unlocked_time > DOOR_LOCK_TIME_AFTER_OPEN))
  {
    // ...and is now open, re-lock it
    if (new_door_contact_state == DOOR_CONTACT_OPEN)
    {
      lock_door();
    }
  }
  else
  {
    // Send door state change message, if applicable
    if (new_door_contact_state == DOOR_CONTACT_CLOSED && _door_contact_state == DOOR_CONTACT_OPEN)
    {
      // Door has been closed
    }

    if (new_door_contact_state == DOOR_CONTACT_OPEN && _door_contact_state == DOOR_CONTACT_CLOSED)
    {
      // Door has been opened
    }
  }

  if (_door_contact_state != new_door_contact_state)
  {
    _door_contact_state = new_door_contact_state;
    _door_contact_change_time = millis();
  }
}


void send_door_state(door_state_t door_state)
{
  char state[8];

  switch (door_state)
  {
    case DS_OPEN:
      strcpy(state, "OPEN");
      break;

    case DS_CLOSED:
      strcpy(state, "CLOSED");
      break;

    case DS_LOCKED:
      strcpy(state, "LOCKED");
      break;

    default:
      strcpy(state, "FAULT");
      break;
  }

  _client->publish(_door_state_topic, (uint8_t*)state, strlen(state), true);
}

// Called regulary from the main loop. Work out if the door state has changed, and if it has, report it.
void update_door_state(boolean always_send_update)
{
  static door_state_t current_door_state = DS_UNKNOWN;
  static unsigned long current_door_state_change_time=0;

  door_state_t new_door_state = DS_FAULT;
  uint8_t door_sense = digitalRead(DOOR_SENSE);


  // Wait for things to settle: don't normally send an update if relevant states have changed recently (e.g. doors only just been opened).
  if
  (
    (millis()-_door_contact_change_time < 100)     || // Door contact has just changed state
    (millis()-_door_unlocked_time < 100)           || // Door has just been unlocked
    (millis()-current_door_state_change_time < 100)   // Door state has just changed
  )
  {
    if (always_send_update)
      send_door_state(new_door_state);
    return;
  }

  if ((_door_contact_state == DOOR_CONTACT_OPEN) && (door_sense == DOOR_SENSE_UNSECURE))
    new_door_state = DS_OPEN;   // Door is open and not not locked (open+locked should be reported as a fault)
  else if ((_door_contact_state == DOOR_CONTACT_CLOSED) && (door_sense == DOOR_SENSE_UNSECURE))
    new_door_state = DS_CLOSED; // Door is closed and not locked
  else if ((_door_contact_state == DOOR_CONTACT_CLOSED) && (door_sense == DOOR_SENSE_SECURE) && (_door_relay_state == DR_LOCKED))
    new_door_state = DS_LOCKED; // Door is closed and locked
  else
    new_door_state = DS_FAULT;  // Door is in some weird state it probably shouldn't/can't be (e.g. open and locked)

  if (always_send_update)
  {
    current_door_state = new_door_state;
    send_door_state(new_door_state);
  }
  else
  {
    // If the door state has changed, send an update
    if (current_door_state != new_door_state)
    {
      send_door_state(new_door_state);
      current_door_state_change_time = millis();
      current_door_state = new_door_state;
    }
  }
}


void checkMQTT()
/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 ****************************************************/
{
  static boolean first_connect = true;

  if (_client->connected())
  {
    // already connected
    _wdt_connected_timer = millis();
  }
  else
  {
    if (_client->connect(_dev_name, _door_state_topic, 0, 1, "UNKNOWN")) // on disconnection, set door state to unknown
    {

      char buf_sts[30]="";
      char buf_subscribe[50]="";

      dbg_println(F("Connected to MQTT"));
      sprintf(buf_sts, "Restart: %s", _dev_name);
      _client->publish(P_STATUS, buf_sts);

      _client->subscribe(S_STATUS);

      // Door side specific messages (display / LCD)
      sprintf(buf_subscribe, "%s/%d/+/+", _base_topic, _door_id);
      _client->subscribe(buf_subscribe);

      // Door messages (unlock)
      sprintf(buf_subscribe, "%s/%d/+", _base_topic, _door_id);
      _client->subscribe(buf_subscribe);

      _client->subscribe(DOOR_DEFAULT_MESSAGE_A);
      _client->subscribe(DOOR_DEFAULT_MESSAGE_B);

      // Send door state
      update_door_state(true);

      // Update state
      if (first_connect)
      {
        first_connect = true;
        _mqtt_connected = true;
        dbg_println(F("Boot->idle"));
      }
      else
      {
        _mqtt_connected = true;
        dbg_println(F("Reset->idle"));
      }
    }
    else
    {
      _mqtt_connected = false;
      display_message('Z', "ERR", 1000);
    }
  }
}


void callbackMQTT(char* topic, byte* payload, unsigned int length)
{
  char buf [30];
  char *pTopic;
  
  
  // Respond to status requests
  if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
      dbg_println(F("Status Request"));
      sprintf(buf, "Running: %s", _dev_name);
      _client->publish(P_STATUS, buf);
      return;
    }
  }

  if (!strcmp(topic, DOOR_DEFAULT_MESSAGE_A))
  {
    _SideA->set_default_message(payload, length);
    return;
  } 
  else if (!strcmp(topic, DOOR_DEFAULT_MESSAGE_B))
  {
    _SideB->set_default_message(payload, length);
    return;
  }


  if ((strncmp(topic, _door_topic, strlen(_door_topic))) || (strlen(topic) < strlen(_door_topic)+2))
  {
    dbg_println(F("Unexpected topic (door specific)"));
    return;
  }
  pTopic = topic + strlen(_door_topic);

  // Look for messages aimed at the door (not a specific side)

  if (!strcmp(pTopic, "Unlock"))
  {
    // Payload is unimportant. A seperate message will be sent by the server to
    // the door side that requested the unlock to display on the relevant LCD.

    dbg_println(F("Got unlock message"));
    unlock_door();
    return;
  }

  // Look for message for specific door side
  if ( ((!strncmp(pTopic, "A", 1)) || (!strncmp(pTopic, "B", 1)) || (!strncmp(pTopic, "Z", 1)) )  && (strlen(pTopic) > 4) )
  {
    char door_side;
    dbg_println(F("Got door side specific message"));
    door_side = *(pTopic);
    pTopic += 2; // Skip over, e.g. "A/"

    if (!strcmp(pTopic, "Display"))
    {
      // Payload should be in the form "<duration ms>:<message>", lengths: nnnn:<up to 40 char msg>
      char lcdmsg[41]=""; // LCD can hold at most 40 (20x2) characters
      uint16_t duration=0;
      char durtmp[5];

      if (length <= 6)
      {
        dbg_println(F("Invalid message"));
        return;
      }

      // get duration
      memset(durtmp, 0, sizeof(durtmp));
      memcpy(durtmp, payload, 4);
      duration = atoi(durtmp);

      // get message
      memset(lcdmsg, 0, sizeof(lcdmsg));
      memcpy(lcdmsg, payload+5, length-5);

      display_message(door_side, lcdmsg, duration);
    }

    if (!strcmp(pTopic, "Buzzer"))
    {
      char tmp[6];
      uint16_t tone_hz;  // hz
      uint16_t duration; // ms

      // payload is <tone_hz>:<duration>, in the form nnnnn:nnnn
      if (length < 10)
      {
        dbg_println(F("Invalid message"));
        return;
      }

      // get tone
      memset(tmp, 0, sizeof(tmp));
      memcpy(tmp, payload, 5);
      tone_hz = atoi(tmp);

      // get duration
      memset(tmp, 0, sizeof(tmp));
      memcpy(tmp, payload+6, 4);
      duration = atoi(tmp);

      buzzer(door_side, tone_hz, duration);
    }
  }
} // end void callback(char* topic, byte* payload,int length)


void display_message(char side, char *msg, int duration)
// Display <msg> and the LCD of door side <side>. If <side> is Z, show on both.
{
  if (side == 'A' || side == 'Z')
    _SideA->show_message(msg, duration);

  if (side == 'B' || side == 'Z')
    _SideB->show_message(msg, duration);
}

void buzzer(char door_side, uint16_t tone_hz, uint16_t duration)
{
  if (door_side == 'A' || door_side == 'Z')
    _SideA->buzzer(tone_hz, duration);

  if (door_side == 'B' || door_side == 'Z')
    _SideB->buzzer(tone_hz, duration);
}

// set/clear single bit (pin) on port expander
void port_expander_write(uint8_t pin, bool val)
{
  static uint8_t port_exp_val=0;

  if (val)
    port_exp_val |= 1 << pin;    // set pin/bit
  else
    port_exp_val &= ~(1 << pin); // clear pin/bit

  // Write value to port expander
  Wire.beginTransmission(PORT_EXPANDER_ADDR);
  Wire.write(port_exp_val);
  Wire.endTransmission();
}

void lock_door()
{
  // config.h can be used to set if a high or low on the relay pin should lock the door
  if (DOOR_RELAY_LOCKED)
  {
    port_expander_write(PORT_EXPANDER_RELAY, true);  // Set bit to lock door
    
    // Temp bodge: pin0 on the gk-4 (CNCCORIDOR) board doesn't seem to work. There is now a wire link between pin 0 and pin 1. Remove when gk-4 is fixed.
    port_expander_write(1, true);
  }
  else
  {
    port_expander_write(PORT_EXPANDER_RELAY, false); // clear bit to lock door
    port_expander_write(1, false); 
  }

  dbg_println(F("Door locked")); 
  _door_relay_state = DR_LOCKED;
  return;
}

void unlock_door()
{
  // config.h can be used to set if a high or low on the relay pin should unlock the door
  if (DOOR_RELAY_LOCKED)
  {
    port_expander_write(PORT_EXPANDER_RELAY, false); // clear bit to unlock door
    port_expander_write(1, false); // bodge for gk-4
  }
  else
  {
    port_expander_write(PORT_EXPANDER_RELAY, true);  // set bit to unlock door
    port_expander_write(1, true); // bodge for gk-4
  }
  
  dbg_println(F("Door unlocked"));
  _door_relay_state = DR_UNLOCKED;
  _door_unlocked_time = millis();
  return;
}

void dbg_println(const __FlashStringHelper *n)
{
  uint8_t c;
  byte *payload;
  char *pn = (char*)n;

  memset(_pmsg, 0, sizeof(_pmsg));
  strcpy(_pmsg, "INFO:");
  payload = (byte*)_pmsg + sizeof("INFO");

  while ((c = pgm_read_byte_near(pn++)) != 0)
    *(payload++) = c;

//#ifdef DEBUG_MQTT TODO/FIXME
 //   _client->publish("INFO", _pmsg);
//#endif


  Serial.println(_pmsg+sizeof("INFO"));

}


void dbg_println(const char *msg)
{
  /*
  byte *payload;
 
 memset(_pmsg, 0, sizeof(_pmsg));
 strcpy(_pmsg, " ");
 strcat(_pmsg, msg);
 payload = (byte*)_pmsg + sizeof("");
 
 
 
 
 Serial.println(_pmsg+sizeof(""));
 
 
  _client->publish("INFO", _pmsg);
  */
  Serial.println(msg);
}




