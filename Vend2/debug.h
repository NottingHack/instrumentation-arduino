#ifndef VENDDEBUG_H
#define VENDDEBUG_H

#include "Config.h"
#include "net_transport.h"

extern short gDebug;

void dbg_init();
void dbg_print(const char* str);
void dbg_println(const char* str);
void dbg_println(const __FlashStringHelper *n);

#endif
