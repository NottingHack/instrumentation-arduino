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

