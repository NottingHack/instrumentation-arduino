/*
 * SPIEEPROM.cpp - library for SPI EEPROM IC's
 * https://bitbucket.org/trunet/SPIEEPROM/
 *
 * This library is based on code by Heather Dewey-Hagborg
 * available on http://www.arduino.cc/en/Tutorial/SPIEEPROM
 *
 * by Wagner Sartori Junior <wsartori@gmail.com>
 *
 * modified by R.Boullard to add
 * writeAnything and readAnything functions
 *
 */

#include <SPI.h>
#include "SPIEEPROM.h"

SPIEEPROM::SPIEEPROM() {
    eeprom_type = 0;
    _page_boundry = SPI_EEPROM_DEFAULT_PAGE_BOUNDERY;
}

SPIEEPROM::SPIEEPROM(byte type) {
    if (type>1) {
        eeprom_type = 0;
    } else {
        eeprom_type = type;
    }
}

SPIEEPROM::SPIEEPROM(byte type, int page_boundry): _page_boundry(page_boundry) {
    if (type>1) {
        eeprom_type = 0;
    } else {
        eeprom_type = type;
    }
}

void SPIEEPROM::begin(int slave_select_pin) {
    _slave_select_pin = slave_select_pin;
    pinMode(_slave_select_pin, OUTPUT);
    digitalWrite(_slave_select_pin, HIGH);
}

void SPIEEPROM::send_address(long addr) {
    if (eeprom_type == 1) SPI.transfer((byte)(addr >> 16));
    SPI.transfer((byte)(addr >> 8));
    SPI.transfer((byte)(addr));
}

void SPIEEPROM::start_write() {
    digitalWrite(_slave_select_pin, LOW);
    SPI.transfer(WREN); //send WREN command
    digitalWrite(_slave_select_pin, HIGH);
    digitalWrite(_slave_select_pin, LOW);
    SPI.transfer(WRITE); //send WRITE command
}

bool SPIEEPROM::isWIP() {
    byte data;
    digitalWrite(_slave_select_pin, LOW);
    SPI.transfer(RDSR); // send RDSR command
    data = SPI.transfer(0xFF); //get data byte
    digitalWrite(_slave_select_pin, HIGH);
    return (data & (1 << 0));
}
