
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
 *   - I2C 20x4 LCD (HCARDU0023) - OPTIONAL
 *   - SPI RFID reader (HCMODU0016)
 *   - Wiznet W5100 based Ethernet shield
 * Pin assignments are in Config.h
 *
 * Configuration (IP, MAC, etc) is done using serial @ 9600. Reset the Arduino after changing the config.
 *
 * Additional required libraries:
 *   - PubSubClient                       - https://github.com/knolleary/pubsubclient
 *   - HCARDU0023_LiquidCrystal_I2C_V2_1  - http://forum.hobbycomponents.com/viewtopic.php?f=39&t=1368
 *   - rfid                               - https://github.com/miguelbalboa/rfid
 */

#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <avr/wdt.h> 
#include "Config.h"

#define LCD_WIDTH 16

void callbackMQTT(char* topic, byte* payload, unsigned int length);

void mqtt_rx_display(char *payload);
void poll_rfid();
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);

EthernetClient _ethClient;
PubSubClient _client(server, MQTT_PORT, callbackMQTT, _ethClient);
//LiquidCrystal_I2C lcd(0x27,20,4);  // 20x4 display at address 0x27
MFRC522 _rfid_reader(PIN_RFID_SS, PIN_RFID_RST);

serial_state_t _serial_state;
dev_state_t    _dev_state;

char _pmsg[DMSG];
char _base_topic[41];
char _dev_name [21];

// MAC / IP address of device
byte _mac[6];
byte _ip[4];

unsigned long _tool_start_time;
unsigned long _auth_start;
volatile boolean _induct_button_pushed;
volatile boolean _signoff_button_pushed;
boolean _can_induct;
unsigned long _last_rejected_card = 0;
unsigned long _last_rejected_read = 0;
unsigned long _card_number;
boolean _authd_card_present = false;
unsigned long _authd_card_last_seen = 0; // How long ago was the card used to active the tool last see

unsigned long _inductor_card = 0;

char tool_topic[20+40+10+4]; // name + _base_topic + action + delimiters + terminator

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
      _client.publish(P_STATUS, buf);
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

    // For the GRANT message, the payload should be either "U", "I" or "M" (User, Inductor or Maintainer) 
    if (strcmp(buf, "GRANT") == 0)
    {
      if (length < 1)
        return;
        
      // Check if member can induct others. For now, maintainer & inductor's are treated equally
      if ((payload[0] == 'I') || (payload[0] == 'M'))
        _can_induct = true;
      else 
        _can_induct = false;      
      
      // Enable tool!
      set_dev_state(DEV_ACTIVE);
    } 

    if (strcmp(buf, "DENY") == 0)
    {
      // Card's been rejected. Go back to idle.
      set_dev_state(DEV_IDLE);
      dbg_println("Card rejected!");
      _last_rejected_card = _card_number;
      _last_rejected_read = millis();
    } 
    
    if (strcmp(buf, "ISUC") == 0) // induct success
    {
      // Induction complete - go back to idle
      set_dev_state(DEV_IDLE);
    }
    
    if (strcmp(buf, "IFAL") == 0) // induct failed
    {
      // Induct failed (e.g. unknown card). Go back to DEV_INDUCT from INDUCT_WAIT
      set_dev_state(DEV_INDUCT);
    }

  }



} // end void callback(char* topic, byte* payload,int length)

