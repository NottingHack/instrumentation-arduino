/****************************************************
 * sketch = nh-485io
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = https://wiki.nottinghack.org.uk/wiki/Lighting_Automation/Switch
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
 History
  000 - Started 02/05/2021
  001 - Initial release

 Known issues:

 Future changes:

 ToDo:

 Authors:
 'RepRap' Matt    dps.lwk at gmail.com
 James Hayward    jhayward1980 at gmail.com
 Daniel Swann     hs at dswann.co.uk

 For more details, see:
 https://wiki.nottinghack.org.uk/wiki/Lighting_Automation/Switch
 */

#include "Config.h"
#include "debug.h"
#include "serial_menu.h"

#include <FlashAsEEPROM.h>
#include <avr/pgmspace.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

uint8_t _modbus_address = 10;
uint16_t _direction_mask = 0x0000;
uint8_t _debug_level = 1;

volatile boolean _input_int_pushed;
volatile unsigned long _input_int_pushed_time;
uint16_t _input_state = 0;
volatile unsigned long _input_pushed_time[NUMBER_IO_CHANNELS];
uint16_t _output_state = 0;

char buf[25];

uint8_t _pins[NUMBER_IO_CHANNELS] = {
    PIN_IO_0,
    PIN_IO_1,
    PIN_IO_2,
    PIN_IO_3,
    PIN_IO_4,
    PIN_IO_5,
    PIN_IO_6,
    PIN_IO_7,
    PIN_IO_8,
    PIN_IO_9,
    PIN_IO_10,
    PIN_IO_11,
    PIN_IO_12,
    PIN_IO_13,
    PIN_IO_14,
    PIN_IO_15
};


void input_interrupt()
{
  _input_int_pushed_time = millis();
  _input_int_pushed = true;
}

void read_inputs()
{
  for (int chan = 0; chan < NUMBER_IO_CHANNELS; chan++) {
    if ((_direction_mask & (1UL<<(chan))) == 0) { // direction is input
      if (digitalRead(_pins[chan]) == LOW) { // LOW == Pressed button
        if ((_input_state & (1UL<<(chan))) == 0) {
          _input_state |= (1UL<<(chan)); // set bit

          ModbusRTUServer.discreteInputWrite(chan, 1); // ON

          if (_debug_level == 2) {
            sprintf(buf, "Input [%d] is Low", chan);
            dbg_println(buf);
          }
        }
      } else {
        if ((_input_state & (1UL<<(chan))) != 0) {
          _input_state &= ~(1UL<<(chan)); // clear bit

          ModbusRTUServer.discreteInputWrite(chan, 0); // OFF

          if (_debug_level == 2) {
            sprintf(buf, "Input [%d] is High", chan);
            dbg_println(buf);
          }
        }
      }
    }
  }

  ModbusRTUServer.inputRegisterWrite(0x00, _input_state);
}

void set_outputs()
{
  for (int chan = 0; chan < NUMBER_IO_CHANNELS; chan++) {
    if ((_direction_mask & (1UL<<(chan))) == 1) { // direction is output
      // read the current value of the coil
      if (ModbusRTUServer.coilRead(chan)) {
        if ((_output_state & (1UL<<(chan))) != 1) {
          _output_state |= (1UL<<(chan)); // set bit

          // coil value set, turn output on
          digitalWrite(_pins[chan], HIGH);

          if (_debug_level == 2) {
            sprintf(buf, "Output [%d] set High", chan);
            dbg_println(buf);
          }
        }
      } else {
        if ((_output_state & (1UL<<(chan))) != 0) {
          _output_state &= ~(1UL<<(chan)); // clear bit

          // coil value clear, turn output off
          digitalWrite(_pins[chan], LOW);

          if (_debug_level == 2) {
            sprintf(buf, "Output [%d] set High", chan);
            dbg_println(buf);
          }
        }
      }
    }
  }
}

void setup()
{
  dbg_init();
  delay(4000);

  serial_menu_init(); // Also reads in network config from EEPROM

  char build_ident[17]="";
  snprintf(build_ident, sizeof(build_ident), "FW:%s", BUILD_IDENT);
  dbg_println(build_ident);

  RS485.setPins(RS485_DEFAULT_TX_PIN, RS485_DE, RS485_RE);
  // start the Modbus RTU client
  if (!ModbusRTUServer.begin(_modbus_address, RS485_BAUD, RS485_SERIAL_CONFIG)) {
    dbg_println(F("Failed to start Modbus RTU Server!"));
    while (1);
  } else {
    sprintf(buf, "Modbus Address %d", _modbus_address);
    dbg_println(buf);
  }

  for (int chan = 0; chan < NUMBER_IO_CHANNELS; chan++) {
    if (_direction_mask & (1UL<<(chan))) {
        pinMode(_pins[chan], OUTPUT);
        digitalWrite(_pins[chan], LOW);

        sprintf(buf, "Pin I%d Output", chan);
        dbg_println(buf);
    } else {
        pinMode(_pins[chan], INPUT_PULLUP);
        digitalWrite(_pins[chan], HIGH);
        attachInterrupt(digitalPinToInterrupt(_pins[chan]), input_interrupt, FALLING);

        sprintf(buf, "Pin I%d Input", chan);
        dbg_println(buf);
    }
  }

  // configure coils at address 0x00
  ModbusRTUServer.configureCoils(0x00, NUMBER_IO_CHANNELS);

  // configure discrete inputs at address 0x00
  ModbusRTUServer.configureDiscreteInputs(0x00, NUMBER_IO_CHANNELS);

  // // configure holding registers at address 0x00
  // ModbusRTUServer.configureHoldingRegisters(0x00, 1);

  // configure input registers at address 0x00
  ModbusRTUServer.configureInputRegisters(0x00, 1);

  dbg_println(F("Setup done..."));

  Serial.println();
  serial_show_main_menu();
}

void loop()
{
  // poll for Modbus RTU requests
  ModbusRTUServer.poll();

  // read inputs and update Modbus registers
  read_inputs();

  // set the outputs
  set_outputs();

  // Do serial menu
  serial_menu();
}
