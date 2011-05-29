/****************************************************	
 * sketch = Gatekeeper
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/Gatekeeper
 * Target controller = Arduino 328 
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 002.
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
  These are the subroutines for using the SeeedStudio RFID Module
  
  Thanks to:
  John Crouchley johng at crouchley.me.uk
  Coolcomponents.co.uk
  SeeedStudo
 */


/*  
void send_command(int CMD, char* DATA, byte dataLen)
{
  int i;  //an index for the data send loop
  int checksum;
  byte STX=0xAA;
  byte STATIONID = 0x00;
  rfid.print(STX,BYTE);
  rfid.print(STATIONID,BYTE);
  
  dataLen++;  //add in the command character
  rfid.print(dataLen,BYTE);
  rfid.print(CMD,BYTE);
  
  checksum = (STATIONID ^ dataLen ^ CMD);

  for (i=0; i<dataLen-1; i++) {
    rfid.print(DATA[i],BYTE);
    checksum = checksum ^ DATA[i];
  } // end for
  
  rfid.print(checksum,BYTE);
  rfid.print(0xBB,BYTE);  
} // end void send_command(int CMD, char* DATA, byte dataLen)
*/

void read_response(char* DATARESP)
{
  byte incomingByte;
  byte responsePtr =0;
  unsigned long time = millis();
  while (millis() - time < 100) {
      // read the incoming byte:
      if (Serial.available() > 0) {
        incomingByte = Serial.read();
        DATARESP[responsePtr++]=incomingByte;
        time = millis();
      } // end if
  } // end while
  DATARESP[responsePtr]=0;
} // end void read_response(char* DATARESP)
