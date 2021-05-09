#ifndef MENU_H
#define MENU_H

#include <avr/wdt.h>
#include <EEPROM.h>

void serial_menu();
void serial_process(char *cmd);
void serial_main_menu(char *cmd);
void serial_show_main_menu();
void serial_show_settings();
void serial_show_override_settings();
void serial_set_mac(char *cmd);
void serial_set_ip(char *cmd);
void serial_set_server_ip(char *cmd);
void serial_set_name(char *cmd);
void serial_set_topic(char *cmd);
void set_mac(byte *mac_addr);
void set_ip(int *ip_addr);
void set_server_ip(int *ip_addr);
void set_name(char *new_name);
void set_topic(char *new_topic);
bool serial_set_input_overide(char *cmd);
void set_input_enables(int channel, bool enable);
void set_override_masks(int channel, uint32_t mask);
void set_override_states(int channel, uint32_t states);
void set_input_statefullness(int channel, bool enable);

#endif
