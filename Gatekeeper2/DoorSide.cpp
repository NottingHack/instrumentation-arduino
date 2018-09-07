
#include "DoorSide.h"

class DoorSide *int0 = NULL;
class DoorSide *int1 = NULL;

DoorSide::DoorSide(uint8_t LcdAddr, uint8_t DoorBellButton, uint8_t Buzzer, uint8_t RfidSs, uint8_t RfidRst, uint8_t StatusLed, char side, bool *mqtt_connected)
  : _LcdAddr(LcdAddr), _DoorBellButton(DoorBellButton), _Buzzer(Buzzer), _RfidSs(RfidSs), _RfidRst(RfidRst), _StatusLed(StatusLed), _side(side), _pMqtt_connected(mqtt_connected)
{

  _lcd = new LiquidCrystal_I2C(_LcdAddr,20,2);  // 20x2 display
  _rfid_reader = new MFRC522(_RfidSs, _RfidRst);
  
  _last_card_read = {0};
  _last_card_read_time = 0;
  _led_last_change = 0;
  _led_on = false;
  _door_button_state = HIGH;
  _display_custom_msg_until = 0;
  _door_button_last_pushed = 0;
  _door_button_pushed_time = 0;
  _button_pushed = false;
  _button_uses_int = false;
  
  memset(_default_message, 0, sizeof(_default_message));
  
  if (_DoorBellButton != 0xFF) // 0xFF means no door bell button
  {
    pinMode(_DoorBellButton, INPUT);
    digitalWrite(_DoorBellButton, HIGH);
  }
  else if (_DoorBellButton == 2 or _DoorBellButton == 3)
  {
    // PINs 2+3 on the 328 can generate interupts. So attach an interupt handler if used

    _button_uses_int = true;
    switch (_DoorBellButton)
    {
      case 2: // digital pin 2 is int0
        int0 = this; 
        attachInterrupt((_DoorBellButton == 2 ? 0 : 1), ButtonPushedInt0, CHANGE);
        break;

      case 3: // pin3 = int1
        int1 = this; 
        attachInterrupt((_DoorBellButton == 2 ? 0 : 1), ButtonPushedInt1, CHANGE);
        break;
        
      default:
        // This shouldn't happen...
        _button_uses_int = false;
        break;
    }
  }
}

DoorSide::~DoorSide()
{
  delete _lcd;
  delete _rfid_reader;
}


void DoorSide::init()
{
   char build_ident[17]="";
  dbg_println(F("Init LCD"));
  _lcd->init();
  _lcd->backlight();
  snprintf(build_ident, sizeof(build_ident), "FW:%s", BUILD_IDENT);
  lcd_display(build_ident);
  dbg_println(build_ident);
  
  dbg_println(F("Init RFID"));
  _rfid_reader->PCD_Init();
}


char* DoorSide::poll_rfid()
{
  static char rfid_serial_str[20];

  boolean got_card = false;
  rfid_uid card_number;

  digitalWrite(_RfidRst, LOW);

  _rfid_reader->PCD_Init();

  // Look for RFID card
  if (_rfid_reader->PICC_IsNewCardPresent())
  {
    dbg_println(F("card present"));
    if (_rfid_reader->PICC_ReadCardSerial())
    {
      dbg_println(F("Read card"));
      got_card=true;
    }
  }

  if (!got_card)
    return NULL;

  cpy_rfid_uid(&card_number, &_rfid_reader->uid);

  // If the same card has already been seen recently, don't bother querying the server again
  if (eq_rfid_uid(_last_card_read, card_number) && ((millis() - _last_card_read_time) < CARD_TIMEOUT))
  {
    dbg_println(F("Card seen too recently - ignoring"));
    return NULL;
  }

  cpy_rfid_uid(&_last_card_read, &card_number);
  _last_card_read_time = millis();

  uid_to_hex(rfid_serial_str, card_number);

  return rfid_serial_str;
}

void DoorSide::cpy_rfid_uid(rfid_uid *dst, rfid_uid *src)
{
  memcpy(dst, src, sizeof(rfid_uid));
}

