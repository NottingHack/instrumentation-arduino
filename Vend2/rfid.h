
#ifndef VENDRFID_H
#define VENDRFID_H



#include "Config.h"
#include "debug.h"

#include <avr/pgmspace.h>
#include <MFRC522.h>

extern short gDebug;

typedef MFRC522::Uid rfid_uid;

void rfid_init();

bool rfid_poll(char *rfid_serial);
void rfid_uid_to_hex(char *uidstr, rfid_uid uid);
void rfid_cpy_uid(rfid_uid *dst, rfid_uid *src);
uint8_t rfid_eq_uid(rfid_uid u1, rfid_uid u2);

#endif





