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


/* Arduino note acceptor for Nottingham Hackspace
 *
 * Target board = Arduino Uno, Arduino version = 1.0.1
 *
 * Expects to be connected to:
 *   - I2C 20x4 LCD (HCARDU0023)
 *   - SPI RFID reader (HCMODU0016)
 *   - Ethernet shield
 * Pin assignments are in Config.h
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
byte mac[6]; //    = { 0xDE, 0xED, 0xBA, 0xFE, 0x65, 0xF0 };
byte ip[4]; //     = { 192, 168, 0, 21 };


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
      dbg_println(F("Connected to MQTT"));
//      set_state(STATE_READY);
      client.publish(P_STATUS, RESTART);
      client.subscribe(S_NOTE_RX);
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

void serial_menu()
{
  static char serial_line[65];
  static int i = 0;
  char c;
  
  while (Serial.available())
  {
    if (i == 0)
      memset(serial_line, 0, sizeof(serial_line));
      
    c = Serial.read();
    if ((c=='\n') || (c=='\r'))
    {
      serial_process(serial_line);
      //Serial.print("OK: ");
      //Serial.println(serial_line);
      i = 0;
    } else if (i < (sizeof(serial_line)-1))
    {
      serial_line[i++] = c;
    }
  }
}

void serial_process(char *cmd)
{
  if (serial_state == SS_MAIN_MENU)
    serial_main_menu(cmd);
  else if (serial_state == SS_SET_MAC)
  {
    serial_set_mac(cmd);  
    serial_main_menu("0");
  }
  else if (serial_state == SS_SET_IP)
  {
    serial_set_ip(cmd);  
    serial_main_menu("0");
  }
  else if (serial_state == SS_SET_NAME)
  {
    serial_set_name(cmd);  
    serial_main_menu("0");
  }
  else if (serial_state == SS_SET_TOPIC)
  {
    serial_set_topic(cmd);  
    serial_main_menu("0");
  }
}

void serial_main_menu(char *cmd)
{
  int i = atoi(cmd);
  
  switch (i)
  {
    case 1: // "[ 1 ] Show current settings"
      serial_show_settings();
      Serial.println();
      serial_show_main_menu();      
      break;
      
    case 2: // "[ 2 ] Set MAC address"
      serial_set_mac(NULL);
      break;
      
    case 3: // "[ 3 ] Set IP address"
      serial_set_ip(NULL);
      break;
      
    case 4: // "[ 4 ] Set name"
      serial_set_name(NULL);
      break;    
    
    case 5: // "[ 5 ] Set base topic"
      serial_set_topic(NULL);
      break;    
      
    case 6:
      Serial.println("Reboot....");
      wdt_enable(WDTO_2S);
      while(1);
      break;
      
    default:
      Serial.println();
      serial_show_main_menu();
  }
}

void serial_show_main_menu()
{
  Serial.println();
  Serial.println(F("Main menu"));
  Serial.println(F("---------"));
  Serial.println(F("[ 1 ] Show current settings"));
  Serial.println(F("[ 2 ] Set MAC address"));
  Serial.println(F("[ 3 ] Set IP address"));
  Serial.println(F("[ 4 ] Set name"));
  Serial.println(F("[ 5 ] Set base topic"));
  Serial.println(F("[ 6 ] Reset/reboot"));
  Serial.print(F("Enter selection: "));  
}

void serial_show_settings()
{
  char buf[18];

  Serial.println("\n");
  Serial.println(F("Current settings:"));
  
  Serial.print(F("MAC address: "));
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(buf);
 
  Serial.print(F("IP address : "));
  sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  Serial.println(buf);
  
  Serial.print(F("Name       : "));
  Serial.println(dev_name);  
  
  Serial.print(F("Base topic : "));
  Serial.println(base_topic);
}

void serial_set_mac(char *cmd)
{
  serial_state = SS_SET_MAC;
  byte mac_addr[6];
  int c;
  
  if (cmd == NULL)
  {
    Serial.print(F("\nEnter MAC address:"));
  } else
  {
    // MAC address entered - validate
    c = sscanf(cmd, "%x:%x:%x:%x:%x:%x", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
    if (c == 6)
    {
      Serial.println(F("\nOk - Saving address"));
      set_mac(mac_addr);
    } else
    {
      Serial.println(F("\nInvalid MAC address"));
    }
    // Return to main menu
    serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_ip(char *cmd)
{
  serial_state = SS_SET_IP;
  int ip_addr[4];
  int c;
  byte ip_good = true;
  
  if (cmd == NULL)
  {
    Serial.print(F("\nEnter IP address:"));
  } else
  {
    // IP address entered - validate
    c = sscanf(cmd, "%d.%d.%d.%d", &ip_addr[0], &ip_addr[1], &ip_addr[2], &ip_addr[3]);
    if (c != 4)
    {
      ip_good = false;
    } else
    {
      for (int i=0; i < 4; i++)
        if (ip_addr[i] < 0 || ip_addr[i] > 255)
          ip_good = false;
    }
      
    if (ip_good)
    {     
      Serial.println(F("\nOk - Saving address"));
      set_ip(ip_addr);
    } else
    {
      Serial.println(F("\nInvalid IP address"));
    }
    // Return to main menu
    serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_name(char *cmd)
{
  serial_state = SS_SET_NAME;
  
  if (cmd == NULL)
  {
    Serial.print(F("\nEnter device name:"));
  } else
  {
    if (strlen(cmd) > 2)
    {    
      Serial.println(F("\nOk - Saving name"));
      set_name(cmd);
    } else
    {
      Serial.println(F("\nError: too short"));
    }
    // Return to main menu
    serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_topic(char *cmd)
{
  serial_state = SS_SET_TOPIC;
  
  if (cmd == NULL)
  {
    Serial.print(F("\nEnter base topic:"));
  } else
  {
    if (strlen(cmd) > 2)
    {    
      Serial.println(F("\nOk - Saving topic"));
      set_topic(cmd);
    } else
    {
      Serial.println(F("\nError: too short"));
    }
    // Return to main menu
    serial_state = SS_MAIN_MENU;
  }
  return;
}

void set_mac(byte *mac_addr)
{
  for (int i = 0; i < 6; i++)
  {
    EEPROM.write(EEPROM_MAC+i, mac_addr[i]);
    mac[i] = mac_addr[i];
  }
}

void set_ip(int *ip_addr)
{
  for (int i = 0; i < 4; i++)
  {
    EEPROM.write(EEPROM_IP+i, ip_addr[i]);
    ip[i] = ip_addr[i];
  }
}

void set_name(char *new_name)
{
  for (int i = 0; i < 20; i++)
  {
    EEPROM.write(EEPROM_NAME+i, new_name[i]);
    dev_name[i] = new_name[i];
  }
}

void set_topic(char *new_topic)
{
  for (int i = 0; i < 40; i++)
  {
    EEPROM.write(EEPROM_BASE_TOPIC+i, new_topic[i]);
    base_topic[i] = new_topic[i];
  }
}
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