void mqtt_rx_display(char *payload)
{
  // Display payload on LCD display - \n in string seperates line 1+2.
  char ln1[LCD_WIDTH+1];
  char ln2[LCD_WIDTH+1];
  char *payPtr, *lnPtr;

  // Skip over "DISP:"
  payload += sizeof("DISP");

  payPtr = (char*)payload;

  memset(ln1, 0, sizeof(ln1));
  memset(ln2, 0, sizeof(ln2)); 

  /* Clear display */
  //lcd.clear();

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

  /* Output 1st line */
  // lcd.setCursor(0, 2);
  //lcd.print(ln1); 

  /* Output 2nd line */
  //  lcd.setCursor(0, 3);
  // lcd.print(ln2);
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

  if (!_client.connected()) 
  {
    if (_client.connect(_dev_name)) 
    {
      char buf[30];

      dbg_println(F("Connected to MQTT"));
      sprintf(buf, "Restart: %s", _dev_name);
      _client.publish(P_STATUS, buf);

      // Subscribe to <tool_topic>/#, without having to declare another large buffer just for this.
      pToolTopic = tool_topic + strlen(tool_topic);
      *pToolTopic     = '/';
      *(pToolTopic+1) = '#';
      *(pToolTopic+2) = '\0';
      Serial.println(tool_topic);
      _client.subscribe(tool_topic);
      *pToolTopic = '\0';

      Serial.println(S_STATUS);
      _client.subscribe(S_STATUS); 

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
        dbg_println("Reset->idle");  
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
  Serial.begin(9600);
  dbg_println(F("Start!"));
  _serial_state = SS_MAIN_MENU;
  set_dev_state(DEV_NO_CONN);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_SIGNOFF_LIGHT, OUTPUT);   
  pinMode(PIN_INDUCT_LED, OUTPUT); 
  pinMode(PIN_INDUCT_BUTTON, INPUT);
  pinMode(PIN_SIGNOFF_BUTTON, INPUT);

  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_SIGNOFF_LIGHT, LOW); 
  digitalWrite(PIN_INDUCT_LED, LOW);
  digitalWrite(PIN_INDUCT_BUTTON, HIGH);
  digitalWrite(PIN_SIGNOFF_BUTTON, HIGH); 


  dbg_println(F("Init LCD"));
  // lcd.init();
  //  lcd.backlight();
  //  lcd.home();
  //  lcd.print(F("Nottinghack note    "));
  // lcd.setCursor(0, 1);
  //  lcd.print(F("acceptor v0.01....  "));

  dbg_println(F("Init SPI"));
  SPI.begin();

  // Read settings from eeprom
  for (int i = 0; i < 6; i++)
    _mac[i] = EEPROM.read(EEPROM_MAC+i);

  for (int i = 0; i < 4; i++)
    _ip[i] = EEPROM.read(EEPROM_IP+i);

  for (int i = 0; i < 40; i++)
    _base_topic[i] = EEPROM.read(EEPROM_BASE_TOPIC+i);
  _base_topic[40] = '\0';

  for (int i = 0; i < 20; i++)
    _dev_name[i] = EEPROM.read(EEPROM_NAME+i);
  _dev_name[20] = '\0';

  sprintf(tool_topic, "%s/%s", _base_topic, _dev_name);

  dbg_println(F("Start Ethernet"));
  Ethernet.begin(_mac, _ip);

  dbg_println(F("Init RFID"));
  _rfid_reader.PCD_Init();

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  checkMQTT();

  _signoff_button_pushed = false;
  _induct_button_pushed = false;

  attachInterrupt(0, signoff_button, FALLING); // int0 = PIN_SIGNOFF_BUTTON
  attachInterrupt(1, induct_button , FALLING); // int1 = PIN_INDUCT_BUTTON 

  delay(100);

  dbg_println(F("Setup done..."));

  Serial.println();
  serial_show_main_menu();  
} // end void setup()

void signoff_button()
{
  _signoff_button_pushed = true;
}

void induct_button()
{
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
  }
  
}

void loop()
{
  // Poll MQTT
  // should cause callback if there's a new message
  _client.loop();

  // are we still connected to MQTT
  checkMQTT();

  // Check for RFID card
  poll_rfid();

  // Do serial menu
  serial_menu();

  // Check if either the sign-off or induct buttons have been pushed
  check_buttons();
  
  induct_loop();

} // end void loop()


boolean set_dev_state(dev_state_t new_state)
{
  boolean ret = true;

  switch (new_state)
  {
  case DEV_NO_CONN:
    if ((_dev_state == DEV_IDLE) || (_dev_state == DEV_AUTH_WAIT) || (_dev_state == DEV_INDUCT))
    {
      dbg_println("NO_CONN");
      _dev_state =  DEV_NO_CONN;
      digitalWrite(PIN_SIGNOFF_LIGHT, LOW);
    }
    else
      ret = false;
    break;

  case DEV_IDLE:
    // Any state can change to idle
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(PIN_SIGNOFF_LIGHT, LOW);
    digitalWrite(PIN_INDUCT_LED, LOW);
    if (_dev_state == DEV_ACTIVE)
      send_action("COMPLETE", "0");
    _dev_state =  DEV_IDLE;
    dbg_println("IDLE");
    break;

  case DEV_AUTH_WAIT:
    if (_dev_state == DEV_IDLE)
    {
      _dev_state =  DEV_AUTH_WAIT;
      _auth_start = millis();
      digitalWrite(PIN_SIGNOFF_LIGHT, HIGH); 
      dbg_println("AUTH_WAIT");
    }
    else
      ret = false;
    break;

  case DEV_ACTIVE:
    if (_dev_state == DEV_AUTH_WAIT)
    {
      _dev_state = DEV_ACTIVE;
      digitalWrite(PIN_RELAY, HIGH);
      _tool_start_time = millis();
      dbg_println("ACTIVE");

      if (_can_induct)
        digitalWrite(PIN_INDUCT_LED, HIGH);
    }
    else
      ret = false;
    break;

  case DEV_INDUCT:
    if (_dev_state != DEV_ACTIVE || !_can_induct)
      ret = false;
    else
    {
      send_action("COMPLETE", "0");      
      _inductor_card = _card_number;
      digitalWrite(PIN_RELAY, LOW);
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
    }
    break;      

  default:
    ret = false;
    break;
  }

  return ret;
}

