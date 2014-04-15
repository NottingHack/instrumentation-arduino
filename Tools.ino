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
 *   - Ethernet shield
 * Pin assignments are in Config.h
 *
 * Config (IP, MAC, etc) is done using serial @ 9600
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

#define LCD_WIDTH 20

void callbackMQTT(char* topic, byte* payload, unsigned int length);

void mqtt_rx_display(char *payload);
void poll_rfid();
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);

EthernetClient ethClient;
PubSubClient client(server, MQTT_PORT, callbackMQTT, ethClient);
//LiquidCrystal_I2C lcd(0x27,20,4);  // 20x4 display at address 0x27
MFRC522 rfid_reader(PIN_RFID_SS, PIN_RFID_RST);

serial_state_t serial_state;
dev_state_t    dev_state;

char pmsg[DMSG];
unsigned long card_number;
char rfid_serial[20];
char tran_id[10]; // transaction id
char mqtt_rx_buf[30];
char base_topic[41];
char dev_name [21];

// MAC / IP address of device
byte mac[6];
byte ip[4];

unsigned long _tool_start_time;
unsigned long _auth_start;

char tool_topic[20+40+10+4]; // name + base_topic + action + delimiters + terminator


/** TODO: complete message. Track time. SIGNOFF action */

/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
{
  char buf [30];
  
  if (length >= sizeof(mqtt_rx_buf))
    return;
    
  // Respond to status requests
  else if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
      dbg_println(F("Status Request"));
      sprintf(buf, "Running: %s", dev_name);      
      client.publish(P_STATUS, buf);
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
    
    if (strcmp(buf, "GRANT"))
    {
      // Enable tool!
      set_dev_state(DEV_ACTIVE);
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
  
  if (!client.connected()) 
  {
    if (client.connect(CLIENT_ID)) 
    {
      char buf[30];
      
      dbg_println(F("Connected to MQTT"));
      sprintf(buf, "Restart: %s", dev_name);
      client.publish(P_STATUS, buf);
      
      // Subscribe to <tool_topic>/#, without having to declare another large buffer just for this.
      pToolTopic = tool_topic + strlen(tool_topic);
      *pToolTopic     = '/';
      *(pToolTopic+1) = '#';
      *(pToolTopic+2) = '\0';
      Serial.println(tool_topic);
      client.subscribe(tool_topic);
      *pToolTopic = '\0';

      Serial.println(S_STATUS);
      client.subscribe(S_STATUS); 
   
      // Update state - but be sure not to deactive tool if connection was lost/restored whilst in use
      if (first_connect)
      {
        send_action("RESET", "BOOT");
        first_connect = false;   
        set_dev_state(DEV_IDLE);        
      }
      else if (dev_state == DEV_ACTIVE)
      {
        send_action("RESET", "ACTIVE");
      } else
      {
        send_action("RESET", "IDLE");
        set_dev_state(DEV_IDLE);
      }
    } else
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
  serial_state = SS_MAIN_MENU;
  set_dev_state(DEV_NO_CONN);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_INDUCT_BUTTON, INPUT);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_INDUCT_BUTTON, HIGH);

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
    mac[i] = EEPROM.read(EEPROM_MAC+i);
    
  for (int i = 0; i < 4; i++)
    ip[i] = EEPROM.read(EEPROM_IP+i);
    
  for (int i = 0; i < 40; i++)
    base_topic[i] = EEPROM.read(EEPROM_BASE_TOPIC+i);
  base_topic[40] = '\0';
  
  for (int i = 0; i < 20; i++)
    dev_name[i] = EEPROM.read(EEPROM_NAME+i);
  dev_name[20] = '\0';
  
  sprintf(tool_topic, "%s/%s", base_topic, dev_name);

  dbg_println(F("Start Ethernet"));
  Ethernet.begin(mac, ip);

  dbg_println(F("Init RFID"));
 // rfid_reader.PCD_Init();

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
  checkMQTT();

  delay(100);

  dbg_println(F("Setup done..."));

  Serial.println();
  serial_show_main_menu();  
} // end void setup()



void loop()
{
  // Poll MQTT
  // should cause callback if there's a new message
  client.loop();

  // are we still connected to MQTT
  checkMQTT();

  // Check for RFID card
  poll_rfid();
  
  // Do serial menu
  serial_menu();


} // end void loop()


boolean set_dev_state(dev_state_t new_state)
{
  boolean ret = true;
  
  switch (new_state)
  {
    case DEV_NO_CONN:
      if ((dev_state == DEV_IDLE) || (dev_state == DEV_AUTH_WAIT))
        dev_state =  DEV_NO_CONN;
      else
        ret = false;
      break;
      
    case DEV_IDLE:
      // Any state can change to idle
      digitalWrite(PIN_RELAY, LOW);
      dev_state =  DEV_IDLE;
      break;
      
    case DEV_AUTH_WAIT:
      if (dev_state == DEV_IDLE)
      {
        dev_state =  DEV_AUTH_WAIT;
        _auth_start = millis();
      }
      else
        ret = false;
      break;
      
    case DEV_ACTIVE:
      if (dev_state == DEV_AUTH_WAIT)
      {
        dev_state = DEV_ACTIVE;
        digitalWrite(PIN_RELAY, HIGH);
        _tool_start_time = millis();
      }
      else
        ret = false;
      break;
      
    default:
      ret = false;
      break;
  }
  
  return ret;
}

