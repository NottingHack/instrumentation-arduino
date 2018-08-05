

#include "rfid.h"

MFRC522 *_rfid_reader; 

rfid_uid _lastCardNumber = {0};
unsigned long _cardTimeOut = 0;

void rfid_init()
{
  SPI.begin();
  _rfid_reader = new MFRC522(PIN_RFID_SS, PIN_RFID_RST);
  _rfid_reader->PCD_Init();
}

bool rfid_poll(char *rfid_serial)
{
  if (gDebug > 1)
    dbg_println(F("Poll R"));

  if (!_rfid_reader->PICC_IsNewCardPresent())
    return false;

  if (!_rfid_reader->PICC_ReadCardSerial())
    return false;

  // check we are not reading the same card again or if we are its been a sensible time since last read it
  if (!rfid_eq_uid(_rfid_reader->uid, _lastCardNumber) || (millis() - _cardTimeOut) > CARD_TIMEOUT)
  {
    rfid_cpy_uid(&_lastCardNumber, &_rfid_reader->uid);
    _cardTimeOut = millis();

    // convert cardNumber to a string and send out
    rfid_uid_to_hex(rfid_serial, _rfid_reader->uid);
    return true;
  }

  return false;
}

//void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src)
void rfid_cpy_uid(rfid_uid *dst, rfid_uid *src)
{
  memcpy(dst, src, sizeof(rfid_uid));
}

// uint8_t eq_rfid_uid(rfid_uid u1, rfid_uid u2)
uint8_t rfid_eq_uid(rfid_uid u1, rfid_uid u2)
{
  if (memcmp(&u1, &u2, sizeof(rfid_uid)))
    return false;
  else
    return true;
}

//void uid_to_hex(char *uidstr, rfid_uid uid)
void rfid_uid_to_hex(char *uidstr, rfid_uid uid)
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



