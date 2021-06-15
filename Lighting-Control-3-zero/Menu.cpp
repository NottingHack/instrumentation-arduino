/*
 * Copyright (c) 2021, Daniel Swann <hs@dswann.co.uk>, Matt Lloyd <dps.lwk@gmail.com>
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
#include <SPIEEPROM.h>

#include "Config.h"
#include "Menu.h"

#include <errno.h>

extern char _base_topic[40];
extern char _dev_name[20];

// MAC / IP address of device
extern byte _mac[6];
extern byte _ip[4];
extern byte _server[4];
extern uint8_t _input_enables;
extern uint32_t _override_masks[8];
extern uint32_t _override_states[8];
extern uint8_t _input_statefullness;
extern bool _energy_monitor_enabled;
extern uint8_t _rs485_io_count;
extern uint16_t _rs485_io_input_enables[10];
extern uint32_t _rs485_io_override_masks[10][16];
extern uint32_t _rs485_io_override_states[10][16];
extern uint16_t _rs485_io_input_statefullness[10];

serial_state_t _serial_state;
static Stream *serial;
SPIEEPROM spieeprom;

enum serial_input_override_state_t
{
  NODE,
  CHANNEL,
  ENABLE,
  MASK,
  STATES,
  STATEFULL,
  DONE
};
serial_input_override_state_t _serial_input_override_state;
uint8_t _override_node;
uint8_t _override_channel;

/* Serial based menu configuration - settings saved in EEPROM */

void serial_menu_init()
{
  SerialUSB.begin(115200);
  serial = &SerialUSB;

  _serial_state = SS_MAIN_MENU;
  eeprom_init();
}

void eeprom_init()
{
  spieeprom.begin(SPI_EEPROM_SS);

  uint8_t valid;

  // digitalWrite(PIN_LED_3, HIGH);
  spieeprom.read(EEPROM_VALID, valid);
  // digitalWrite(PIN_LED_3, LOW);

  if (valid != 64) {
    // invalid settings
    serial->print("Valid: ");
    serial->println(valid);
    serial->println("Using default settings");
    eeprom_default();
  } else {
    serial->println("Using saved settings");
    eeprom_load();
  }
}

void eeprom_load()
{
  // digitalWrite(PIN_LED_3, HIGH);
  // Read settings from eeprom
  spieeprom.read(EEPROM_MAC, _mac);
  spieeprom.read(EEPROM_IP, _ip);
  spieeprom.read(EEPROM_SERVER_IP, _server);

  spieeprom.read(EEPROM_BASE_TOPIC, _base_topic);
  _base_topic[39] = '\0';

  spieeprom.read(EEPROM_NAME, _dev_name);
  _dev_name[19] = '\0';

  spieeprom.read(EEPROM_INPUT_ENABLES, _input_enables);
  spieeprom.read(EEPROM_OVERRIDE_MASKS, _override_masks);
  spieeprom.read(EEPROM_OVERRIDE_STATES, _override_states);
  spieeprom.read(EEPROM_INPUT_STATEFULL, _input_statefullness);

  spieeprom.read(EEPROM_ENERGY_MONITOR_ENABLE, _energy_monitor_enabled);

  spieeprom.read(EEPROM_RS458_IO_COUNT, _rs485_io_count);
  spieeprom.read(EEPROM_RS485_INPUT_ENABLES, _rs485_io_input_enables);
  spieeprom.read(EEPROM_RS485_OVERRIDE_MASKS, _rs485_io_override_masks);
  spieeprom.read(EEPROM_RS485_OVERRIDE_STATES, _rs485_io_override_states);
  spieeprom.read(EEPROM_RS485_INPUT_STATEFULL, _rs485_io_input_statefullness);
  // digitalWrite(PIN_LED_3, LOW);
  serial->println("EEPROM Loaded");
  serial_show_settings();
}

