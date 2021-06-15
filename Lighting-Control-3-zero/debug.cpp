#include "Arduino.h"
#include "debug.h"

#define DEBUG_SERIAL 1

void dbg_init()
{
#if DEBUG_SERIAL
  SerialUSB.begin(115200);
#endif
}

void dbg_println(void)
{
#if DEBUG_SERIAL
  SerialUSB.println();
#endif
}

void dbg_println(const __FlashStringHelper *n)
{
#if DEBUG_SERIAL
  SerialUSB.println(n);
#endif
}

void dbg_println(const char* str)
{
#if DEBUG_SERIAL
  SerialUSB.println(str);
#endif
}

void dbg_print(const char* str)
{
#if DEBUG_SERIAL
  SerialUSB.print(str);
#endif
}
