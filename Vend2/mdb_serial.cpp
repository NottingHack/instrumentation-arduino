


#include "mdb_serial.h"
#include "wiring_private.h"

#define NO_RTS_PIN 255
#define NO_CTS_PIN 255

extern uint8_t _debug_level;

Sercom* _Sercom;
SERCOM* _SERCOM;

bool mdb_serial_data_available()
{
  return _Sercom->USART.INTFLAG.bit.RXC;
}

void mdb_serial_get_byte(struct MDB_Byte* mdbb)
{
  uint16_t b;
  char tmpstr[8];

  while ( !mdb_serial_data_available() );

  b = 0;
  b = mdb_serial_USART_Receive();
  memcpy (mdbb, &b, 2); 
  
  if (mdbb->mode)
    sprintf(tmpstr, "< %.2x*", mdbb->data);
  else
    sprintf(tmpstr, "< %.2x", mdbb->data);
     
  if (_debug_level > 1) dbg_print(tmpstr);
}

uint16_t mdb_serial_USART_Receive()
{
  return _Sercom->USART.DATA.bit.DATA;
}

void mdb_serial_init()
{
  // mostly ripped off from Uart.cpp

  unsigned long baudrate = 9600;
  uint8_t uc_pinRX;
  uint8_t uc_pinTX;
  SercomRXPad uc_padRX;
  SercomUartTXPad uc_padTX;
  uint8_t uc_pinRTS;
  uint8_t uc_pinCTS;

  _Sercom = SERCOM0;  // (type: Sercom*)
  _SERCOM = &sercom0; // (type: SERCOM*)

  uc_pinRX = PIN_SERIAL1_RX;
  uc_pinTX = PIN_SERIAL1_TX;
  uc_padRX = PAD_SERIAL1_RX;
  uc_padTX = PAD_SERIAL1_TX;
  uc_pinRTS = NO_RTS_PIN;
  uc_pinCTS = NO_CTS_PIN;

  pinPeripheral(uc_pinRX, g_APinDescription[uc_pinRX].ulPinType);
  pinPeripheral(uc_pinTX, g_APinDescription[uc_pinTX].ulPinType);

  if (uc_padTX == UART_TX_RTS_CTS_PAD_0_2_3) { 
    if (uc_pinCTS != NO_CTS_PIN) {
      pinPeripheral(uc_pinCTS, g_APinDescription[uc_pinCTS].ulPinType);
    }
  }

  _SERCOM->initUART(UART_INT_CLOCK, SAMPLE_RATE_x16, baudrate);
  _Sercom->USART.INTENCLR.reg = SERCOM_SPI_INTENCLR_RXC; // disable rx interupts
  _SERCOM->initFrame(UART_CHAR_SIZE_9_BITS, LSB_FIRST, SERCOM_NO_PARITY, SERCOM_STOP_BIT_1); // 9bit
  _SERCOM->initPads(uc_padTX, uc_padRX);
  _SERCOM->enableUART();

  return;
}

void mdb_serial_write(struct MDB_Byte mdbb)
{
  char tmpstr[10];
  memset(tmpstr, 0, sizeof(tmpstr));

  // Wait for data register to be empty before putting something in it
  while (!_Sercom->USART.INTFLAG.bit.DRE);

  uint16_t data = mdbb.data;

  if (mdbb.mode)
    data |= 1 << 8;

  _Sercom->USART.DATA.reg = data;

  if (mdbb.mode)
    sprintf(tmpstr, "> %.2x*", mdbb.data);
  else
    sprintf(tmpstr, "> %.2x", mdbb.data);


  if (_debug_level > 1) dbg_print(tmpstr);

  return;
}
