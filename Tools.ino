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
#include <LiquidCrystal_I2C.h>
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
LiquidCrystal_I2C lcd(0x27,20,4);  // 20x4 display at address 0x27
MFRC522 rfid_reader(PIN_RFID_SS, PIN_RFID_RST);

serial_state_t serial_state;

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

char tool_topic[20+40+1];


/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
{
  if (length >= sizeof(mqtt_rx_buf))
    return;

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
  lcd.setCursor(0, 2);
  lcd.print(ln1); 

  /* Output 2nd line */
  lcd.setCursor(0, 3);
  lcd.print(ln2);
}

/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *
 ****************************************************/
void checkMQTT() 
{
  if (!client.connected()) 
  {
    if (client.connect(CLIENT_ID)) 
    {
      char buf[30];
      
      dbg_println(F("Connected to MQTT"));
      sprintf(buf, "Restart: %s", dev_name);
//      set_state(STATE_READY);
      client.publish(P_STATUS, buf);
      client.subscribe(tool_topic);
      client.subscribe(S_STATUS);
    } else
    {
    //  set_state(STATE_CONN_WAIT);
    }
  }
}

void setup()
{
  wdt_disable();
  Serial.begin(9600);
  dbg_println(F("Start!"));

  serial_state = SS_MAIN_MENU;

  dbg_println(F("Init LCD"));
  lcd.init();
  lcd.backlight();
  lcd.home();
//  lcd.print(F("Nottinghack note    "));
  lcd.setCursor(0, 1);
//  lcd.print(F("acceptor v0.01....  "));

  dbg_println(F("Init SPI"));
 // SPI.begin();
  
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
  
  

  dbg_println(F("Start Ethernet"));
//  Ethernet.begin(mac, ip);

  dbg_println(F("Init RFID"));
 // rfid_reader.PCD_Init();

  // Start MQTT and say we are alive
  dbg_println(F("Check MQTT"));
 // checkMQTT();

  //delay(100);

  dbg_println(F("Setup done..."));
} // end void setup()



void loop()
{
  // Poll MQTT
  // should cause callback if there's a new message
 // client.loop();

  // are we still connected to MQTT
 // checkMQTT();

  // Check for RFID card
  //poll_rfid();
  
  // Do serial menu
  serial_menu();


} // end void loop()


void poll_rfid()
{
  byte *pCard_number = (byte*)&card_number;

  //if (state != STATE_READY)
  //  return;

  if (!rfid_reader.PICC_IsNewCardPresent())
    return;

  if (!rfid_reader.PICC_ReadCardSerial())
    return;

  // Convert 4x bytes received to long (4 bytes)
  for (int i = 3; i >= 0; i--) 
    *(pCard_number++) = rfid_reader.uid.uidByte[i];

  ultoa(card_number, rfid_serial, 10);

  sprintf(pmsg, "AUTH:%s", rfid_serial);
  client.publish(P_NOTE_TX, pmsg);
 // set_state(STATE_AUTH_WAIT);
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
  client.publish(P_NOTE_TX, pmsg);
#endif

#ifdef DEBUG_SERIAL
  Serial.println(pmsg+sizeof("INFO"));
#endif
}

void dbg_println(const char *msg)
{
  byte *payload;

  memset(pmsg, 0, sizeof(pmsg));
  strcpy(pmsg, "INFO:");
  strcat(pmsg, msg);
  payload = (byte*)pmsg + sizeof("INFO");



#ifdef DEBUG_SERIAL
  Serial.println(pmsg+sizeof("INFO"));
#endif
}

