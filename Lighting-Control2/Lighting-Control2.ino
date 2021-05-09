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


/* Arduino automated lighting control for Nottingham Hackspace
 *
 * Target board = Arduino Uno, Arduino version = 1.0.1
 *
 * Expects to be connected to:
 *   - Wiznet W5100 based Ethernet shield
 * Pin assignments are in Config.h
 *
 * Configuration (IP, MAC, etc) is done using serial @ 9600. Reset the Arduino after changing the config.
 *
 * Additional required libraries:
 *   - PubSubClient                       - https://github.com/knolleary/pubsubclient
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include "Config.h"
#include "Lighting.h"
#include "Menu.h"

// #define BUILD_IDENT "IDE"
#define LCD_WIDTH 16

EthernetClient _ethClient;
PubSubClient *_client;
LiquidCrystal_I2C lcd(0x27,16,2);  // 16x2 display at address 0x27

dev_state_t    _dev_state;

char _pmsg[DMSG];

volatile boolean _input_int_pushed;
volatile unsigned long _input_int_pushed_time;
char tool_topic[20+40+10+4]; // name + _base_topic + action + delimiters + terminator
unsigned long _last_state_change = 0;
uint8_t _input_state = 0;
uint8_t _input_state_tracking = 0;
uint32_t _output_state = 0x00000000;
uint32_t _output_retained = 0x00000000;

/****************************************************
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
{
  char buf [30];

  // Respond to status requests
  if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
      dbg_println(F("Status Request"));
      sprintf(buf, "Running: %s", _dev_name);
      _client->publish(P_STATUS, buf);
    }
  }

  // Messages to the lighting topics
  // nh/li/LightsCNC/{channel}/set      deal with this
  // nh/li/LightsCNC/{channel}/state    only care if its first one for this channel (the retained state)
  // nh/li/LightsCNC                    dont care
  if (!strncmp(topic, tool_topic, strlen(tool_topic)))
  {
    if (strlen(topic) < strlen(tool_topic)+4)
      return;
    strncpy(buf, topic+strlen(tool_topic)+4, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';

    if (strcmp(buf, "set") == 0)
    {
      strncpy(buf, topic+strlen(tool_topic)+1, 2);
      buf[2] = '\0';
      if (buf[0] == 'I')
        return;
      handle_set(buf, payload, length);
    }

    if (strcmp(buf, "state") == 0)
    {
      strncpy(buf, topic+strlen(tool_topic)+1, 2);
      buf[2] = '\0';
      if (buf[0] == 'I')
        return;
      handle_state(buf, payload, length);
    }
  }

} // end void callback(char* topic, byte* payload,int length)

void handle_set(char* channel, byte* payload, int length)
{
  int chan = atoi(channel);
  if (chan < 0 || chan > 31)
    return;

  // update chan
  _output_retained |= (1UL<<(chan));
  if (strncmp_P((char*)payload, sTOGGLE, strlen_P(sTOGGLE)) == 0){
    // toggle bit
    _output_state ^= (1UL<<(chan));
  } else if (strncmp_P((char*)payload, sON, strlen_P(sON)) == 0) {
    // set bit
    _output_state |= (1UL<<(chan));
  } else if (strncmp_P((char*)payload, sOFF, strlen_P(sOFF)) == 0) {
    // clear bit
    _output_state &= ~(1UL<<(chan));
  } else {
      //unknown command
      return;
  }
  // update outputs
  update_outputs(chan);

  // publish chan
  publish_output_state(channel);
}

void handle_state(char* channel, byte* payload, int length)
{
  int chan = atoi(channel);
  uint32_t chanMask = 1UL<<(chan);
  if (chan < 0 || chan > 31)
    return;
  if (_output_retained == 0xFFFFFFFF || (_output_retained & chanMask) )
    return;

  // update chan
  _output_retained |= chanMask;
  if (strncmp_P((char*)payload, sON, strlen_P(sON)) == 0) {
    // set bit
    _output_state |= chanMask;
  } else if (strncmp_P((char*)payload, sOFF, strlen_P(sOFF)) == 0) {
    // clear bit
    _output_state &= ~chanMask;
  } else {
      //unknown command
      return;
  }
  // update outputs
  update_outputs(chan);

  // publish chan
  publish_output_state(channel);

  if (_output_retained == 0xFFFFFFFF) {
    send_action("RETAINED", "SYNCED");
  }
}

void update_outputs(int chan)
{
  if (chan >= 24) {
    Wire.beginTransmission(PCF_BASE_ADDRESS | ((chan/8)+1));
  } else {
    Wire.beginTransmission(PCF_BASE_ADDRESS | (chan/8));
  }
  Wire.write((uint8_t)(_output_state >> (((uint32_t)chan/8UL)*8UL)));
  Wire.endTransmission();
}

void read_inputs()
{
  uint8_t old_input = _input_state;
  Wire.requestFrom(PCF_BASE_ADDRESS | 3 , 1);
  _input_state = Wire.read();

  // find out which input has changed if any
  // _input_state = new_input;
  for (int i = 0; i < 8; ++i)
  {
    // (n & ( 1 << k )) >> k
    if ((old_input & ( 1 << i )) != (_input_state & ( 1 << i )) )
    {
      publish_input_state(i);
      // check input mapping and update outputs
      if (!(_input_state & ( 1 << i ))) // low == pressed
      {
        if ((_input_enables & (1<<i)) == 0)
        {
          uint32_t _override_state = _override_states[i];
          // if we are tracking the state of this input pin is enabled (low == enabled)
          // and this the second press
          if (((_input_statefullness & (1<<i)) == 0) && ((_input_state_tracking & (1<<i)) != 0))
          {
            // flip the output state requests
            _override_state = ~_override_state;
          }

          for (uint32_t chan = 0; chan <= 31; ++chan)
          {
            uint32_t chanMask = 1UL<<chan;
            if (_override_masks[i] & chanMask) {
              if (_override_state & chanMask) {
                _output_state |= chanMask;
              } else {
                _output_state &= ~chanMask;
              }
              update_outputs(chan);
              char channel[3];
              sprintf(channel, "%02lu", chan);
              publish_output_state(channel);
            }
          }
        }

        // toggle tracking state bit for this input pin
        _input_state_tracking ^= 1 << i;
      }
    }
  }
}

void publish_all_input_states()
{
  Wire.requestFrom(PCF_BASE_ADDRESS | 3 , 1);
  _input_state = Wire.read();

  for (int i = 0; i < 8; ++i)
  {
    // boolean bit_state = (_input_state & ( 1 << i )) >> i;
    publish_input_state(i);
  }
}

void publish_input_state(int channel)
{
  char msg[4];
  if (!(_input_state & (1<<(channel)))) {
    strcpy_P(msg, sON);
  } else {
    strcpy_P(msg, sOFF);
  }

  int tt_len = strlen(tool_topic);
  tool_topic[tt_len] = '/';
  tool_topic[tt_len+1] = 'I';
  tool_topic[tt_len+2] = channel | 0x30;
  strcpy(tool_topic+tt_len+3, "/state");
  _client->publish(tool_topic, (uint8_t*) msg, strlen(msg), true);
  tool_topic[tt_len] = '\0';
}

void publish_output_state(char* channel)
{
  char msg[4];
  int chan = atoi(channel);
  if (_output_state & (1UL<<chan)) {
    strcpy_P(msg, sON);
  } else {
    strcpy_P(msg, sOFF);
  }

  int tt_len = strlen(tool_topic);
  tool_topic[tt_len] = '/';
  strcpy(tool_topic+tt_len+1, channel);
  strcpy(tool_topic+tt_len+3, "/state");
  _client->publish(tool_topic, (uint8_t*) msg, strlen(msg), true);
  tool_topic[tt_len] = '\0';
}

void lcd_display_mqtt(char *payload)
{
  // Display payload on LCD display - \n in string seperates line 1+2.
  char ln1[LCD_WIDTH+1];
  char ln2[LCD_WIDTH+1];
  char *payPtr, *lnPtr;

  payPtr = (char*)payload;

  memset(ln1, 0, sizeof(ln1));
  memset(ln2, 0, sizeof(ln2));

  /* Clear display */
  lcd.clear();

  lnPtr = ln1;
  for (int i=0; i < LCD_WIDTH; i++)
  {
    if ((*payPtr=='\n') || (*payPtr=='\0') || (*payPtr=='\r'))
    {
      payPtr++;
      break;
    }

    *lnPtr++ = *payPtr++;
  }

  lnPtr = ln2;
  for (int i=0; i < LCD_WIDTH; i++)
  {
    if (*payPtr=='\0')
      break;

    if (*payPtr=='\n')
    {
      payPtr++;
      *lnPtr=' ';
    }

    *lnPtr++ = *payPtr++;
  }

  lcd.clear();
  /* Output 1st line */
  lcd.setCursor(0, 0);
  lcd.print(ln1);

  /* Output 2nd line */
  lcd.setCursor(0, 1);
  lcd.print(ln2);
}

