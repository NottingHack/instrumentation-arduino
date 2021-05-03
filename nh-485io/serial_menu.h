#ifndef SERIAL_MENU_H
#define SERIAL_MENU_H

#include <FlashAsEEPROM.h>


// Serial/config menu current position
enum serial_state_t
{
  SS_MAIN_MENU,
  SS_SET_ADDRESS,
  SS_SET_PIN_DIRECTION,
  SS_SET_DEBUG_LEVEL
};


void serial_menu_init();
void serial_menu();
void serial_process(char *cmd);
void serial_main_menu(char *cmd);
void serial_show_main_menu();
void serial_show_settings();
void serial_set_modbus_address(char *cmd);
void serial_set_io_direction(char *cmd);
void serial_set_debug_level(char *cmd);
void set_modbus_address(uint8_t modbus_address);
void set_io_direction(uint16_t io_direction);
void set_debug_level(uint8_t dbglvl);

#endif
