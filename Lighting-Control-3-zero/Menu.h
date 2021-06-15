#ifndef MENU_H
#define MENU_H

// Serial/config menu current position
enum serial_state_t
{
  SS_MAIN_MENU,
  SS_SET_MAC,
  SS_SET_IP,
  SS_SET_NAME,
  SS_SET_TOPIC,
  SS_SET_SERVER_IP,
  SS_SET_INPUT_OVERRIDE,
  SS_SET_ENERGY_MONITOR,
  SS_SET_RS48_IO,
  SS_SET_RS48_IO_INPUT_OVERRIDE
};

void serial_menu_init();
void eeprom_init();
void eeprom_load();
void eeprom_default();
void eeprom_dump();
void serial_menu();
void serial_process(char *cmd);
void serial_main_menu(char *cmd);

void serial_show_main_menu();
void serial_show_settings();
void serial_show_override_settings();
void serial_show_rs485_settings();

void serial_set_mac(char *cmd);
void serial_set_ip(char *cmd);
void serial_set_server_ip(char *cmd);
void serial_set_name(char *cmd);
void serial_set_topic(char *cmd);
bool serial_set_input_override(char *cmd);
void serial_set_energy_monitor(char *cmd);
void serial_set_rs485_io_count(char *cmd);
bool serial_set_rs485_io_input_override(char *cmd);

void set_mac(unsigned int *mac_addr);
void set_ip(int *ip_addr);
void set_server_ip(int *ip_addr);
void set_name(char *new_name);
void set_topic(char *new_topic);
void set_input_enables(int channel, bool enable);
void set_override_masks(int channel, uint32_t mask);
void set_override_states(int channel, uint32_t states);
void set_input_statefullness(int channel, bool enable);
void set_energy_monitor(bool enable);
void set_rs485_io_count(int count);
void set_rs485_input_enables(int node, int channel, bool enable);
void set_rs485_override_masks(int node, int channel, uint32_t mask);
void set_rs485_override_states(int node, int channel, uint32_t states);
void set_rs485_input_statefullness(int node, int channel, bool enable);

#endif
