/****************************************************
 * sketch = nh-485io
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = https://wiki.nottinghack.org.uk/wiki/Vending_Machine/Cashless_Device_Implementation
 * Target controller = SAM D21 / Arduino Zero
 * Development platform = Arduino IDE 1.8.5
 *
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * RS485 16 port io module for Lighting Automation Switches at
 * Nottingham Hackspace and is part of the
 * Hackspace Instrumentation Project
 *
 *
 ****************************************************/


/*
 These are the global config parameters
 Including Pin Outs, IP's, TimeOut's etc
 */

#ifndef NH_485IO_CONFIG_H
#define NH_485IO_CONFIG_H

#include "Arduino.h"

/* Pin assigments */
#define PIN_IO_1          A0
#define PIN_IO_2          A1
#define PIN_IO_3          A2
#define PIN_IO_4          A3
#define PIN_IO_5          A4
#define PIN_IO_0          A5
#define RS485_RE           2
#define PIN_IO_8           3
#define RS485_DE           4
#define PIN_IO_9           5
#define PIN_IO_14          6
#define PIN_IO_15          7
#define PIN_IO_6           8
#define PIN_IO_7           9
#define PIN_IO_12         10
#define PIN_IO_10         11
#define PIN_IO_13         12
#define PIN_IO_11         13

#define NUMBER_IO_CHANNELS   16

// Locations in EEPROM of various settings
#define EEPROM_MODBUS_ADDRESS       0 //  1 bytes
#define EEPROM_IO_DIRECTION         1 //  2 bytes  0bit == input, 1 == output
#define EEPROM_DEBUG                3 //  1 byte

#define RS485_BAUD 9600
#define RS485_SERIAL_CONFIG SERIAL_8E1

#endif
