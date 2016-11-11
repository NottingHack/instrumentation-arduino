#include <MFRC522.h>

// Device states
enum dev_state_t
{
  DEV_NO_CONN,    // No MQTT connection
  DEV_IDLE,       // Idle - tool powered off, waiting for RFID card
  DEV_AUTH_WAIT,  // Card presented, waiting for yay/nay response from server
  DEV_ACTIVE,     // Tool power enabled - RFID string in memory 
  DEV_INDUCT,     // Waiting for card to add to inducted list
  DEV_INDUCT_WAIT // Asked server to induct new card, waiting for reply.
};

typedef MFRC522::Uid rfid_uid;


void callbackMQTT(char* topic, byte* payload, unsigned int length);

void mqtt_rx_display(char *payload);
void checkMQTT();
void setup();
void signoff_button();
void induct_button();
void induct_loop();
void loop();
boolean set_dev_state(dev_state_t new_state);
void lcd_loop();
void poll_rfid();
int induct_member(rfid_uid inductee, rfid_uid inductor);
void check_buttons();
void send_action(const char *act, char *msg);
char *time_diff_to_str(unsigned long start_time, unsigned long end_time);
void dbg_println(const __FlashStringHelper *n);
void dbg_println(const char *msg);
void lcd_display(const __FlashStringHelper *n, short line = 0, boolean wipe_display = true);
void lcd_display(char *msg, short line = 0, boolean wipe_display = true);
void lcd_display_mqtt(char *payload);
void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src);
bool eq_rfid_uid(rfid_uid u1, rfid_uid r2);
void uid_to_hex(char *uidstr, rfid_uid uid);

// IP of MQTT server
byte server[] = { 192, 168, 0, 1 };

char _base_topic[41];
char _dev_name [21];

// MAC / IP address of device
byte _mac[6];
byte _ip[4];

serial_state_t _serial_state;


