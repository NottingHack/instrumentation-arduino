// #include <avr/wdt.h> 

#include <FlashAsEEPROM.h>


// Serial/config menu current position
enum serial_state_t
{
  SS_MAIN_MENU,
  SS_SET_MAC,
  SS_SET_IP,
  SS_SET_NAME,
  SS_SET_TOPIC,
  SS_SET_SERVER_IP
};


void serial_menu_init();
void serial_menu();
void serial_process(char *cmd);
void serial_main_menu(char *cmd);
void serial_show_main_menu();
void serial_show_settings();
void serial_set_mac(char *cmd);
void serial_set_ip(char *cmd);
void serial_set_server_ip(char *cmd);
void serial_set_name(char *cmd);
void serial_set_topic(char *cmd);
void set_mac(unsigned int *mac_addr);
void set_ip(int *ip_addr);
void set_server_ip(int *ip_addr);
void set_name(char *new_name);
void set_topic(char *new_topic);


