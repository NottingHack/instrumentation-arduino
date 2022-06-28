/****************************************************	
 * sketch = Gatekeeper
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/Gatekeeper
 * Target controller = Arduino 328 with Wiznet shield
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 1.0.1
 * C compiler = WinAVR from Arduino IDE 1.0.1
 * 
 * 
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 * Gatekeeper is the access control system at 
 * Nottingham Hackspace and is part of the 
 * Hackspace Instrumentation Project
 *
 * 
 ****************************************************/

/*
 These are the subroutines for using the ATTiny2313 I2C Keypad and LCD controller.
 
 Thanks to:
	John Crouchley 
	johng at crouchley.me.uk
 
 */

#include <Arduino.h>
#include <Wire.h>
#include "Station_Control.h"

void sendStr(char* b)
{
	Wire.beginTransmission(0x12); // transmit to device 12
	while (*b)
	{
		if (*b == 0xfe || *b == 0xff) Wire.write(0xfe);
		Wire.write(*b++); // sends one byte
	}
	Wire.endTransmission(); // stop transmitting
	delay(2);
}


void clearLCD()
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xfe); // signal command follows
	Wire.write(0x01); // send the command
	Wire.endTransmission(); // stop transmitting
	delay(2);
}

void backlight(byte on)
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xFF); // sends command flag
	Wire.write(0x01); // backlight
	Wire.write(on); // sends on/off flag
	Wire.endTransmission(); // stop transmitting
	delay(1); // always a good idea to give the
	// I2C device a chance to catch up.
}


byte readEEPROM(byte addr)
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0x02); // send command read EEPROM
	Wire.write(addr); // send the address
	Wire.endTransmission(); // stop transmitting
	delay(1); // give it a chance
	Wire.requestFrom(0x12,1); // ask for the byte
	return Wire.read(); // and return it
}

void changeI2CAddress(byte value)
{
	Wire.beginTransmission(0x00); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0x03); // send the command write
	// EEPROM
	Wire.write(0x00); // EEPROM addr is I2C address
	Wire.write(value); // send the new I2C address
	Wire.endTransmission(); // stop transmitting
	delay(1);
	Wire.beginTransmission(0x00); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0xf1); // send the reset
	Wire.endTransmission(); // stop transmitting
	delay(4);
	// the address is now changed and you need to use the new address
}

byte readKeyp()
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xFF); // sends command flag
	Wire.write(0x11); // read next key
	Wire.endTransmission(); // stop transmitting
	delay(2);
	Wire.requestFrom(0x12,1);
	return Wire.read(); // get the next char in the
	// buffer
	// note zero is returned if none
}


// 'RepRap'
byte readNumKeyp()
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xFF); // sends command flag
	Wire.write(0x10); // read number of keys in buffer
	Wire.endTransmission(); // stop transmitting
	delay(2);
	Wire.requestFrom(0x12,1);
	return Wire.read(); // get the number of chars in the
	// buffer
	// note zero is returned if none
}

void resetEEPROM()
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0xf0); // send the command to reset EEPROM to default
	Wire.endTransmission(); // stop transmitting
	delay(1);
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0xf1); // send the reset
	Wire.endTransmission(); // stop transmitting
	delay(4);
}


void writeEEPROM(byte addr, byte value)
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0x03); // send the command write
	// EEPROM
	Wire.write(addr); // EEPROM addr to chnage
	Wire.write(value); // send the new value
	Wire.endTransmission(); // stop transmitting
	/* delay(1);
	 Wire.beginTransmission(0x12); // transmit to device 12
	 Wire.write(0xff); // signal command follows
	 Wire.write(0xf1); // send the reset
	 Wire.endTransmission(); // stop transmitting
	 */
	delay(4);
}


// the station contoler keymap needs a change from the default values
// use the following only if the EEPROM has been reset or is giving unkown results
void remapKeyp()
{
	writeEEPROM(9, 0x37);	// 7
	writeEEPROM(10, 0x38);	// 8
	writeEEPROM(11, 0x39);	// 9
	writeEEPROM(12, 0x34);	// 4
	writeEEPROM(13, 0x35);	// 5
	writeEEPROM(14, 0x2B);	// 6
	writeEEPROM(15, 0x31);	// 1
	writeEEPROM(16, 0x32);	// 2
	writeEEPROM(17, 0x33);	// 3
	writeEEPROM(18, 0x2A);	// *
	writeEEPROM(19, 0x30);	// 0
	writeEEPROM(20, 0x23);	// #
	// writeEEPROM doesnt reload the values once writen
	delay(1);
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0xf1); // send the reset
	Wire.endTransmission(); // stop transmitting
	delay(4);
}

void modeResetLCD() 
{
	//wrtieEEPROM(2, 0x28);  // LCD Function Mode
	writeEEPROM(3, 0x0C);  // LCD cursor direction : gatekeepr doesnt show the cursor
	//writeEEPROM(4, 0x06);  // LCD entry mode
	//writeEEPROM(5, 0x01);  // LCD clear
	// writeEEPROM doesnt reload the values once writen
	delay(1);
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xff); // signal command follows
	Wire.write(0xf1); // send the reset
	Wire.endTransmission(); // stop transmitting
	delay(4);
}

void setCursorLCD(uint8_t col, uint8_t row)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > 1 ) {
		row = 0;    // we count rows starting w/0
	}
	sendCommandLCD(0x80 | (col + row_offsets[row]));
}

void sendCommandLCD(uint8_t cmd)
{
	Wire.beginTransmission(0x12); // transmit to device 12
	Wire.write(0xfe); // signal command follows
	Wire.write(cmd); // send the command
	Wire.endTransmission(); // stop transmitting
	delay(2);
}