void eeprom_default()
{
  uint8_t valid = 255;
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_VALID, valid);
  // digitalWrite(PIN_LED_2, LOW);

  serial->println("Erasing EEPROM: ");
  // digitalWrite(PIN_LED_2, HIGH);
  for (uint16_t i = 0; i <= EEPROM_VALID; ++i)
  {
    spieeprom.write(i, valid);
  }
  // digitalWrite(PIN_LED_2, LOW);
  serial->println("\nDone");

  memcpy(_mac       , _default_mac      , 6);
  memcpy(_ip        , _default_ip       , 4);
  memcpy(_server    , _default_server_ip, 4);
  strcpy(_dev_name  , DEFAULT_NAME);
  strcpy(_base_topic, DEFAULT_BASE_TOPIC);

  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_MAC, _mac);
  spieeprom.write(EEPROM_IP, _ip);
  spieeprom.write(EEPROM_SERVER_IP, _server);
  spieeprom.write(EEPROM_BASE_TOPIC, _base_topic);
  spieeprom.write(EEPROM_NAME, _dev_name);
  // digitalWrite(PIN_LED_2, LOW);

  // for (int channel = 0; channel < 8; ++channel) {
  //   set_input_enables(channel, false);
  //   set_override_masks(channel, 0xFFFFFFFF);
  //   set_override_states(channel, 0xFFFFFFFF);
  //   set_input_statefullness(channel, false);
  // }

  set_energy_monitor(false);
  set_rs485_io_count(0);

  // for (int node = 0; node < 10; ++node) {
  //   for (int channel = 0; channel < 16; ++channel) {
  //     set_rs485_input_enables(node, channel, false);
  //     set_rs485_override_masks(node, channel, 0xFFFFFFFF);
  //     set_rs485_override_states(node, channel, 0xFFFFFFFF);
  //     set_rs485_input_statefullness(node, channel, true);
  //   }
  // }

  // mark eeprom as valid
  valid = 64;
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_VALID, valid);
  // digitalWrite(PIN_LED_2, LOW);

  serial->println("EEPROM defaults Set");
  delay(10);
  eeprom_dump();
  delay(10);

  eeprom_load(); // reload then all just to be sure
}

void eeprom_dump()
{
  uint8_t mem;
  char buf[4];

  for (int i = 0; i <= 2048; ++i)
  {
    if (i % 32 == 0)
      serial->println("");
    spieeprom.read(i, mem);
    sprintf(buf, "%02x,", mem);
    serial->print(buf);
  }

  serial->println("");
}

