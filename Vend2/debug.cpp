#include "debug.h"

#define DEBUG_NETWORK 0
#define DEBUG_SERIAL 1

void dbg_init()
{
#if DEBUG_SERIAL
  SerialUSB.begin(115200);
#endif
}


void dbg_println(const __FlashStringHelper *n)
{
  uint8_t c;
  byte *payload;

  if (gDebug > 0)
  {

#if DEBUG_SERIAL
    SerialUSB.println(n);
#endif
    
#if DEBUG_NETWORK
    char *pn = (char*)n;
    struct net_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    payload = msg.payload;

    memcpy(msg.msgtype,"INFO", 4);

    while ((c = pgm_read_byte_near(pn++)) != 0)
      *(payload++) = c;

    net_transport_send(&msg);
#endif

  }
}

void dbg_println(const char* str)
{
  //todo.
  dbg_print(str);
}

void dbg_print(const char* str)
{
 // if (gDebug > 0)
  {
#if DEBUG_SERIAL
    SerialUSB.println(str);
#endif

#if DEBUG_NETWORK
    struct net_msg_t msg;
    memcpy(msg.msgtype,"INFO", 4);
    strncpy((char*)msg.payload, str, sizeof(msg.payload));
    net_transport_send(&msg);
#endif

  }
}