void poll_rfid()
{
  MFRC522::Uid card;	
  static unsigned long authd_card_last_seen = 0; // How long ago was the card used to active the tool last seen
  static boolean authd_card_present = false;
  
  byte *pCard_number = (byte*)&card_number;

  if ((dev_state != DEV_IDLE) && (dev_state == DEV_ACTIVE))
    return;
    
  // If idle, poll for any/new card
  if (dev_state == DEV_IDLE)
  {
    int tt_len; // tool topic string length
    authd_card_present = false;
    memset(&card, 0, sizeof(card));
    
    if (!rfid_reader.PICC_IsNewCardPresent())
      return;
  
    if (!rfid_reader.PICC_ReadCardSerial())
      return;      

    // Convert 4x bytes received to long (4 bytes)
    for (int i = 3; i >= 0; i--) 
      *(pCard_number++) = rfid_reader.uid.uidByte[i];
  
    // Send AUTH request to server
    ultoa(card_number, rfid_serial, 10);
    sprintf(pmsg, "%s", rfid_serial);    
    send_action("AUTH", pmsg);

    // Keep a copy of the card ID we're auth'ing
    memcpy(&card, &rfid_reader.uid, sizeof(card));
    
    authd_card_last_seen = millis();
    authd_card_present = true;
    
    set_dev_state(DEV_AUTH_WAIT);
  }
  else /* ACTIVE */
  {
    // If we've seen the card in the last <ACTIVE_POLL_FREQ> ms, don't bother checking for it
    if (millis()-authd_card_last_seen < ACTIVE_POLL_FREQ)
      return;
    
    // If we saw the card last poll, try polling for the card by id (instead of looking for any) 
    if (authd_card_present) 
    {
      memcpy(&rfid_reader.uid.uidByte, &card, sizeof(rfid_reader.uid.uidByte));
      if (rfid_reader.PICC_ReadCardSerial())
      {
        boolean match = true;
        /* card found - just check it really is the expected card */
        for (int i = 3; i >= 0; i--) 
          if (rfid_reader.uid.uidByte[i] != card.uidByte[i])
            match = false;
            
        if (match)
        {
          /* Ok, expected card found - reset card last seen time and return */
          authd_card_last_seen = millis();
          return;
        }
      }
    }
     
    // Card not found - try polling for any card 
    if (rfid_reader.PICC_IsNewCardPresent())
    {
      if (rfid_reader.PICC_ReadCardSerial())      
      {
       // card found & serial read - see if it's for the auth'd card
        boolean match = true;
        // Test if it's the auth'd card
        for (int i = 3; i >= 0; i--) 
          if (rfid_reader.uid.uidByte[i] != card.uidByte[i])
            match = false;
            
        if (match)
        {
          // auth'd card found - reset card last seen time and return
          authd_card_last_seen = millis();
          authd_card_present = true;
          return;
        }
      }
    }
    
    // To get here, we've polled for the auth'd card, but not found it.
    // If we're now beyond the preset session timout, disable tool by
    // switching back to IDLE state; otherwise... wait some more.
    if ((millis() - authd_card_last_seen) > TIMEOUT_SES)
    {
      send_action("INFO", "Session timeout!");
      set_dev_state(DEV_IDLE);
    }  
  } //end if active   
}

// Publish <msg> to topic "<tool_topic>/<act>"
void send_action(char *act, char *msg)
{
  int tt_len = strlen(tool_topic);
  tool_topic[tt_len] = '/';
  strcpy(tool_topic+tt_len, act);
  client.publish(tool_topic, msg);
  tool_topic[tt_len] = '\0';
}

void dbg_println(const __FlashStringHelper *n)
{
  uint8_t c;
  byte *payload;
  char *pn = (char*)n;

  memset(pmsg, 0, sizeof(pmsg));
  strcpy(pmsg, "INFO:");
  payload = (byte*)pmsg + sizeof("INFO");

  while ((c = pgm_read_byte_near(pn++)) != 0)
    *(payload++) = c;

#ifdef DEBUG_MQTT
//  client.publish(P_NOTE_TX, pmsg);
#endif


  Serial.println(pmsg+sizeof("INFO"));

}

void dbg_println(const char *msg)
{
  byte *payload;

  memset(pmsg, 0, sizeof(pmsg));
  strcpy(pmsg, "INFO:");
  strcat(pmsg, msg);
  payload = (byte*)pmsg + sizeof("INFO");




  Serial.println(pmsg+sizeof("INFO"));

}