void serial_menu()
/* Process any serial input since last call - if any - and call serial_process when we have a cr/lf terminated line. */
{
  static char serial_line[65];
  static unsigned int i = 0;
  char c;

  while (serial->available())
  {
    if (i == 0)
      memset(serial_line, 0, sizeof(serial_line));

    c = serial->read();
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
  else if (_serial_state == SS_SET_NAME)
  {
    serial_set_name(cmd);
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
  else if (_serial_state == SS_SET_INPUT_OVERRIDE)
  {
    if (serial_set_input_override(cmd))
      serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_ENERGY_MONITOR)
  {
    serial_set_energy_monitor(cmd);
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_RS48_IO)
  {
    serial_set_rs485_io_count(cmd);
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_RS48_IO_INPUT_OVERRIDE)
  {
    if (serial_set_rs485_io_input_override(cmd))
      serial_main_menu("0");
  }
}

void serial_main_menu(char *cmd)
/* Respond to supplied input <cmd> for main menu */
{
  int i = atoi(cmd);

  switch (i)
  {
  case 1: // "[ 1 ] Show current settings"
    serial_show_settings();
    serial->println();
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

  case 5: // "[ 5 ] Set name"
    serial_set_name(NULL);
    break;

  case 6: // "[ 6 ] Set base topic"
    serial_set_topic(NULL);
    break;

  case 7: // [ 7 ] Set input override enables
    serial_set_input_override(NULL);
    break;

  case 8: // [ 8 ] Set input override enables
    serial_set_energy_monitor(NULL);
    break;

  case 9: // [ 9 ] Set RS485 IO Count
    serial_set_rs485_io_count(NULL);
    break;

  case 10: // [ 10 ] Show current RS485 IO input override settings
    serial_show_rs485_settings();
    serial->println();
    serial_show_main_menu();
    break;

  case 11: // [ 11 ] Set RS485 IO input override
    serial_set_rs485_io_input_override(NULL);
    break;

  case 12: // [ 12 ] Erase EEPROM and set defaults
    eeprom_default();
    serial->println();
    serial_show_main_menu();
    break;

  case 13: // [ 13 ] Dump EEPROM
    eeprom_dump();
    serial->println();
    serial_show_main_menu();
    break;

  case 99: // "[ 99 ] Reset/reboot"
    serial->println("Reboot....");
    // wdt_enable(WDTO_2S); // Watchdog abuse...
    while(1);
    break;

  default:
    /* Unrecognised input - redisplay menu */
    serial->println();
    serial_show_main_menu();
  }
}

/*
 * Serial Sets
 */

void serial_show_main_menu()
/* Output the main menu */
{
  serial->println();
  serial->println(F("Main menu"));
  serial->println(F("---------"));
  serial->println(F("[ 1 ] Show current settings"));
  serial->println(F("[ 2 ] Set MAC address"));
  serial->println(F("[ 3 ] Set IP address"));
  serial->println(F("[ 4 ] Set server IP address"));
  serial->println(F("[ 5 ] Set name"));
  serial->println(F("[ 6 ] Set base topic"));
  serial->println(F("[ 7 ] Set input override"));
  serial->println(F("[ 8 ] Set energy monitor"));
  serial->println(F("[ 9 ] Set RS485 IO Count"));
  serial->println(F("[ 10 ] Show current RS485 IO input override settings"));
  serial->println(F("[ 11 ] Set RS485 IO input override"));
  serial->println(F("[ 12 ] Erase EEPROM and set defaults"));
  serial->println(F("[ 13 ] Dump EEPROM"));
  serial->println(F("[ 99 ] Reset/reboot"));
  serial->print(F("Enter selection: "));
}

void serial_show_settings()
/* Output current config, as read during setup() */
{
  char buf[18];

  serial->println("\n");
  serial->println(F("Current settings:"));

  serial->print(F("MAC address: "));
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
  serial->println(buf);

  serial->print(F("IP address : "));
  sprintf(buf, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
  serial->println(buf);

  serial->print(F("Server IP  : "));
  sprintf(buf, "%d.%d.%d.%d", _server[0], _server[1], _server[2], _server[3]);
  serial->println(buf);

  serial->print(F("Name       : "));
  serial->println(_dev_name);

  serial->print(F("Base topic : "));
  serial->println(_base_topic);

  serial_show_override_settings();

  serial->print(F("Energy Monitor Enabled: is currently "));
  if (_energy_monitor_enabled == 0)
  {
    serial->print(F("enabled ("));
  }
  else
  {
    serial->print(F("disabled ("));
  }
  serial->print(_energy_monitor_enabled);
  serial->println(F(")"));

  serial->print(F("RS485 IO Count : "));
  serial->println(_rs485_io_count);
}

void serial_show_override_settings()
{
  for (int i = 0; i < 8; ++i)
  {
    serial->print(F("Channel: "));
    serial->print(i);
    serial->print(F(" Enabled: "));
    serial->print((_input_enables & (1<<i)) == 0);
    serial->print(F(" Mask: 0x"));
    serial->print(_override_masks[i], HEX);
    serial->print(F(" States: 0x"));
    serial->print(_override_states[i], HEX);
    serial->print(F(" Statefull: "));
    serial->println((_input_statefullness & (1<<i)) == 0);
  }

}

void serial_show_rs485_settings()
{
  serial->print(F("RS485 IO Count : "));
  serial->println(_rs485_io_count);
  for (int node = 0; node < 10; ++node)
  {
    serial->print(F("Node: "));
    serial->println(node);
    for (int channel = 0; channel < 16; ++channel)
    {
      serial->print(F("Channel: "));
      serial->print(channel);
      serial->print(F(" Enabled: "));
      serial->print((_rs485_io_input_enables[node] & (1<<channel)) == 0);
      serial->print(F(" Mask: 0x"));
      serial->print(_rs485_io_override_masks[node][channel], HEX);
      serial->print(F(" States: 0x"));
      serial->print(_rs485_io_override_states[node][channel], HEX);
      serial->print(F(" Statefull: "));
      serial->println((_rs485_io_input_statefullness[node] & (1<<channel)) == 0);
    }
  }
}

/*
 * ====================
 * End of Serial Shows
 */

/*
 * Serial Sets
 */

void serial_set_mac(char *cmd)
/* Validate (and save, if valid) the supplied mac address */
{
  _serial_state = SS_SET_MAC;
  unsigned int mac_addr[6];
  int c;

  if (cmd == NULL)
  {
    serial->print(F("\nEnter MAC:"));
  }
  else
  {
    // MAC address entered - validate
    c = sscanf(cmd, "%2hx:%2hx:%2hx:%2hx:%2hx:%2hx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
    if (c == 6)
    {
      serial->println(F("\nOk - Saving MAC"));
      set_mac(mac_addr);
    }
    else
    {
      serial->println(F("\nInvalid MAC"));
      serial->println(c);
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
    serial->print(F("\nEnter IP address:"));
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
      serial->println(F("\nOk - Saving address"));
      set_ip(ip_addr);
    }
    else
    {
      serial->println(F("\nInvalid IP address"));
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
    serial->print(F("\nEnter server IP address:"));
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
      serial->println(F("\nOk - Saving address"));
      set_server_ip(ip_addr);
    }
    else
    {
      serial->println(F("\nInvalid IP address"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_name(char *cmd)
/* Set device name, and output confirmation */
{
  _serial_state = SS_SET_NAME;

  if (cmd == NULL)
  {
    serial->print(F("\nEnter device name:"));
  }
  else
  {
    if (strlen(cmd) > 2)
    {
      serial->println(F("\nOk - Saving name"));
      set_name(cmd);
    }
    else
    {
      serial->println(F("\nError: too short"));
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
    serial->print(F("\nEnter base topic:"));
  }
  else
  {
    if (strlen(cmd) > 2)
    {
      serial->println(F("\nOk - Saving topic"));
      set_topic(cmd);
    }
    else
    {
      serial->println(F("\nError: too short"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

bool serial_set_input_override(char *cmd)
/* set a single input mask */
{
  _serial_state = SS_SET_INPUT_OVERRIDE;

  if (cmd == NULL)
  {
    _serial_input_override_state = CHANNEL;
    serial->print(F("\nEnter input channel to change [0-7]: "));
    return false;
  }
  else if (_serial_input_override_state == CHANNEL)
  {
    // stash channe for later
    _override_channel = atoi(cmd);
    serial->print(F("\nOverride for channel "));
    serial->print(_override_channel);
    serial->print(F(" is currently "));
    if ((_input_enables & (1<<_override_channel)) == 0)
    {
      serial->print(F("enabled"));
    }
    else
    {
      serial->print(F("disabled"));
    }

    serial->print(F("\n[y] to enable, [n] to disable? "));
    _serial_input_override_state = ENABLE;
    return false;
  }
  else if (_serial_input_override_state == ENABLE)
  {
    if (cmd[0] != 'y') {
      // Disable mask
      set_input_enables(_override_channel, false);

      serial->print(F("\nOverride for channel "));
      serial->print(_override_channel);
      serial->print(F(" Disabled"));
      // Return to main menu
      _serial_input_override_state = DONE;
      _serial_state = SS_MAIN_MENU;
      return true;
    }

    // enable channel
    set_input_enables(_override_channel, true);

    serial->print(F("\nOverride for channel "));
    serial->print(_override_channel);
    serial->print(F(" Enabled"));

    serial->print(F("\nEnter mask for channel "));
    serial->print(_override_channel);
    serial->print(F("? [mmmmmmmm]: "));
    _serial_input_override_state = MASK;
    return false;
  }
  else if (_serial_input_override_state == MASK)
  {
    // save the mask
    if (strlen(cmd) != 8) {
      serial->print(F("Invalid mask length"));
      serial->print(F("\nEnter mask for channel "));
      serial->print(_override_channel);
      serial->print(F("? [mmmmmmmm]: "));
      return false;
    }

    char *e;
    errno = 0;
    uint32_t mask = strtoul(cmd, &e, 16);
    if ( *e || errno==EINVAL || errno==ERANGE )
    {
      serial->print(F("Invalid mask"));
      serial->print(F("\nEnter mask for channel "));
      serial->print(_override_channel);
      serial->print(F("? [mmmmmmmm]: "));
      return false;
    }

    set_override_masks(_override_channel, mask);

    serial->print(F("\nEnter state for channel "));
    serial->print(_override_channel);
    serial->print(F("? [ssssssss]: "));

    _serial_input_override_state = STATES;
    return false;
  }
  else if (_serial_input_override_state == STATES)
  {
    // save the states
    if (strlen(cmd) != 8) {
      serial->print(F("Invalid state length"));
      serial->print(F("\nEnter state for channel "));
      serial->print(_override_channel);
      serial->print(F("? [ssssssss]: "));
      return false;
    }

    char *e;
    errno = 0;
    uint32_t states = strtoul(cmd, &e, 16);
    if ( *e || errno==EINVAL || errno==ERANGE )
    {
      serial->print(F("Invalid state"));
      serial->print(F("\nEnter state for channel "));
      serial->print(_override_channel);
      serial->print(F("? [ssssssss]: "));
      return false;
    }

    set_override_states(_override_channel, states);

    serial->print(F("\nState tracking for channel "));
    serial->print(_override_channel);
    serial->print(F(" is currently "));
    if ((_input_statefullness & (1<<_override_channel)) == 0)
    {
      serial->print(F("enabled"));
    }
    else
    {
      serial->print(F("disabled"));
    }

    serial->print(F("\n[y] to enable, [n] to disable? "));
    _serial_input_override_state = STATEFULL;
    return false;
  }
  else if (_serial_input_override_state == STATEFULL)
  {
    if (cmd[0] != 'y') {
      // Disable state tracking for channel
      set_input_statefullness(_override_channel, false);

      serial->print(F("\nState tracking for channel "));
      serial->print(_override_channel);
      serial->print(F(" Disabled"));
      // Return to main menu
      _serial_input_override_state = DONE;
      _serial_state = SS_MAIN_MENU;
      return true;
    }

    // enable state tracking for channel
    set_input_statefullness(_override_channel, true);

    serial->print(F("\nState tracking for channel "));
    serial->print(_override_channel);
    serial->print(F(" Enabled"));

    serial->print(F("Channel updated"));
    // Return to main menu
    _serial_input_override_state = DONE;
    _serial_state = SS_MAIN_MENU;
    return true;
  }
  return false;
}

void serial_set_energy_monitor(char *cmd)
{
  _serial_state = SS_SET_ENERGY_MONITOR;

  if (cmd == NULL)
  {
    serial->print(F("\nEnable Energy monitor:"));
    serial->print(F("\n[y] to enable, [n] to disable? "));
  }
  else
  {
    if (cmd[0] == 'y') {
      set_energy_monitor(true);
      serial->print(F("\nEnergy monitor Enabled"));
    }
    else
    {
      set_energy_monitor(false);
      serial->print(F("\nEnergy monitor Disabled"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

void serial_set_rs485_io_count(char *cmd)
{
  _serial_state = SS_SET_RS48_IO;

  if (cmd == NULL)
  {
    serial->print(F("\nEnter number of RS458 IO module attached [0-10]:"));
  }
  else
  {
    if (strlen(cmd) <= 2)
    {
      int count = atoi(cmd);
      if (count >= 0 && count <= 10) {
        serial->println(F("\nOk - Saving count"));
        set_rs485_io_count(count);
      }
      else
      {
        serial->println(F("\nError: invalid entry"));
      }
    }
    else
    {
      serial->println(F("\nError: too long"));
    }
    // Return to main menu
    _serial_state = SS_MAIN_MENU;
  }
  return;
}

bool serial_set_rs485_io_input_override(char *cmd)
/* set a single input mask */
{
  _serial_state = SS_SET_RS48_IO_INPUT_OVERRIDE;

  if (cmd == NULL)
  {
    _serial_input_override_state = NODE;
    serial->print(F("\nEnter node to change [0-9]: "));
    return false;
  }
  else if (_serial_input_override_state == NODE)
  {
    // stash channe for later
    _override_node = atoi(cmd);
    serial->print(F("\nOverride for Node "));
    serial->print(_override_node);
    _serial_input_override_state = CHANNEL;
    serial->print(F("\nEnter input channel to change [0-15]: "));
    return false;
  }
  else if (_serial_input_override_state == CHANNEL)
  {
    // stash channe for later
    _override_channel = atoi(cmd);
    serial->print(F("\nOverride for channel "));
    serial->print(_override_channel);
    serial->print(F(" is currently "));
    if ((_rs485_io_input_enables[_override_node] & (1<<_override_channel)) == 0)
    {
      serial->print(F("enabled"));
    }
    else
    {
      serial->print(F("disabled"));
    }

    serial->print(F("\n[y] to enable, [n] to disable? "));
    _serial_input_override_state = ENABLE;
    return false;
  }
  else if (_serial_input_override_state == ENABLE)
  {
    if (cmd[0] != 'y') {
      // Disable mask
      set_rs485_input_enables(_override_node, _override_channel, false);

      serial->print(F("\nOverride for channel "));
      serial->print(_override_channel);
      serial->print(F(" Disabled"));
      // Return to main menu
      _serial_input_override_state = DONE;
      _serial_state = SS_MAIN_MENU;
      return true;
    }

    // enable channel
    set_rs485_input_enables(_override_node, _override_channel, true);

    serial->print(F("\nOverride for channel "));
    serial->print(_override_channel);
    serial->print(F(" Enabled"));

    serial->print(F("\nEnter mask for channel "));
    serial->print(_override_channel);
    serial->print(F("? [mmmmmmmm]: "));
    _serial_input_override_state = MASK;
    return false;
  }
  else if (_serial_input_override_state == MASK)
  {
    // save the mask
    if (strlen(cmd) != 8) {
      serial->print(F("Invalid mask length"));
      serial->print(F("\nEnter mask for channel "));
      serial->print(_override_channel);
      serial->print(F("? [mmmmmmmm]: "));
      return false;
    }

    char *e;
    errno = 0;
    uint32_t mask = strtoul(cmd, &e, 16);
    if ( *e || errno==EINVAL || errno==ERANGE )
    {
      serial->print(F("Invalid mask"));
      serial->print(F("\nEnter mask for channel "));
      serial->print(_override_channel);
      serial->print(F("? [mmmmmmmm]: "));
      return false;
    }

    set_rs485_override_masks(_override_node, _override_channel, mask);

    serial->print(F("\nEnter state for channel "));
    serial->print(_override_channel);
    serial->print(F("? [ssssssss]: "));

    _serial_input_override_state = STATES;
    return false;
  }
  else if (_serial_input_override_state == STATES)
  {
    // save the states
    if (strlen(cmd) != 8) {
      serial->print(F("Invalid state length"));
      serial->print(F("\nEnter state for channel "));
      serial->print(_override_channel);
      serial->print(F("? [ssssssss]: "));
      return false;
    }

    char *e;
    errno = 0;
    uint32_t states = strtoul(cmd, &e, 16);
    if ( *e || errno==EINVAL || errno==ERANGE )
    {
      serial->print(F("Invalid state"));
      serial->print(F("\nEnter state for channel "));
      serial->print(_override_channel);
      serial->print(F("? [ssssssss]: "));
      return false;
    }

    set_rs485_override_states(_override_node, _override_channel, states);

    serial->print(F("\nState tracking for channel "));
    serial->print(_override_channel);
    serial->print(F(" is currently "));
    if ((_rs485_io_input_statefullness[_override_node] & (1<<_override_channel)) == 0)
    {
      serial->print(F("enabled"));
    }
    else
    {
      serial->print(F("disabled"));
    }

    serial->print(F("\n[y] to enable, [n] to disable? "));
    _serial_input_override_state = STATEFULL;
    return false;
  }
  else if (_serial_input_override_state == STATEFULL)
  {
    if (cmd[0] != 'y') {
      // Disable state tracking for channel
      set_rs485_input_statefullness(_override_node, _override_channel, false);

      serial->print(F("\nState tracking for channel "));
      serial->print(_override_channel);
      serial->print(F(" Disabled"));
      // Return to main menu
      _serial_input_override_state = DONE;
      _serial_state = SS_MAIN_MENU;
      return true;
    }

    // enable state tracking for channel
    set_rs485_input_statefullness(_override_node, _override_channel, true);

    serial->print(F("\nState tracking for channel "));
    serial->print(_override_channel);
    serial->print(F(" Enabled"));

    serial->print(F("Channel updated"));
    // Return to main menu
    _serial_input_override_state = DONE;
    _serial_state = SS_MAIN_MENU;
    return true;
  }
  return false;
}

/*
 * ====================
 * End of Serial Sets
 */

/*
 * EEPROM Write methods
 */

void set_mac(unsigned int  *mac_addr)
/* Write mac_addr to EEPROM */
{
  for (int i = 0; i < 6; i++)
  {
    _mac[i] = mac_addr[i];
  }

  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_MAC, _mac);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_ip(int *ip_addr)
/* Write ip_addr to EEPROM */
{
  for (int i = 0; i < 4; i++)
  {
    _ip[i] = ip_addr[i];
  }

  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_IP, _ip);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_server_ip(int *ip_addr)
/* Write ip_addr to EEPROM */
{
  for (int i = 0; i < 4; i++)
  {
    _server[i] = ip_addr[i];
  }

  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_SERVER_IP, _server);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_name(char *new_name)
/* Write new_name to EEPROM */
{
  for (int i = 0; i < 20; i++)
  {
    _dev_name[i] = new_name[i];
  }
  _dev_name[19] = '\0';
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_NAME, _dev_name);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_topic(char *new_topic)
/* Write new_topic to EEPROM */
{
  for (int i = 0; i < 40; i++)
  {
    _base_topic[i] = new_topic[i];
  }
  _base_topic[39] = '\0';
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_BASE_TOPIC, _base_topic);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_input_enables(int channel, bool enable)
{
  if (enable)
  {
    _input_enables &= ~(1UL<<(channel));
  }
  else
  {
    _input_enables |= (1UL<<(channel));
  }
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_INPUT_ENABLES, _input_enables);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_override_masks(int channel, uint32_t mask)
{
  _override_masks[channel] = mask;
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_OVERRIDE_MASKS+(channel*sizeof(mask)), mask);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_override_states(int channel, uint32_t states)
{
  _override_states[channel] = states;
  spieeprom.write(EEPROM_OVERRIDE_STATES+(channel*sizeof(states)), states);
}

void set_input_statefullness(int channel, bool enable)
{
  if (enable)
  {
    _input_statefullness &= ~(1UL<<(channel));
  }
  else
  {
    _input_statefullness |= (1UL<<(channel));
  }
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_INPUT_STATEFULL, _input_statefullness);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_energy_monitor(bool enable)
{
  if (enable)
  {
    _energy_monitor_enabled = 0;
  }
  else
  {
    _energy_monitor_enabled = 1;
  }

  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_ENERGY_MONITOR_ENABLE, _energy_monitor_enabled);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_rs485_io_count(int count)
{
  _rs485_io_count = count;
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_RS458_IO_COUNT, _rs485_io_count);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_rs485_input_enables(int node, int channel, bool enable)
{
  if (enable)
  {
    _rs485_io_input_enables[node] &= ~(1UL<<(channel));
  }
  else
  {
    _rs485_io_input_enables[node] |= (1UL<<(channel));
  }
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_RS485_INPUT_ENABLES+(node*sizeof(_rs485_io_input_enables[node])), _rs485_io_input_enables[node]);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_rs485_override_masks(int node, int channel, uint32_t mask)
{

  _rs485_io_override_masks[node][channel] = mask;
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_RS485_OVERRIDE_MASKS+(node*16*sizeof(mask))+(channel*sizeof(mask)), mask);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_rs485_override_states(int node, int channel, uint32_t states)
{

  _rs485_io_override_states[node][channel] = states;
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_RS485_OVERRIDE_STATES+(node*16*sizeof(states))+(channel*sizeof(states)), states);
  // digitalWrite(PIN_LED_2, LOW);
}

void set_rs485_input_statefullness(int node, int channel, bool enable)
{

  if (enable)
  {
    _rs485_io_input_statefullness[node] &= ~(1UL<<(channel));
  }
  else
  {
    _rs485_io_input_statefullness[node] |= (1UL<<(channel));
  }
  // digitalWrite(PIN_LED_2, HIGH);
  spieeprom.write(EEPROM_RS485_INPUT_STATEFULL+(node*sizeof(_rs485_io_input_statefullness[node])), _rs485_io_input_statefullness[node]);
  // digitalWrite(PIN_LED_2, LOW);
}

/*
 * ====================
 * End of EEPROM Write methods
 */
