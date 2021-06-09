#include <SPIEEPROM.h>

#define SLAVESELECT 11

SPIEEPROM eeprom(0); // parameter is type
                     // type = 0: 16-bits address
                     // type = 1: 24-bits address
                     // type > 1: defaults to type 0

float testWrite[8] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
float testRead[8];
long address = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  SPI.begin();
  eeprom.begin(SLAVESELECT);
}

void loop() {
  eeprom.write(address, testWrite);
  eeprom.read(address, testRead);
  for (int i = 0; i < 8; i++)
  {
    Serial.print(testRead[i]);
    Serial.print(" ");
  }
  Serial.println();
  address += 32; // 4 bytes per float value -> 8x4 bytes
  for (int i = 0; i < 8; i++)
  {
    testWrite[i] += 8;
  }
  delay(200);
}