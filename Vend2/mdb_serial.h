

#ifndef MDBSERIAL_H
#define MDBSERIAL_H

//#define BAUD 9600

#include "Config.h"
#include "debug.h"

#include "SERCOM.h"

//#include <util/setbaud.h>

extern short gDebug;

struct MDB_Byte
{
  byte data;
  byte mode;
};

bool mdb_serial_data_available();
void mdb_serial_get_byte(struct MDB_Byte* mdbb);
uint16_t mdb_serial_USART_Receive();
void mdb_serial_init();
void mdb_serial_write(struct MDB_Byte mdbb);


#endif

