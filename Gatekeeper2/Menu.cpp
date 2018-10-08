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

#include "Arduino.h"
#include "Config.h"
#include "Menu.h"

extern char _base_topic[BASE_TOPIC_LEN];
extern byte _door_id;
extern char _rfid_override[21];

// MAC / IP address of device
extern byte _mac[6];
extern byte _ip[4];
extern byte _server[4];

extern serial_state_t _serial_state;

/* Serial based menu configuration - settings saved in EEPROM */

void serial_menu()
/* Process any serial input since last call - if any - and call serial_process when we have a cr/lf terminated line. */
{
  static char serial_line[41];
  static unsigned int i = 0;
  char c;

  while (Serial.available())
  {
    if (i == 0)
      memset(serial_line, 0, sizeof(serial_line));

    c = Serial.read();
    if ((c=='\n') || (c=='\r'))
    {
      /* CR/LF detected (enter pressed), so process the serial input */
      serial_process(serial_line); 
      i = 0;
    } 
    else if (i < (sizeof(serial_line)-1))
    {
      serial_line[i++] = c;
    }
  }
}

void serial_process(char *cmd)
/* Process input from the serial console. I.e. <cmd> should be what was keyed in prior to pressing enter.
 * <cmd> string is passed on to the revevant menu function, depening on where the user is in the menu. */
{
  if (_serial_state == SS_MAIN_MENU)
    serial_main_menu(cmd);
  else if (_serial_state == SS_SET_MAC)
  {
    serial_set_mac(cmd);  
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_IP)
  {
    serial_set_ip(cmd);  
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_DOOR_ID)
  {
    serial_set_door_id(cmd);
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_TOPIC)
  {
    serial_set_topic(cmd);  
    serial_main_menu("0");
  } 
  else if (_serial_state == SS_SET_SERVER_IP)
  {
    serial_set_server_ip(cmd);
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_EMGCY_RFID)
  {
    serial_set_rfid(cmd);
    serial_main_menu("0");
  }
}

void serial_main_menu(const char *cmd)
/* Respond to supplied input <cmd> for main menu */
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

  case 4: // "[ 4 ] Set server IP address"
    serial_set_server_ip(NULL);
    break;

  case 5: // "[ 5 ] Set door id"
    serial_set_door_id(NULL);
    break;

  case 6: // "[ 6 ] Set base topic"
    serial_set_topic(NULL);
    break;

  case 7: // "[ 7 ] Set RFID override"
    serial_set_rfid(NULL);
    break;

  case 8: // "[ 8 ] Reset/reboot"
    Serial.println("Reboot....");
    wdt_enable(WDTO_2S); // Watchdog abuse...
    while(1);
    break;

  default:
    /* Unrecognised input - redisplay menu */
    Serial.println();
    serial_show_main_menu();
  }
}

void serial_show_main_menu()
/* Output the main menu */
{
  Serial.println();
  Serial.println(F("Main menu"));
  Serial.println(F("---------"));
  Serial.println(F("[ 1 ] Settings"));
  Serial.println(F("[ 2 ] Set MAC"));
  Serial.println(F("[ 3 ] Set IP"));
  Serial.println(F("[ 4 ] Set server IP"));
  Serial.println(F("[ 5 ] Set door id"));
  Serial.println(F("[ 6 ] Set base topic"));
  Serial.println(F("[ 7 ] Set RFID override"));
  Serial.println(F("[ 8 ] Reset"));
  Serial.print(F("Selection: "));
}

