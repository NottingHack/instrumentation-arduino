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
#include "Tools.h"
#include "Menu.h"


#define LCD_WIDTH 16



EthernetClient _ethClient;
PubSubClient *_client;
LiquidCrystal_I2C lcd(0x27,16,2);  // 16x2 display at address 0x27
MFRC522 _rfid_reader(PIN_RFID_SS, PIN_RFID_RST);

dev_state_t    _dev_state;

char _pmsg[DMSG];



unsigned long _tool_start_time;
unsigned long _auth_start;
volatile boolean _induct_button_pushed;
volatile unsigned long _induct_button_pushed_time;
volatile boolean _signoff_button_pushed;
volatile unsigned long _signoff_button_pushed_time;
boolean _can_induct;
rfid_uid _last_rejected_card = {0};
unsigned long _last_rejected_read = 0;
rfid_uid _card_number;
boolean _authd_card_present = false;
unsigned long _authd_card_last_seen = 0; // How long ago was the card used to active the tool last see
rfid_uid _inductor_card = {0};
boolean _just_inducted = false; // used to ensure that when a member is inducted, they're not immiedaley signed on. Instead require the card to be removed (so we see no card), before allowing it to be used to sign on 
int _rfid_polls_without_card=0;
char tool_topic[20+40+10+4]; // name + _base_topic + action + delimiters + terminator
unsigned long _last_state_change = 0;
boolean _disp_active_wiped = false;
boolean _deny_reason_displed = false;
unsigned long _last_rfid_reset=0;

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

  // Messages to the tools topic
  if (!strncmp(topic, tool_topic, strlen(tool_topic)))
  {
    // get action
    if (strlen(topic) < strlen(tool_topic)+3)
      return;
    strncpy(buf, topic+strlen(tool_topic)+1, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';

    // For the GRANT message, the first character of the payload should be 
    // either "U", "I" or "M" (User, Inductor or Maintainer). The reset is
    // a message to be displayed on the LCD
    if (strcmp(buf, "GRANT") == 0)
    {
      if (length < 2)
        return;
        
      // Ensure message/string is null terminated
      payload[length] = '\0';
        
      // Check if member can induct others. For now, maintainer & inductor's are treated equally
      if ((payload[0] == 'I') || (payload[0] == 'M'))
        _can_induct = true;
      else 
        _can_induct = false;      
      
      // Show rest of received message on LCD
      lcd_display_mqtt((char*)payload+1);
      
      // Enable tool!
      set_dev_state(DEV_ACTIVE);
    } 

    if (strcmp(buf, "DENY") == 0)
    {
      // Card's been rejected. Go back to idle.
      set_dev_state(DEV_IDLE);
      dbg_println(F("Card rejected!"));
      cpy_rfid_uid(&_last_rejected_card, &_card_number);
      _last_rejected_read = millis();
      
      // Show message on LCD
      payload[length] = '\0';
      lcd_display((char*)payload);     
     _deny_reason_displed = true; 
    } 
    
    if (strcmp(buf, "ISUC") == 0) // induct success
    {
      // Induction complete - go back to idle
      set_dev_state(DEV_IDLE);      
      _just_inducted = true;
      _rfid_polls_without_card = 0;
    }
    
    if (strcmp(buf, "IFAL") == 0) // induct failed
    {
      // Induct failed (e.g. unknown card). Go back to DEV_INDUCT from INDUCT_WAIT
      set_dev_state(DEV_INDUCT);
      payload[length] = '\0';
      lcd_display((char*)payload);
      Serial.println((char*)payload);
    }
  }

} // end void callback(char* topic, byte* payload,int length)

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

      // Update state - but be sure not to deactive tool if connection was lost/restored whilst in use
      if (first_connect)
      {
        send_action("RESET", "BOOT");
        first_connect = false;   
        set_dev_state(DEV_IDLE);      
        dbg_println("Boot->idle");  
      }
      else if (_dev_state == DEV_ACTIVE)
      {
        send_action("RESET", "ACTIVE");
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
  char build_ident[17]="";
  Serial.begin(9600);
  dbg_println(F("Start!"));
  _serial_state = SS_MAIN_MENU;
  set_dev_state(DEV_NO_CONN);

  pinMode(PIN_RELAY_RESET, OUTPUT);
  pinMode(PIN_RELAY_SET  , OUTPUT);
  pinMode(TOOL_POWER_LED, OUTPUT);   
  pinMode(PIN_INDUCT_LED, OUTPUT); 
  pinMode(PIN_INDUCT_BUTTON, INPUT);
  pinMode(PIN_SIGNOFF_BUTTON, INPUT);
  pinMode(PIN_STATE_LED, OUTPUT);

  digitalWrite(PIN_RELAY_RESET, LOW);
  digitalWrite(PIN_RELAY_SET  , LOW);
  digitalWrite(TOOL_POWER_LED, LOW); 
  digitalWrite(PIN_INDUCT_LED, LOW);
  digitalWrite(PIN_INDUCT_BUTTON, HIGH);
  digitalWrite(PIN_SIGNOFF_BUTTON, HIGH); 

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

  dbg_println(F("Init RFID"));
  _rfid_reader.PCD_Init();

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  checkMQTT();

  _signoff_button_pushed = false;
  _signoff_button_pushed_time = 0;
  _induct_button_pushed = false;
  _induct_button_pushed_time = 0;

  attachInterrupt(0, signoff_button, FALLING); // int0 = PIN_SIGNOFF_BUTTON
  attachInterrupt(1, induct_button , FALLING); // int1 = PIN_INDUCT_BUTTON 

  delay(100);

  dbg_println(F("Setup done..."));

  Serial.println();
  serial_show_main_menu();
  wdt_enable(WDTO_8S);
} // end void setup()

