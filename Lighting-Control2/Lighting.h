#ifndef LIGHTING_CONTROL_H
#define LIGHTING_CONTROL_H

// Device states
enum dev_state_t
{
  DEV_NO_CONN,    // No MQTT connection
  DEV_IDLE,       // Idle - tool powered off, waiting for RFID card
};


void callbackMQTT(char* topic, byte* payload, unsigned int length);
void handle_set(char* channel, byte* payload, int length);
void handle_state(char* channel, byte* payload, int length);
void update_outputs(int chan);
void read_inputs();
void publish_all_input_states();
void publish_input_state(int channel);
void publish_output_state(char* channel);
void lcd_display_mqtt(char *payload);
void checkMQTT();
void setup();
void input_int();
void state_led_loop();
void loop();
boolean set_dev_state(dev_state_t new_state);
void lcd_loop();
void check_buttons();
void send_action(const char *act, char *msg);
void lcd_display(const __FlashStringHelper *n, short line = 0, boolean wipe_display = true);
void lcd_display(char *msg, short line = 0, boolean wipe_display = true);
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);

// IP of MQTT server
byte _server[4];

char _base_topic[41];
char _dev_name [21];

// MAC / IP address of device
byte _mac[6];
byte _ip[4];

// automatic input handling
uint8_t _input_enables;
uint32_t _override_masks[8];
uint32_t _override_states[8];
uint8_t _input_statefullness;

serial_state_t _serial_state;


#endif
