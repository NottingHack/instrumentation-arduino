
void sendCommandLCD(uint8_t cmd);
void setCursorLCD(uint8_t col, uint8_t row);
void modeResetLCD() ;
void remapKeyp();
void writeEEPROM(byte addr, byte value);
void resetEEPROM();
byte readNumKeyp();
byte readKeyp();
void changeI2CAddress(byte value);
byte readEEPROM(byte addr);
void backlight(byte on);
void clearLCD();
void sendStr(char* b);