/****************************************************
 * check we are still connected to MQTT
 * reconnect if needed
 *
 ****************************************************/
void checkMQTT()
{
  char *pToolTopic;
  static boolean first_connect = true;

  if (!_client->connected())
  {
    if (_client->connect(_dev_name))
    {
      char buf[30];

      dbg_println(F("Connected to MQTT"));
      sprintf(buf, "Restart: %s", _dev_name);
      _client->publish(P_STATUS, buf);

      // Subscribe to <tool_topic>/#, without having to declare another large buffer just for this.
      pToolTopic = tool_topic + strlen(tool_topic);
      *pToolTopic     = '/';
      *(pToolTopic+1) = '#';
      *(pToolTopic+2) = '\0';
      Serial.println(tool_topic);
      _client->subscribe(tool_topic);
      *pToolTopic = '\0';

      Serial.println(S_STATUS);
      _client->subscribe(S_STATUS);

      // Update state
      if (first_connect)
      {
        send_action("RESET", "BOOT");
        first_connect = false;
        set_dev_state(DEV_IDLE);
        dbg_println("Boot->idle");
      }
      else
      {
        send_action("RESET", "IDLE");
        set_dev_state(DEV_IDLE);
        dbg_println(F("Reset->idle"));
      }
    }
    else
    {
      // If state is ACTIVE, this won't have any effect
      set_dev_state(DEV_NO_CONN);
    }
  }
}