bool DoorSide::eq_rfid_uid(rfid_uid u1, rfid_uid u2)
{
  if (memcmp(&u1, &u2, sizeof(rfid_uid)))
    return false;
  else
    return true;
}

void DoorSide::uid_to_hex(char *uidstr, rfid_uid uid)
{
  switch (uid.size)
  {
    case 4:
      sprintf(uidstr, "%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3]);
      break;

    case 7:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6]);
      break;

    case 10:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6], uid.uidByte[7], uid.uidByte[8], uid.uidByte[9]);
      break;

    default:
      sprintf(uidstr, "ERR:%d", uid.size);
      break;
  }
}

char DoorSide::get_side()
{
  return _side;
}

void DoorSide::ButtonPushedInt0()
{
  if (int0)
    int0->ButtonPushed();
}

void DoorSide::ButtonPushedInt1()
{
  if (int1)
    int1->ButtonPushed();
}

void DoorSide::ButtonPushed()
{
  _button_pushed = true;
}

bool DoorSide::check_button()
// Returns true if the door bell button has been pushed for DOOR_BUTTON_DELAY ms, and hasn't
// been pushed in the last DOOR_BUTTON_INTERVAL ms.
// Return false otherwise.
{

  if (_DoorBellButton == 0xFF) // 0xFF means no button
    return false;
  
  bool button_state = digitalRead(_DoorBellButton);
  
  if (_button_uses_int)
  {
    if (!_button_pushed)
    {
      return false;
    }
    else
    {
      _button_pushed = false;

      // button push recorded by INT handler, is it still pushed?
      if (button_state == HIGH)
      {
        // not pushed
        return false;
      }
      else
      {
        // button still pushed
        if ((millis() - _door_button_last_pushed) > DOOR_BUTTON_INTERVAL) // only allow a button press every DOOR_BUTTON_INTERVAL ms to be acknowledged
        {
          // Ok, now count the button as pressed...
          _door_button_last_pushed = millis();
          return true;
        }
      }
    }

    // Done with door-button-uses-interupt logic
    return false;
  }

  // and logic for if the door bell button isn't on an interupt pin...
  if (button_state == HIGH)
  {
    _door_button_pushed_time = 0;
    _door_button_state = HIGH;

    return false; // not pushed
  }

  if (_door_button_state == HIGH) 
  {
    // door button now pushed, and it wasn't before. Wait x ms before acting on this
    _door_button_state = LOW;
    _door_button_pushed_time = millis();
    return false;
  }

  if ((millis() - _door_button_pushed_time) > DOOR_BUTTON_DELAY)
  {
    if (digitalRead(_DoorBellButton) == LOW) // is button still pushed?
    {
      if ((millis() - _door_button_last_pushed) > DOOR_BUTTON_INTERVAL) // only allow a button press every DOOR_BUTTON_INTERVAL ms to be acknowledged
      {
        // Ok, now count the button as pressed...
        _door_button_last_pushed = millis();
        return true;
      }
    }
  }

  return false;
}

void DoorSide::status_led()
{
  
  if (*_pMqtt_connected)
  {
    pinMode(_StatusLed, OUTPUT);
    digitalWrite(_StatusLed, HIGH);
  }
  else
  {
    if (millis()-_led_last_change > STATE_FLASH_FREQ)
    {
      _led_last_change = millis();
      if (_led_on)
      {
        _led_on = false;
        pinMode(_StatusLed, INPUT); // switch off
      }
      else
      {
        _led_on = true;
        pinMode(_StatusLed, OUTPUT);
        digitalWrite(_StatusLed, LOW);
      }
    }
  }
}

void DoorSide::buzzer(uint16_t tone_hz, uint16_t duration)
{
  tone(_Buzzer, tone_hz, duration);
}

void DoorSide::set_default_message(byte *msg, uint16_t length)
{
  // copy as much of <msg> into _default_message as will fit
  memset(_default_message, 0, sizeof(_default_message));
  memcpy(_default_message, msg, (length > (sizeof(_default_message)-1) ? sizeof(_default_message)-1 : length));
  lcd_display(_default_message);
}