void signoff_button()
{
  _signoff_button_pushed_time = millis();
  _signoff_button_pushed = true;
}

void induct_button()
{
  _induct_button_pushed_time = millis();
  _induct_button_pushed = true;
}

void induct_loop()
{
  static unsigned long led_last_change = 0;
  static boolean led_state = false;
  
  if (_dev_state == DEV_INDUCT)
  {
    if (millis()-led_last_change > INDUCT_FLASH_FREQ)
    {
      led_last_change = millis();
      if (led_state)
      {
        led_state = false;
        digitalWrite(PIN_INDUCT_LED, HIGH);
      }
      else
      {
        led_state = true;
        digitalWrite(PIN_INDUCT_LED, LOW);
      }
    }
    
    // Don't stay in the induct state forever if no card is presented
    if ((millis() - _last_state_change) > INDUCT_TIMEOUT*1000)
    {
      dbg_println(F("Induct timeout"));
      set_dev_state(DEV_IDLE);
    }
  }
}

void state_led_loop()
{
  static unsigned long led_last_change = 0;
  static boolean led_on = false;
  
  switch (_dev_state)
  {
    case DEV_NO_CONN:
    case DEV_AUTH_WAIT:
    case DEV_INDUCT_WAIT:
      /* Flash LED. For auth & induct wait, if all goes well, the LED won't have time to flash */
      if (millis()-led_last_change > STATE_FLASH_FREQ)
      {
        led_last_change = millis();
        if (led_on)
        {
          led_on = false;
          digitalWrite(PIN_STATE_LED, LOW);
        }
        else
        {
          led_on = true;
          digitalWrite(PIN_STATE_LED, HIGH);
        }
      }
      break;
      
    case DEV_IDLE:
    case DEV_ACTIVE:
    case DEV_INDUCT:
      if (!led_on)
      {
        led_on = true;
        digitalWrite(PIN_STATE_LED, HIGH);
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

  // Check for RFID card
  poll_rfid();

  // Do serial menu
  serial_menu();

  // Check if either the sign-off or induct buttons have been pushed
  check_buttons();
  
  induct_loop();
  
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
    if ((_dev_state == DEV_IDLE) || (_dev_state == DEV_AUTH_WAIT) || (_dev_state == DEV_INDUCT))
    {
      dbg_println("NO_CONN");
      _dev_state =  DEV_NO_CONN;
      lcd_display(F("No network"));
      lcd_display(F("conection!"), 1, false);
    }
    else
      ret = false;
    break;

  case DEV_IDLE:
    // Any state can change to idle
    
   // If this is the first boot, don't change the relay state. This should mean if the tool power is on,
   // and we reboot, power won't be cut on power up (and should sign straight back on again if the card
   // is still on the reader).
    if (!first_boot)
      relay_off();
    first_boot = false;
    digitalWrite(PIN_INDUCT_LED, LOW);
    if (_dev_state == DEV_ACTIVE)
      send_action("COMPLETE", "0");
    _dev_state =  DEV_IDLE;
    dbg_println(F("IDLE"));
    lcd_display(F("Scan RFID card"));
    break;

  case DEV_AUTH_WAIT:
    if (_dev_state == DEV_IDLE)
    {
      _dev_state =  DEV_AUTH_WAIT;
      _auth_start = millis();
      dbg_println("AUTH_WAIT");
      lcd_display(F("Checking..."));
    }
    else
      ret = false;
    break;

  case DEV_ACTIVE:
    if (_dev_state == DEV_AUTH_WAIT)
    {
      _dev_state = DEV_ACTIVE;
      relay_on();
      _tool_start_time = millis();
      dbg_println(F("ACTIVE"));
      
      _disp_active_wiped = false;
      if (_can_induct)
        digitalWrite(PIN_INDUCT_LED, HIGH);
    }
    else
      ret = false;
    break;

  case DEV_INDUCT:
    if ((_dev_state != DEV_ACTIVE && _dev_state != DEV_INDUCT_WAIT) || !_can_induct)
      ret = false;
    else
    {
      if (_dev_state == DEV_ACTIVE)
      {
        send_action("COMPLETE", "0");      
        cpy_rfid_uid(&_inductor_card, &_card_number);
        relay_off();       
        lcd_display(F("Scan inductees"));
        lcd_display(F("RFID card....."), 1, false);
      }
      _dev_state = DEV_INDUCT;
    }
    break;
    
  case DEV_INDUCT_WAIT:
    if (_dev_state != DEV_INDUCT)
      ret = false;
    else
    {
      _dev_state = DEV_INDUCT_WAIT;
      digitalWrite(PIN_INDUCT_LED, HIGH);
      //lcd_display(F("Ok, recording..."));   
    }
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
  
  if ((millis() - _last_rfid_reset) > RFID_RESET_INTERVAL)
  {
    _last_rfid_reset = millis();
    _rfid_reader.PCD_Init();
    return;
  }

  // If tool's been active for more than 5 seconds, wipe the second line of the LCD
  // (this shows pledged time remaining, but doesn't count down)
  if ((_dev_state == DEV_ACTIVE) && ((millis()-_last_state_change) > 5000) && (!_disp_active_wiped))
  {
    _disp_active_wiped = true;
    lcd.setCursor(0, 1);
    lcd.print(F("                "));
  }
  
  // If the tools been active for more than 5 seconds, show total active time & refresh every second.
  if ((_dev_state == DEV_ACTIVE) && _disp_active_wiped && (millis()-last_time_display > 1000))
  {
    last_time_display = millis();
    lcd.setCursor(0, 1);
    lcd.print(time_diff_to_str(_last_state_change, millis()));
  }
  
  // The last card scanned was rejected, the LCD will be showing the reason. If more than 5
  // seconds have past, clear the reason.
  if ((_dev_state == DEV_IDLE) && ((millis()-_last_state_change) > 5000) && _deny_reason_displed)
  {
    _deny_reason_displed = false;
    lcd_display(F("Scan RFID card"));
  }
}

void poll_rfid()
{
  static MFRC522::Uid card;	 
  char rfid_serial[20];
  boolean got_card = false;

  if ((_dev_state != DEV_IDLE) && (_dev_state != DEV_ACTIVE) && (_dev_state != DEV_INDUCT))
    return;

  // If idle, poll for any/new card
  if (_dev_state == DEV_IDLE)
  {
    _authd_card_present = false;
    memset(&card, 0, sizeof(card));

    // Look for RFID card
    if (_rfid_reader.PICC_IsNewCardPresent())
    {
      Serial.println("card present");
      if (_rfid_reader.PICC_ReadCardSerial())
      {
        Serial.println("GOT CARD");
        got_card=true;     
      }
    }
    
    // If there's been no card present for the last 10 polls, clear the just inducted flag
    if ((_rfid_polls_without_card > 10) && _just_inducted)
      _just_inducted = false;
    
    if (got_card)
    {
      _rfid_polls_without_card=0;
    }
    else
    {
      _rfid_polls_without_card++;
      return;
    }
    
    // If someone's just this moment been inducted, give them a chance to remove their card before signing on 
    if (_just_inducted)
      return;

    // store UID in _card_number
    cpy_rfid_uid(&_card_number, &_rfid_reader.uid);

    Serial.println("GOT CARD DATA");

    // If the same card has already been rejected in the last 10s, don't bother querying the server again
    if ((eq_rfid_uid(_card_number, _last_rejected_card)) && ((millis() - _last_rejected_read) < 10000))
      return;

    // Send AUTH request to server
    uid_to_hex(rfid_serial, _card_number);
    sprintf(_pmsg, "%s", rfid_serial);

    send_action("AUTH", _pmsg);

    // Keep a copy of the card ID we're auth'ing
    memcpy(&card, &_rfid_reader.uid, sizeof(card));

    _authd_card_last_seen = millis();
    _authd_card_present = true;

    set_dev_state(DEV_AUTH_WAIT);
  }
  else if (_dev_state == DEV_ACTIVE)
  {
    // If we've seen the card in the last <ACTIVE_POLL_FREQ> ms, don't bother checking for it
    if (millis()-_authd_card_last_seen < ACTIVE_POLL_FREQ)
      return;

    dbg_println(F("Poll for auth'd card"));

    // Poll for card 
    if (_rfid_reader.PICC_IsNewCardPresent())
    {
      if (_rfid_reader.PICC_ReadCardSerial())      
      {
        _rfid_polls_without_card=0;
        // card found & serial read - see if it's for the auth'd card
        boolean match = true;

        for (unsigned int i=0; i < _rfid_reader.uid.size && (i < sizeof(rfid_uid)); i--)
        {          
          if (_rfid_reader.uid.uidByte[i] != card.uidByte[i])
            match = false;
        }
        if (match)
        {
          // auth'd card found - reset card last seen time and return
          _authd_card_last_seen = millis();
          _authd_card_present = true;
          dbg_println(F("Card found"));
          return;
        }
        else
        {
          dbg_println(F("Unexpect card found"));
        }
      }
    }
    _authd_card_present = false;

    // To get here, we've polled for the auth'd card, but not found it.
    // If we're now beyond the preset session timout, disable tool by
    // switching back to IDLE state; otherwise... wait some more.
    if ((millis() - _authd_card_last_seen) > TIMEOUT_SES)
    {
      send_action("INFO", "Session timeout!");
      set_dev_state(DEV_IDLE);
      dbg_println(F("ses to->idle"));
    }  
  } //end if active   
  else if (_dev_state == DEV_INDUCT)
  {
    // get card other than the inductors card
    // Poll for card 
    if (_rfid_reader.PICC_IsNewCardPresent())
    {
      if (_rfid_reader.PICC_ReadCardSerial())      
      {
        _rfid_polls_without_card=0;
        rfid_uid new_card_number;

        cpy_rfid_uid(&new_card_number, &_rfid_reader.uid);

        if (eq_rfid_uid(new_card_number, _inductor_card))
        {
          _authd_card_present = true;
        }
        else
        {
          // Card seen that isn't the inductors card, so send induction request to server
          induct_member(new_card_number, _inductor_card);
        }
      }
      else
        _authd_card_present = false;
    }
    else
      _authd_card_present = false;
  }
}

int induct_member(rfid_uid inductee, rfid_uid inductor)
{
  char buf[22+22+2]="";

  char inductorStr[22] = "";
  char inducteeStr[22] = "";
  uid_to_hex(inductorStr, inductor);
  uid_to_hex(inducteeStr, inductee);

  // Send induct message to server
  sprintf(buf, "%s:%s", inductorStr, inducteeStr);
  send_action("INDUCT", buf);

  // Set state to waiting
  set_dev_state(DEV_INDUCT_WAIT);
  
  return 0;
}

void check_buttons()
{
  if (_signoff_button_pushed)
  {
    if (millis() - _signoff_button_pushed_time > 50)
    {
      _signoff_button_pushed = false;
    
      // If the card is still on the reader, don't signoff as we'll sign straight back on again after it's read
      if (( (!_authd_card_present) && (millis()-_authd_card_last_seen > 1500) && (digitalRead(PIN_SIGNOFF_BUTTON) == LOW)  ) || 
            (_dev_state == DEV_AUTH_WAIT) )
            {
              set_dev_state(DEV_IDLE);
            }
    }
  }
  
  if (_induct_button_pushed)
  {
    if (millis() - _induct_button_pushed_time > 50)
    {
      _induct_button_pushed = false;
      
      if ( (_can_induct) && (_dev_state == DEV_ACTIVE) && (digitalRead(PIN_INDUCT_BUTTON) == LOW) )
      {
        set_dev_state(DEV_INDUCT); 
      }
    }
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

char *time_diff_to_str(unsigned long start_time, unsigned long end_time)
{
  unsigned int seconds = 0;
  unsigned int minutes = 0;
  unsigned int hours   = 0;
  unsigned long total_millis = end_time - start_time;
  unsigned long total_seconds = total_millis / 1000;
  static char time_str[17]  = "";

  hours = total_seconds / 60 / 60;
  minutes = (total_seconds - (hours*60*60)) / 60;
  seconds = total_seconds - (hours*60*60) - (minutes*60);

  sprintf(time_str, "Session %.2d:%.2d:%.2d", hours, minutes, seconds);
  return time_str;
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

void relay_on()
{
  digitalWrite(TOOL_POWER_LED, HIGH);
  dbg_println(F("Relay ON"));
  digitalWrite(PIN_RELAY_SET, HIGH);
  delay(100);
  digitalWrite(PIN_RELAY_SET, LOW);
}

void relay_off()
{
  dbg_println(F("Relay OFF"));
  digitalWrite(PIN_RELAY_RESET, HIGH);
  delay(100);
  digitalWrite(PIN_RELAY_RESET, LOW);
  digitalWrite(TOOL_POWER_LED, LOW);
}

void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src)
{
  memcpy(dst, src, sizeof(rfid_uid));
}

bool eq_rfid_uid(rfid_uid u1, rfid_uid u2)
{
  if (memcmp(&u1, &u2, sizeof(rfid_uid)))
    return false;
  else
    return true;
}

void uid_to_hex(char *uidstr, rfid_uid uid)
{
  switch (uid.size)
  {
    case 4:
      sprintf(uidstr, "%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3]);
      break;

    case 7:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6]);
      break;

    case 10:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6], uid.uidByte[7], uid.uidByte[8], uid.uidByte[9]);
      break;

    default:
      sprintf(uidstr, "ERR:%d", uid.size);
      break;
  }
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