void setup()
{
  wdt_disable();

  // for I2C port expanders
  Wire.begin();

  char build_ident[17]="";
  Serial.begin(9600);
  dbg_println(F("Start!"));
  _serial_state = SS_MAIN_MENU;
  set_dev_state(DEV_NO_CONN);

  pinMode(PIN_LED0, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(PIN_INPUT_INT, INPUT);

  digitalWrite(PIN_LED0, HIGH);
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(PIN_LED3, HIGH);
  digitalWrite(PIN_INPUT_INT, HIGH);

  dbg_println(F("Init SPI"));
  SPI.begin();

  // Read settings from eeprom
  for (int i = 0; i < 6; i++)
    _mac[i] = EEPROM.read(EEPROM_MAC+i);

  for (int i = 0; i < 4; i++)
    _ip[i] = EEPROM.read(EEPROM_IP+i);

  for (int i = 0; i < 4; i++)
    _server[i] = EEPROM.read(EEPROM_SERVER_IP+i);

  for (int i = 0; i < 40; i++)
    _base_topic[i] = EEPROM.read(EEPROM_BASE_TOPIC+i);
  _base_topic[40] = '\0';

  for (int i = 0; i < 20; i++)
    _dev_name[i] = EEPROM.read(EEPROM_NAME+i);
  _dev_name[20] = '\0';

  _input_enables = EEPROM.read(EEPROM_INPUT_ENABLES);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 4; j++) {
      _override_masks[i] |= (uint32_t)EEPROM.read(EEPROM_OVERRIDE_MASKS+(i*4)+j) << (j*8);
      _override_states[i] |= (uint32_t)EEPROM.read(EEPROM_OVERRIDE_STATES+(i*4)+j) << (j*8);
    }
  }

  _input_statefullness = EEPROM.read(EEPROM_INPUT_STATEFULL);

  sprintf(tool_topic, "%s/%s", _base_topic, _dev_name);

  dbg_println(F("Init LCD"));
  lcd.init();
  lcd.backlight();
  snprintf(build_ident, sizeof(build_ident), "FW:%s", BUILD_IDENT);
  lcd_display(build_ident);
  dbg_println(build_ident);
  lcd_display(_dev_name,1, false);

  dbg_println(F("Start Ethernet"));
  Ethernet.begin(_mac, _ip);

  _client = new PubSubClient(_server, MQTT_PORT, callbackMQTT, _ethClient);

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  checkMQTT();

  _input_int_pushed = false;
  _input_int_pushed_time = 0;

  attachInterrupt(0, input_int, FALLING); // int0 = PIN_INPUT_INT

  delay(100);

  publish_all_input_states();

  dbg_println(F("Setup done..."));

  Serial.println();
  serial_show_main_menu();
  wdt_enable(WDTO_8S);

  // read and bin the input port expander to clear any old interrupt
  Wire.requestFrom(PCF_BASE_ADDRESS | 3 , 1);
  Wire.read();

} // end void setup()