void DoorSide::show_message(char *msg, int duration)
{
  //dbg_println(F("show_message"));
  _lcd->clear();
  lcd_display_wordwraped(msg);
  _display_custom_msg_until = millis() + duration;
}

void DoorSide::lcd_loop()
{
  if (_display_custom_msg_until)
  {
    if (millis() > _display_custom_msg_until)
    {
      _display_custom_msg_until = 0;
      lcd_display(_default_message);
    }
  }
}

void DoorSide::lcd_display(char *msg, short line, boolean wipe_display)
{
  if (wipe_display)
    _lcd->clear();

  _lcd->setCursor(0, line);
  _lcd->print(msg);
}


bool DoorSide::output_line(int *lineno, char *str, int typ)
/****************************************************
 * Output line to LCD, and increment lineno.
 * If last line of LCD written, returns TRUE
 ****************************************************/
{

  _lcd->setCursor(0, (*lineno)++);
  _lcd->print(str);

  if (*lineno >= LCD_Y)
    return true;
  else
    return false;
}

void DoorSide::lcd_display_wordwraped(char *msg)
{
  int word_len, space_remain;
  int line_no = 0;
  char msg_line[LCD_X+1];
  char *mptr = msg;
 
  space_remain = LCD_X;
  msg_line[0] = '\0';
  while (*mptr!='\0')
  {
    word_len = 0;
    while (*mptr!='\0' && *mptr++!=' ')
      word_len++;

    // Special case - the word is longer than the display width
    if (word_len > LCD_X)
    {
      // output current line
      if (msg_line[0] != '\0')
        if(output_line(&line_no, msg_line, 1)) return;

      // Start new line
      msg_line[0] = '\0'; 
      space_remain = LCD_X;  

      while (word_len > 0)
      {
        memset(msg_line, 0, sizeof(msg_line));
        strncpy(msg_line, (mptr-word_len), (word_len >= LCD_X ? LCD_X : word_len));
        msg_line[LCD_X] = '\0';
        if (word_len > LCD_X)
          if(output_line(&line_no, msg_line, 2)) return;

        //word_len -= LCD_X;
        word_len -= (word_len >= LCD_X ? LCD_X : word_len);
      }
      strcat(msg_line, " ");

      space_remain = LCD_X - strlen(msg_line);

    }
    else if (word_len <= space_remain) // space left on current line -> add to line
    {
      if (*mptr)
      {
        strncat(msg_line, (mptr-word_len-1), word_len);
        if (word_len < space_remain) 
         strcat(msg_line, " ");
        space_remain -= word_len+1;
      }
      else
      {
        strncat(msg_line, (mptr-word_len), word_len);
        space_remain -= word_len;
      }
    } 
    else // Not enough space left - output current line, and add word to new line
    {
      // output
      if(output_line(&line_no, msg_line, 3)) return;

      // Start new line
      msg_line[0] = '\0';
      space_remain = LCD_X;

      strncat(msg_line, (mptr-word_len - ((*(mptr-word_len-1) == ' ') ? 0 : 1) ), word_len);

      strcat(msg_line, " ");
    }
  }

  if (msg_line[0] != '\0')
    if (output_line(&line_no, msg_line, 4)) return;

}

void DoorSide::dbg_println(const char *msg)
{
  Serial.print(_side);
  Serial.print(": ");
  Serial.println(msg);
}

void DoorSide::dbg_println(const __FlashStringHelper *n)
{
  /*
  uint8_t c;
  byte *payload;
  char *pn = (char*)n;

  memset(_pmsg, 0, sizeof(_pmsg));
  strcpy(_pmsg, "INFO:");
  payload = (byte*)_pmsg + sizeof("INFO");

  while ((c = pgm_read_byte_near(pn++)) != 0)
    *(payload++) = c;

*/
  Serial.print(_side);
  Serial.print(": ");
  Serial.println(n);

}