void poll_rfid()
{
  static MFRC522::Uid card;	

  char rfid_serial[20];

  byte *pCard_number = (byte*)&_card_number;

  if ((_dev_state != DEV_IDLE) && (_dev_state != DEV_ACTIVE) && (_dev_state != DEV_INDUCT))
    return;

  // If idle, poll for any/new card
  if (_dev_state == DEV_IDLE)
  {
    int tt_len; // tool topic string length
    _authd_card_present = false;
    memset(&card, 0, sizeof(card));

    if (!_rfid_reader.PICC_IsNewCardPresent())
      return;

    if (!_rfid_reader.PICC_ReadCardSerial())
      return;      

    // Convert 4x bytes received to long (4 bytes)
    for (int i = 3; i >= 0; i--) 
      *(pCard_number++) = _rfid_reader.uid.uidByte[i];

    ultoa(_card_number, rfid_serial, 10);
    
    // If the same card has already been rejected in the last 10s, don't bother querying the server again
    if ((_card_number == _last_rejected_card) && ((millis() - _last_rejected_read) < 10000))
      return;

    // Send AUTH request to server
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

    dbg_println("Poll for auth'd card");

    // Poll for card 
    if (_rfid_reader.PICC_IsNewCardPresent())
    {
      if (_rfid_reader.PICC_ReadCardSerial())      
      {
        // card found & serial read - see if it's for the auth'd card
        boolean match = true;
        // Test if it's the auth'd card
        for (int i = 3; i >= 0; i--) 
        {          
          if (_rfid_reader.uid.uidByte[i] != card.uidByte[i])
            match = false;
        }
        if (match)
        {
          // auth'd card found - reset card last seen time and return
          _authd_card_last_seen = millis();
          _authd_card_present = true;
          dbg_println("Card found");
          return;
        }
        else
        {
          dbg_println("Unexpect card found");
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
      dbg_println("ses to->idle");
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
        unsigned long new_card_number;
        byte *pNewCard_number = (byte*)&new_card_number;
    
        // Convert 4x bytes received to long (4 bytes)
        for (int i = 3; i >= 0; i--) 
          *(pNewCard_number++) = _rfid_reader.uid.uidByte[i];
          
       if (new_card_number == _inductor_card)
       {
         _authd_card_present = true;
       }
       else
       {
         // Card seen that isn't the inductors card
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

int induct_member(unsigned long inductee, unsigned long inductor)
{
  char buf[40]="";

  // Send induct message to server
  sprintf(buf, "%lu:%lu", inductor, inductee);
  send_action("INDUCT", buf);
  
  // Set state to waiting
  set_dev_state(DEV_INDUCT_WAIT);
  
  return 0;
}

void check_buttons()
{
  if (_signoff_button_pushed)
  {
    _signoff_button_pushed = false;
  
    // If the card is still on the reader, don't signoff as we'll sign straight back on again after it's read
    if ((!_authd_card_present) && (millis()-_authd_card_last_seen > 400))
      set_dev_state(DEV_IDLE);
  }
  
  if (_induct_button_pushed)
  {
    _induct_button_pushed = false;
    if ((_can_induct) && (_dev_state == DEV_ACTIVE))
    {
      set_dev_state(DEV_INDUCT); 
    }
  }

  /* TODO: Induct button */
// Clear  _last_rejected_read
}

// Publish <msg> to topic "<tool_topic>/<act>"
void send_action(char *act, char *msg)
{
  int tt_len = strlen(tool_topic);
  tool_topic[tt_len] = '/';
  strcpy(tool_topic+tt_len+1, act);
  _client.publish(tool_topic, msg);
  tool_topic[tt_len] = '\0';
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