void input_int()
{
  _input_int_pushed_time = millis();
  _input_int_pushed = true;
}

void state_led_loop()
{
  static unsigned long led_last_change = 0;
  static boolean led_on = false;

  switch (_dev_state)
  {
    case DEV_NO_CONN:
      /* Flash LED. For auth & induct wait, if all goes well, the LED won't have time to flash */
      if (millis()-led_last_change > STATE_FLASH_FREQ)
      {
        led_last_change = millis();
        if (led_on)
        {
          led_on = false;
          digitalWrite(PIN_LED0, LOW);
        }
        else
        {
          led_on = true;
          digitalWrite(PIN_LED0, HIGH);
        }
      }
      break;

    case DEV_IDLE:
      if (!led_on)
      {
        led_on = true;
        digitalWrite(PIN_LED0, HIGH);
      }
      break;
  }
}

void loop()
{
  wdt_reset();

  // Poll MQTT
  // should cause callback if there's a new message
  _client->loop();

  // are we still connected to MQTT
  checkMQTT();

  // Do serial menu
  serial_menu();

  // Check if any buttons have been pushed
  check_buttons();

  lcd_loop();

  state_led_loop();

} // end void loop()


boolean set_dev_state(dev_state_t new_state)
{
  static bool first_boot = true;
  boolean ret = true;

  switch (new_state)
  {
  case DEV_NO_CONN:
    if ((_dev_state == DEV_IDLE))
    {
      dbg_println(F("NO_CONN"));
      _dev_state =  DEV_NO_CONN;
      lcd_display(F("No network"));
      lcd_display(F("conection!"), 1, false);
    }
    else
      ret = false;
    break;

  case DEV_IDLE:
    // Any state can change to idle

    first_boot = false;
    _dev_state =  DEV_IDLE;
    dbg_println(F("IDLE"));
    break;

  default:
    ret = false;
    break;
  }

  if (ret)
    _last_state_change = millis();

  return ret;
}

void lcd_loop()
{
  static unsigned long last_time_display = 0;

}

void check_buttons()
{
  if (_input_int_pushed)
  {
    if (millis() - _input_int_pushed_time > 50)
    {
      _input_int_pushed = false;

      read_inputs();
    }
  } else if ((millis() - _input_int_pushed_time) > 200 && (digitalRead(PIN_INPUT_INT) == LOW)) {
    // its been a while and for some reason the interrupt is low and likely meaning we missed an interrupt
    _input_int_pushed = false;

    read_inputs();
  }
}

// Publish <msg> to topic "<tool_topic>/<act>"
void send_action(const char *act, char *msg)
{
  int tt_len = strlen(tool_topic);
  tool_topic[tt_len] = '/';
  strcpy(tool_topic+tt_len+1, act);
  _client->publish(tool_topic, msg);
  tool_topic[tt_len] = '\0';
}

void lcd_display(const __FlashStringHelper *n, short line, boolean wipe_display)
{
  uint8_t c;
  byte payload[LCD_WIDTH+1];
  char *pn = (char*)n;
  int count=0;

  memset(payload, 0, sizeof(payload));

  while (((c = pgm_read_byte_near(pn++)) != 0) && (count < sizeof(payload)))
    payload[count++] = c;

  lcd_display((char*)payload, line, wipe_display);
}

void lcd_display(char *msg, short line, boolean wipe_display)
{
  if (wipe_display)
    lcd.clear();

  lcd.setCursor(0, line);

  lcd.print(msg);
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

#ifdef DEBUG_MQTT
  //  _client.publish(P_NOTE_TX, _pmsg);
#endif

  Serial.println(_pmsg+sizeof("INFO"));
}

void dbg_println(const char *msg)
{/*
  byte *payload;

 memset(_pmsg, 0, sizeof(_pmsg));
 strcpy(_pmsg, "INFO:");
 strcat(_pmsg, msg);
 payload = (byte*)_pmsg + sizeof("INFO");

 Serial.println(_pmsg+sizeof("INFO"));
 */
  Serial.println(msg);
}

