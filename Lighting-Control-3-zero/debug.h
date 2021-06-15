#ifndef DEBUG_H
#define DEBUG_H

#include "Config.h"

extern short gDebug;

void dbg_init();
void dbg_print(const char* str);
void dbg_println(void);
void dbg_println(const char* str);
void dbg_println(const __FlashStringHelper *n);

#endif
