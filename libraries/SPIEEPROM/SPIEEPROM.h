/*
 * spieeprom.h - library for SPI EEPROM IC's
 * https://bitbucket.org/trunet/spieeprom/
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

#ifndef SPIEEPROM_h
#define SPIEEPROM_h

#include <SPI.h> // relies on arduino SPI library

//opcodes
#define WREN  6
#define WRDI  4
#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

#define SPI_EEPROM_SETTINGS SPISettings(5000000, MSBFIRST, SPI_MODE0)
#define SPI_EEPROM_DEFAULT_PAGE_BOUNDERY 32

class SPIEEPROM
{
  private:
    byte eeprom_type;
    int _slave_select_pin;
    int _page_boundry;

    void send_address(long addr);
    void start_write();
    bool isWIP(); // is write in progress?

  public:
    SPIEEPROM();          // default to type 0
    SPIEEPROM(byte type); // type=0: 16-bits address
                          // type=1: 24-bits address
                          // type>1: defaults to type 0
    SPIEEPROM(byte type, int page_boundry);
    void begin(int slave_select_pin);

    template <typename T> T &write(long addr, T &value)
    {
        SPI.beginTransaction(SPI_EEPROM_SETTINGS);
        byte *p = (byte*) &value;
        start_write(); // send WRITE command
        send_address(addr); // send address
        unsigned int i;
        for (i = 0; i < sizeof(value); i++)
        {
            SPI.transfer(*p++);

            if ((addr+i+1) % _page_boundry == 0 && i+1 < sizeof(value)) {
                // at page boundry and have more to write
                // close out this page
                digitalWrite(_slave_select_pin, HIGH);
                while (isWIP()) {
                    delay(1);
                }
                digitalWrite(_slave_select_pin, HIGH);

                // and start at beginning of next
                start_write(); // send WRITE command
                send_address(addr+i+1); // send address
            }
        }
        digitalWrite(_slave_select_pin, HIGH);
        while (isWIP()) {
            delay(1);
        }
        digitalWrite(_slave_select_pin, HIGH);
        SPI.endTransaction();

        return value;
    }

    template <typename T> T &read(long addr, T &value)
    {
        SPI.beginTransaction(SPI_EEPROM_SETTINGS);
        byte* p = (byte*)(void*)&value;
        digitalWrite(_slave_select_pin, LOW);
        SPI.transfer(READ); // send READ command
        send_address(addr); // send address
        unsigned int i;
        for (i = 0; i < sizeof(value); i++)
        {
            *p++ = SPI.transfer(0xFF);
        }
        digitalWrite(_slave_select_pin, HIGH); //release chip, signal end transfer
        SPI.endTransaction();

        return value;
    }
};

#endif // SPIEEPROM_h
