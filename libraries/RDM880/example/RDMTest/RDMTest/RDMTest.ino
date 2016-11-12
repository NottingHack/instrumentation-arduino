#include <SoftwareSerial.h>
#include <RDM880.h>

typedef RDM880::Uid rfid_uid;
rfid_uid lastCardNumber = {0};

#define RFID_PWR 4
#define RFID_GND 5
#define RFID_TX 6
#define RFID_RX 7

SoftwareSerial rfid_reader(RFID_RX,RFID_TX);

RDM880 _rdm_reader(&rfid_reader);
uint32_t cardTimeOut = 0;
char rfid_serial[21];

void setup() {
  pinMode(RFID_GND, OUTPUT);
  pinMode(RFID_PWR, OUTPUT);
  digitalWrite(RFID_GND, LOW);
  digitalWrite(RFID_PWR, HIGH);
    Serial.begin(9600);
    rfid_reader.begin(9600);
    Serial.print("hello");
    delay(1);
}

void loop() {
  pollRFID();
  Serial.print("poll");
  delay(10);
}

bool pollRFID()
{

  if (_rdm_reader.mfGetSerial()) {
    Serial.print("got Serial");
    // check we are not reading the same card again or if we are its been a sensible time since last read it
    if (!eq_rfid_uid(_rdm_reader.uid, lastCardNumber) || (millis() - cardTimeOut) > 100) {
        cpy_rfid_uid(&lastCardNumber, &_rdm_reader.uid);
        cardTimeOut = millis();
        // convert cardNumber to a string and send out
        uid_to_hex(rfid_serial, _rdm_reader.uid);
        Serial.println(rfid_serial);
        return true;
    } // end if
  }
  return false;
} // end bool pollRFID()

/**************************************************** 
 * RFID Helpers
 *  
 ****************************************************/
 
void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src)
{
  memcpy(dst, src, sizeof(rfid_uid));
}

bool eq_rfid_uid(rfid_uid u1, rfid_uid u2)
{
  if (memcmp(&u1, &u2, sizeof(rfid_uid)))
    return false;
  else
    return true;
}
void uid_to_hex(char *uidstr, rfid_uid uid)
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