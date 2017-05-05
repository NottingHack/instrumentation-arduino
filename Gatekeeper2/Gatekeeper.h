#include "DoorSide.h"



void send_door_side_msg(char side_id, char *topic, char *msg);
void display_message(char side, char *msg, int duration);
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void buzzer(char door_side, uint16_t tone_hz, uint16_t duration);
void lock_door();
void unlock_door();
void check_door_relay();
void door_side_loop(DoorSide *side);
void send_door_state(uint8_t door_state);

void mqtt_rx_display(char *payload);
void checkMQTT();
void setup();
void door_contact();
void door_bell_button();
void timeout_loop();
void check_door();
void update_door_state(boolean always_send_update=false);
void door_bell_loop();
void loop();

void lcd_loop();
void poll_rfid(DoorSide *side);
void check_buttons(DoorSide *side);
void send_action(const char *act, char *msg, bool include_door_id);
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);
void lcd_display(const __FlashStringHelper *n, short line = 0, boolean wipe_display = true);
void lcd_display(char *msg, short line = 0, boolean wipe_display = true);
void lcd_display_mqtt(char *payload);


// IP of MQTT server
byte _server[4];

char _base_topic[41];
char _dev_name [21];
byte _door_id;

// MAC / IP address of device
byte _mac[6];
byte _ip[4];

serial_state_t _serial_state;


