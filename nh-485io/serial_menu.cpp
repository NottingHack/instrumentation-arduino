
/* Serial based menu configuration - settings saved in EEPROM */

#include "Arduino.h"
#include "Config.h"
#include "serial_menu.h"

extern uint8_t _modbus_address;
extern uint16_t _direction_mask;
extern uint8_t _debug_level;

serial_state_t _serial_state;
static Stream *serial;
uint8_t *_direction_mask_ptr = (uint8_t*) &_direction_mask;

void serial_menu_init()
{
  SerialUSB.begin(115200);
  serial = &SerialUSB;

  _serial_state = SS_MAIN_MENU;

  if (EEPROM.isValid())
  {
    serial->println("Using saved settings");
    // Read settings from eeprom
    _modbus_address = EEPROM.read(EEPROM_MODBUS_ADDRESS);

    for (int i = 0; i < 2; i++)
      _direction_mask_ptr[i] = EEPROM.read(EEPROM_IO_DIRECTION+i);

    _debug_level = EEPROM.read(EEPROM_DEBUG);
  }
  else
  {
    // (emulated) EEPROM contents isn't valid - use defaults instead
    serial->println("Using default settings");
    _modbus_address = 10;
    _direction_mask = 0x0000;
    _debug_level = 1;

    // Write the default config to EEPROM

    EEPROM.write(EEPROM_MODBUS_ADDRESS, _modbus_address);

    for (int i = 0; i < 2; i++)
      EEPROM.write(EEPROM_IO_DIRECTION+i, _direction_mask_ptr[i]);

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
  else if (_serial_state == SS_SET_ADDRESS)
  {
    serial_set_modbus_address(cmd);
    serial_main_menu("0");
  }
  else if (_serial_state == SS_SET_PIN_DIRECTION)
  {
    serial_set_io_direction(cmd);
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

  case 2: // "[ 2 ] Set MODBUS Address"
    serial_set_modbus_address(NULL);
    break;

  case 3: // "[ 3 ] Set IO Direction Mask"
    serial_set_io_direction(NULL);
    break;

  case 4: // "4 Set debug level"
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
  serial->println(F("2 Set MODBUS Address"));
  serial->println(F("3 Set IO Direction Mask"));
  serial->println(F("4 Set debug level"));
  serial->print(F("Enter selection: "));
}

void serial_show_settings()
/* Output current config, as read during setup() */
{
  char buf[18];

  serial->println("\n");
  serial->println(F("Current settings:"));

  serial->print(F("MODBUS Address: "));
  serial->println(_modbus_address);

  serial->print(F("IO Direction Mask: "));
  sprintf(buf, "%04x", _direction_mask);
  serial->println(buf);

  serial->print(F("Debug level: "));
  serial->println(_debug_level);
}

void serial_set_modbus_address(char *cmd)
{
  _serial_state = SS_SET_ADDRESS;

  if (cmd == NULL)
  {
    serial->print(F("\nEnter MODBUS Address (0-247): "));
  }
  else
  {
    if (strlen(cmd) < 3)
    {
      uint8_t address = atoi(cmd);
      if (address >= 0 && address <= 247)
      {
        serial->println(F("\nOk - Saving"));
        set_modbus_address(address);
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

void serial_set_io_direction(char *cmd)
{

  _serial_state = SS_SET_PIN_DIRECTION;

  if (cmd == NULL)
  {
    serial->print(F("\nEnter IO Direction Mask [dddd] 0bit=input 1=output: "));
  }
  else
  {
    if (strlen(cmd) == 4)
    {
      int _direction_mask = strtoul(cmd, NULL, 16);
      serial->println(F("\nOk - Saving"));
      set_io_direction((uint16_t) _direction_mask);
    }
    else
    {
      serial->println(F("\nError: too short or long"));
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

void set_modbus_address(uint8_t new_modbus_address)
/* Write new_modbus_address to EEPROM */
{
  EEPROM.write(EEPROM_MODBUS_ADDRESS, new_modbus_address);
  _modbus_address = new_modbus_address;
  EEPROM.commit();
}

void set_io_direction(uint16_t new_io_direction)
/* Write new_io_direction to EEPROM */
{
  uint8_t *p = (uint8_t*) &new_io_direction;
  for (int i = 0; i < 2; i++)
  {
    EEPROM.write(EEPROM_IO_DIRECTION+i, p[i]);

  }
  _direction_mask = new_io_direction;
  EEPROM.commit();
}

void set_debug_level(uint8_t dbglvl)
{
  EEPROM.write(EEPROM_DEBUG, dbglvl);
  _debug_level = dbglvl;
  EEPROM.commit();
}
