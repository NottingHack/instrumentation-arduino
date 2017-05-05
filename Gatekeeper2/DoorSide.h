
#include "Arduino.h"
#include "Config.h"

#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>

// Display is a 20x2 character LCD
#define LCD_X 20
#define LCD_Y 2

typedef MFRC522::Uid rfid_uid;

class DoorSide
{
public:
  
  uint8_t _LcdAddr;
  uint8_t _DoorBellButton;
  uint8_t _Buzzer;
  uint8_t _RfidSs;
  uint8_t _RfidRst;
  uint8_t _StatusLed;
  char    _side;
  bool   *_pMqtt_connected;
  
  rfid_uid _last_card_read;
  unsigned long _last_card_read_time;
  unsigned long _door_button_last_pushed;
  unsigned long _door_button_pushed_time;
  bool _door_button_state;
  char _default_message[33];
  
  unsigned long _display_custom_msg_until; // time custom message should be displayed until, or 0 if default message is showing
  
  
  LiquidCrystal_I2C *_lcd;
  MFRC522 *_rfid_reader;
  
  DoorSide(uint8_t LcdAddr, uint8_t DoorBellButton, uint8_t Buzzer, uint8_t RfidSs, uint8_t RfidRst, uint8_t StatusLed, char side, bool *mqtt_connected);
  ~DoorSide();
  
  void init();
  char get_side();
  char *poll_rfid(); // Returns RFID serial if card found, NULL otherwise
  static void ButtonPushedInt0();
  static void ButtonPushedInt1();
  void ButtonPushed();
  bool check_button();
  void status_led();
  void show_message(char *msg, int duration = 10);
  void lcd_display(char *msg, short line = 0, boolean wipe_display = true);
  void buzzer(uint16_t tone_hz, uint16_t duration);
  void set_default_message(byte *msg, uint16_t length);
  void lcd_loop();
  void lcd_display_wordwraped(char *payload);
  
private:

  void dbg_println(const __FlashStringHelper *n);
  void dbg_println(const char *msg);
  bool output_line(int *lineno, char *str, int typ); // LCD
  void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src);
  bool eq_rfid_uid(rfid_uid u1, rfid_uid r2);
  void uid_to_hex(char *uidstr, rfid_uid uid);

  unsigned long _led_last_change;
  bool _led_on;
  volatile bool _button_pushed;
  bool _button_uses_int;
  
};

