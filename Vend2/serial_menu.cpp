
/* Serial based menu configuration - settings saved in EEPROM */

#include "Arduino.h"
#include "Config.h"
#include "serial_menu.h"

// Network config
extern char _base_topic[NET_BASE_TOPIC_SIZE];
extern char _dev_name [NET_DEV_NAME_SIZE];
extern byte _mac[6];
extern byte _myip[4];
extern byte _serverip[4];
extern uint8_t _debug_level;

serial_state_t _serial_state;
static Stream *serial;


void serial_menu_init()
{
  SerialUSB.begin(9600);
  serial = &SerialUSB;

  _serial_state = SS_MAIN_MENU;


  if (EEPROM.isValid())
  {
    serial->println("Using saved settings");
    // Read settings from eeprom
    for (int i = 0; i < 6; i++)
      _mac[i] = EEPROM.read(EEPROM_MAC+i);
  
    for (int i = 0; i < 4; i++)
      _myip[i] = EEPROM.read(EEPROM_IP+i);
  
    for (int i = 0; i < 4; i++)
      _serverip[i] = EEPROM.read(EEPROM_SERVER_IP+i);
  
    for (int i = 0; i < 40; i++)
      _base_topic[i] = EEPROM.read(EEPROM_BASE_TOPIC+i);
    _base_topic[40] = '\0';
  
    for (int i = 0; i < 20; i++)
      _dev_name[i] = EEPROM.read(EEPROM_NAME+i);
    _dev_name[20] = '\0';

    _debug_level = EEPROM.read(EEPROM_DEBUG);
  }
  else
  {
    // (emulated) EEPROM contents isn't valid - use defaults instead
    serial->println("Using default settings");
    memcpy(_mac       , _default_mac      , 6);
    memcpy(_myip      , _default_ip       , 4);
    memcpy(_serverip  , _default_server_ip, 4);
    strcpy(_base_topic, DEFAULT_BASE_TOPIC);
    strcpy(_dev_name  , DEFAULT_NAME);
    _debug_level = 1;

    // Write the default config to EEPROM
    for (int i = 0; i < 6; i++)
      EEPROM.write(EEPROM_MAC+i, _mac[i]);

    for (int i = 0; i < 4; i++)
      EEPROM.write(EEPROM_IP+i, _myip[i]);

    for (int i = 0; i < 4; i++)
      EEPROM.write(EEPROM_SERVER_IP+i, _serverip[i]);

    for (int i = 0; i < 40; i++)
      EEPROM.write(EEPROM_BASE_TOPIC+i, _base_topic[i]);

    for (int i = 0; i < 20; i++)
      EEPROM.write(EEPROM_NAME+i, _dev_name[i]);

    EEPROM.write(EEPROM_DEBUG, _debug_level);

    EEPROM.commit();
  }
}

void serial_menu()
/* Process any serial input since last call - if any - and call serial_process when we have a cr/lf terminated line. */
{
  static char serial_line[65];
  static int i = 0;
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
  else if (_serial_state == SS_SET_DEBUG_LEVEL)
  {
    serial_set_debug_level(cmd);
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

  case 7: // "7 Set debug level"
    serial_set_debug_level(NULL);
    break;

  default:
    /* Unrecognised input - redisplay menu */
    serial->println();
    serial_show_main_menu();
  }
}

void serial_show_main_menu()
/* Output the main menu */
{
  serial->println();
  serial->println(F("Menu"));
  serial->println(F("----"));
  serial->println(F("1 Show settings"));
  serial->println(F("2 Set MAC"));
  serial->println(F("3 Set IP"));
  serial->println(F("4 Set server IP"));
  serial->println(F("5 Set name"));
  serial->println(F("6 Set base topic"));
  serial->println(F("7 Set debug level"));
  serial->print(F("Enter selection: "));
}

void serial_show_settings()
/* Output current config, as read during setup() */
{
  char buf[18];

  serial->println("\n");
  serial->println(F("Current settings:"));

  serial->print(F("MAC: "));
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
  serial->println(buf);

  serial->print(F("IP: "));
  sprintf(buf, "%d.%d.%d.%d", _myip[0], _myip[1], _myip[2], _myip[3]);
  serial->println(buf);

  serial->print(F("Server IP: "));
  sprintf(buf, "%d.%d.%d.%d", _serverip[0], _serverip[1], _serverip[2], _serverip[3]);
  serial->println(buf);

  serial->print(F("Name: "));
  serial->println(_dev_name);  

  serial->print(F("Base topic: "));
  serial->println(_base_topic);

  serial->print(F("Debug level: "));
  serial->println(_debug_level);
}

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
    serial->print(F("\nEnter IP:"));
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
      serial->println(F("\nOk - Saving IP"));
      set_ip(ip_addr);
    } 
    else
    {
      serial->println(F("\nInvalid IP"));
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
    serial->print(F("\nEnter server IP:"));
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
      serial->println(F("\nOk - Saving IP"));
      set_server_ip(ip_addr);
    } 
    else
    {
      serial->println(F("\nInvalid IP"));
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
    serial->print(F("\nEnter dev name:"));
  } 
  else
  {
    if (strlen(cmd) > 2)
    {
      serial->println(F("\nOk - Saving"));
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
      serial->println(F("\nOk - Saving"));
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


void serial_set_debug_level(char *cmd)
/* Set topic name, and output confirmation */
{
  _serial_state = SS_SET_DEBUG_LEVEL;

  if (cmd == NULL)
  {
    serial->print(F("\nEnter debug level (0=no debug, 1=basic, 2=comms trace): "));
  } 
  else
  {
    if (strlen(cmd) == 1)
    {
      int dbglvl = atoi(cmd);
      if (dbglvl >= 0 && dbglvl <= 2)
      {
        serial->println(F("\nOk - Saving"));
        set_debug_level(dbglvl);
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


void set_mac(unsigned int *mac_addr)
/* Write mac_addr to EEPROM */
{
  for (int i = 0; i < 6; i++)
  {
   EEPROM.write(EEPROM_MAC+i, mac_addr[i]);
   _mac[i] = mac_addr[i];
  }
  EEPROM.commit();
}

void set_ip(int *ip_addr)
/* Write ip_addr to EEPROM */
{
  for (int i = 0; i < 4; i++)
  {
    EEPROM.write(EEPROM_IP+i, ip_addr[i]);
    _myip[i] = ip_addr[i];
  }
  EEPROM.commit();
}

void set_server_ip(int *ip_addr)
/* Write ip_addr to EEPROM */
{
  for (int i = 0; i < 4; i++)
  {
    EEPROM.write(EEPROM_SERVER_IP+i, ip_addr[i]);
    _serverip[i] = ip_addr[i];
  }
  EEPROM.commit();
}

void set_name(char *new_name)
/* Write new_name to EEPROM */
{
  for (int i = 0; i < 20; i++)
  {
    EEPROM.write(EEPROM_NAME+i, new_name[i]);
    _dev_name[i] = new_name[i];
  }
  EEPROM.commit();
}

void set_topic(char *new_topic)
/* Write new_topic to EEPROM */
{
  for (int i = 0; i < 40; i++)
  {
    EEPROM.write(EEPROM_BASE_TOPIC+i, new_topic[i]);
    _base_topic[i] = new_topic[i];
  }
  EEPROM.commit();
}

void set_debug_level(uint8_t dbglvl)
{
  EEPROM.write(EEPROM_DEBUG, dbglvl);
  _debug_level = dbglvl;
  EEPROM.commit();
}