void serial_show_settings()
/* Output current config, as read during setup() */
{
  char buf[18];

  Serial.println("\n");
  Serial.println(F("Settings:"));

  Serial.print(F("MAC: "));
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
  Serial.println(buf);

  Serial.print(F("IP: "));
  sprintf(buf, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
  Serial.println(buf);

  Serial.print(F("Server IP  : "));
  sprintf(buf, "%d.%d.%d.%d", _server[0], _server[1], _server[2], _server[3]);
  Serial.println(buf);

  Serial.print(F("Door ID    : "));
  Serial.println(_door_id);

  Serial.print(F("Base topic : "));
  Serial.println(_base_topic);

  Serial.print(F("RFID override : "));
  Serial.println(_rfid_override);
}

void serial_set_mac(char *cmd)
/* Validate (and save, if valid) the supplied mac address */
{
  _serial_state = SS_SET_MAC;
  byte mac_addr[6];
  int c;

  if (cmd == NULL)
  {
    Serial.print(F("\nEnter MAC:"));
  } 
  else
  {
    // MAC address entered - validate
    c = sscanf(cmd, "%x:%x:%x:%x:%x:%x", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
    if (c == 6)
    {
      Serial.println(F("\nOk - Saving"));
      set_mac(mac_addr);
    } 
    else
    {
      Serial.println(F("\nInvalid MAC"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_ip(char *cmd)
/* Validate (and save, if valid) the supplied IP address. Note that validation *
 * is very crude, e.g. there is no rejection of class E, D, loopback, etc      */
{
  _serial_state = SS_SET_IP;
  int ip_addr[4];
  int c;
  byte ip_good = true;

  if (cmd == NULL)
  {
    Serial.print(F("\nEnter IP:"));
  } 
  else
  {
    // IP address entered - validate
    c = sscanf(cmd, "%d.%d.%d.%d", &ip_addr[0], &ip_addr[1], &ip_addr[2], &ip_addr[3]);
    if (c != 4)
    {
      ip_good = false;
    } 
    else
    {
      for (int i=0; i < 4; i++)
        if (ip_addr[i] < 0 || ip_addr[i] > 255)
          ip_good = false;
    }

    if (ip_good)
    {
      Serial.println(F("\nOk - Saving"));
      set_ip(ip_addr);
    } 
    else
    {
      Serial.println(F("\nInvalid IP"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_server_ip(char *cmd)
/* Validate (and save, if valid) the supplied IP address. Note that validation *
 * is very crude, e.g. there is no rejection of class E, D, loopback, etc      */
{
  _serial_state = SS_SET_SERVER_IP;
  int ip_addr[4];
  int c;
  byte ip_good = true;

  if (cmd == NULL)
  {
    Serial.print(F("\nEnter server IP:"));
  } 
  else
  {
    // IP address entered - validate
    c = sscanf(cmd, "%d.%d.%d.%d", &ip_addr[0], &ip_addr[1], &ip_addr[2], &ip_addr[3]);
    if (c != 4)
    {
      ip_good = false;
    } 
    else
    {
      for (int i=0; i < 4; i++)
        if (ip_addr[i] < 0 || ip_addr[i] > 255)
          ip_good = false;
    }

    if (ip_good)
    {
      Serial.println(F("\nOk - Saving"));
      set_server_ip(ip_addr);
    } 
    else
    {
      Serial.println(F("\nInvalid IP"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_door_id(char *cmd)
/* Set door ID, and output confirmation */
{
  _serial_state = SS_SET_DOOR_ID;

  if (cmd == NULL)
  {
    Serial.print(F("\nEnter door id:"));
  } 
  else
  {
    byte new_door_id = (char)atoi(cmd);

    if (new_door_id > 0)
    {
      Serial.println(F("\nOk - Saving door id"));
      set_door_id(new_door_id);
    }
    else
    {
      Serial.println(F("\nError: door ID must be 1-254"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_topic(char *cmd)
/* Set topic name, and output confirmation */
{
  _serial_state = SS_SET_TOPIC;

  if (cmd == NULL)
  {
    Serial.print(F("\nEnter base topic:"));
  } 
  else
  {
    if (strlen(cmd) > 2)
    {
      Serial.println(F("\nOk - Saving"));
      set_topic(cmd);
    }
    else
    {
      Serial.println(F("\nError: too short"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_rfid(char *cmd)
/* Set RFID serial */
{
  _serial_state = SS_SET_EMGCY_RFID;

  if (cmd == NULL)
  {
    Serial.print(F("\nEnter RFID serial:"));
  } 
  else
  {
    if 
    (
      (strlen(cmd) ==  8) ||
      (strlen(cmd) == 14) ||
      (strlen(cmd) == 20)
    )
    {
      Serial.println(F("\nOk - Saving"));
      set_rfid(cmd);
    }
    else
    {
      Serial.println(F("\nError: incorrect length"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void set_mac(byte *mac_addr)
/* Write mac_addr to EEPROM */
{
  for (int i = 0; i < 6; i++)
  {
    EEPROM.write(EEPROM_MAC+i, mac_addr[i]);
    _mac[i] = mac_addr[i];
  }
}

void set_ip(int *ip_addr)
/* Write ip_addr to EEPROM */
{
  for (int i = 0; i < 4; i++)
  {
    EEPROM.write(EEPROM_IP+i, ip_addr[i]);
    _ip[i] = ip_addr[i];
  }
}

void set_server_ip(int *ip_addr)
/* Write ip_addr to EEPROM */
{
  for (int i = 0; i < 4; i++)
  {
    EEPROM.write(EEPROM_SERVER_IP+i, ip_addr[i]);
    _server[i] = ip_addr[i];
  }
}

void set_door_id(byte door_id)
/* Write door_id to EEPROM */
{
  EEPROM.write(EEPROM_DOOR_ID, door_id);
  _door_id = door_id;
}

void set_topic(char *new_topic)
/* Write new_topic to EEPROM */
{
  for (int i = 0; i < 40; i++)
  {
    EEPROM.write(EEPROM_BASE_TOPIC+i, new_topic[i]);
    _base_topic[i] = new_topic[i];
  }
}

void set_rfid(char *rfid_serial)
/* Write rfid_serial to EEPROM */
{
  for (int i = 0; i < 21; i++)
  {
    EEPROM.write(EEPROM_EMGCY_RFID+i, rfid_serial[i]);
    _rfid_override[i] = rfid_serial[i];
  }
}


