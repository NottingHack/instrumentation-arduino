#include <avr/wdt.h> 
#include <EEPROM.h>


void serial_menu();
void serial_process(char *cmd);
void serial_main_menu(const char *cmd);
void serial_show_main_menu();
void serial_show_settings();
void serial_set_mac(char *cmd);
void serial_set_ip(char *cmd);
void serial_set_server_ip(char *cmd);
void serial_set_door_id(char *cmd);
void serial_set_topic(char *cmd);
void set_mac(byte *mac_addr);
void set_ip(int *ip_addr);
void set_server_ip(int *ip_addr);
void set_door_id(byte new_door_id);
void set_topic(char *new_topic);


