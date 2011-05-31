/****************************************************	
 * sketch = Gatekeeper
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/Gatekeeper
 * Target controller = Arduino 328 (Nanode v5)
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 0022
 * C compiler = WinAVR from Arduino IDE 0022
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


void sendStr(char* b)
{
  Wire.beginTransmission(0x12); // transmit to device 12
  while (*b)
  {
    if (*b == 0xfe || *b == 0xff) Wire.send(0xfe);
    Wire.send(*b++); // sends one byte
  }
  Wire.endTransmission(); // stop transmitting
  delay(2);
}


void clearLCD()
{
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xfe); // signal command follows
  Wire.send(0x01); // send the command
  Wire.endTransmission(); // stop transmitting
  delay(2);
}

void backlight(byte on)
{
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xFF); // sends command flag
  Wire.send(0x01); // backlight
  Wire.send(on); // sends on/off flag
  Wire.endTransmission(); // stop transmitting
  delay(1); // always a good idea to give the
  // I2C device a chance to catch up.
}


byte readEEPROM(byte addr)
{
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xff); // signal command follows
  Wire.send(0x02); // send command read EEPROM
  Wire.send(addr); // send the address
  Wire.endTransmission(); // stop transmitting
  delay(1); // give it a chance
  Wire.requestFrom(0x12,1); // ask for the byte
  return Wire.receive(); // and return it
}

void changeI2CAddress(byte value)
{
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xff); // signal command follows
  Wire.send(0x03); // send the command write
  // EEPROM
  Wire.send(0x00); // EEPROM addr is I2C address
  Wire.send(value); // send the new I2C address
  Wire.endTransmission(); // stop transmitting
  delay(1);
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xff); // signal command follows
  Wire.send(0xf1); // send the reset
  Wire.endTransmission(); // stop transmitting
  delay(4);
  // the address is now changed and you need to use the new address
}

byte readKeyp()
{
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xFF); // sends command flag
  Wire.send(0x11); // read next key
  Wire.endTransmission(); // stop transmitting
  delay(2);
  Wire.requestFrom(0x12,1);
  return Wire.receive(); // get the next char in the
  // buffer
  // note zero is returned if none
}


// 'RepRap'
byte readNumKeyp()
{
  Wire.beginTransmission(0x12); // transmit to device 12
  Wire.send(0xFF); // sends command flag
  Wire.send(0x10); // read number of keys in buffer
  Wire.endTransmission(); // stop transmitting
  delay(2);
  Wire.requestFrom(0x12,1);
  return Wire.receive(); // get the next char in the
  // buffer
  // note zero is returned if none
}