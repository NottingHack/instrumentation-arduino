
/****************************************************	
 * sketch = Vend
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/...
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
 * RFID payment system for Vending Machine at 
 * Nottingham Hackspace and is part of the 
 * Hackspace Instrumentation Project
 *
 * 
 ****************************************************/


/*
 These are the global config parameters
 Including Pin Outs, IP's, TimeOut's etc
 
 Arduino Wiznet shield uses pings 10, 11, 12, 13 for the ethernet
 Nanode v5 uses pins 8, 11, 12, 13 for the ethernet
 
 */

#ifndef Config_h
#define Config_h

#include "Arduino.h"

// Update these with values suitable for your network.
static byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xAD }; // ***LWK*** should randomise this, or use nanode on chip mac, if we have pins :/ and code

enum cState {NO_CARD, NET_WAIT, GOOD, IN_USE};

/*
// Vend's Reserved IP
static byte ip[]     = { 10, 0, 0, 62 }; 

// Holly's IP address
static byte server[4] = { 192, 168, 1, 146};
*/
// Port Holly Listens on  
#define VEND_PORT 56789
  
// The Gateway Ip address
// Can be the same as the server if on same subnet  
static byte gwip[4] ={ 192,168,0,1};

// EtherShield Packet Buffer
#define BUFFER_SIZE 300
static uint8_t vendBuf[BUFFER_SIZE+1];

// RFID module Serial 9600N1
#define RFID_TX 4
#define RFID_RX 3
// query to read a serial number
byte query[8] = {0xAA, 0x00, 0x03, 0x25, 0x26, 0x00, 0x00, 0xBB};
unsigned char rfidReturn[255] = {};
unsigned long lastCardNumber;
cState card_state;
char tran_id[10]; // transaction id


// timeout in mills for how often the same card is read
#define CARD_TIMEOUT 1000
unsigned long cardTimeOut = 0;

//Input / Output
#define SPEAKER A2


#define RETURN_BUTTON 9

#define LED_ACTIVE 2
#define LED_CARD 6
#define LED_WAIT 2
// used to simulate card read
//#define TEMP_BUTTON 7

#define EEPROM_DEBUG 0

#define PING_INTERVAL (400 * 60)

// Maximum time between received network messages before resetting
#define NET_RESET_TIMEOUT (400 * 120)

byte state;
// flags. most of these are used to change the response to the poll command
byte allowACK; // normally we respond with an ACK to a poll, some commands must not until they are ready!
byte cancelled; // vmc has cancelled the operation
byte commandOOS; // 1 == command was out of sequence
byte justReset;	// 1==just had reset but not been polled
byte allowVend; // 0 if not vending, 1 for allow, 2 for disallow
char vendAmountA; // amount (scaled) that is being requested.
char vendAmountB; // amount (scaled) that is being requested.
byte cancel_vend; // Cancel button pressed

//State trackers taken in reference to page 94 of 3.0 spec pdf
#define sINACTIVE	0
#define sDISABLED	1
#define sENABLED	2
#define sSESSION_IDLE	3
#define sVEND	4
#define sREVALUE	5
#define sNEGATIVE_VEND	6

// retransmit states
#define rRETRANSMIT	0
#define rCONTINUE	1
#define rUNKNOWN	2

// VMC setup values
byte vmcFeatureLevel;	// 1
byte displayColoumns;	// 0
byte displayRows;		// 0
byte displayInfo;		// 0
int maxPrice;
int minPrice;

// setup options
#define FEATURE_LEVEL	01
#define COUNTRY_CODE_HIGH	0x18
#define COUNTRY_CODE_LOW	0x26
// see page 105
#define SCALE_FACTOR	5
#define DECIMAL_PLACES	2
#define MAX_RESPONSE_TIME	5
#define MISC_OPTIONS	0x01 // Set cash sale bit only

// see page 114
#define MANUFACTURER_CODE	"NHL"
#define SERIAL_NUMBER	"000000000001"
#define MODEL_NUMBER	"000000000001"
#define OPTIONAL_FEATURE_BYTE31	0x0
#define OPTIONAL_FEATURE_BYTE32	0x0
#define OPTIONAL_FEATURE_BYTE33	0x0
#define OPTIONAL_FEATURE_BYTE34	0x0

#define CASHLESS_ADDRESS	0x10
#define ADDRESS_MASK	0xF8

// generic ACK stuff from page 25 of the 3.0 spec pdf
#define sACK	0x100
#define rACK	0x00
#define RET	0xAA
#define sNAK	0x1FF
#define rNAK	0xFF

// Commands from VMC 
// the following are taken from page 100 of the 3.0 spec pdf
// i think might arrive with 9bit set
#define RESET	0x10
#define SETUP	0x11
#define POLL	0x12
#define VEND	0x13
#define READER	0x14
#define REVALUE	0x15
#define EXPANSION	0x16

// Sub Commands
// setup
#define CONFIG_DATA 0x00
#define MINMAX_PRICES 0x01
// vend
#define VEND_REQUEST	0x00
#define VEND_CANCEL	0x01
#define VEND_SUCCESS	0x02
#define VEND_FAILURE	0x03
#define SESSION_COMPLETE	0x04
#define CASH_SALE	0x05
#define NEGATIVE_VEND_REQUEST	0x06
//reader
#define READER_DISABLE	0x00
#define READER_ENABLE	0x01
#define READER_CANCEL	0x02
#define DATE_ENTRY_RESPONSE	0x03
// revaule
#define REVALUE_REQUEST	0x00
#define REVALUE_LIMIT_REQUEST	0x01
// expansion
#define RESQUEST_ID	0x00
#define READ_USER_FILE	0x01
#define WRITE_USER_FILE	0x02
#define WRITE_TIME_DATE	0x03
#define OPTIONAL_FEATURE_ENABLE	0x04
#define FTL_REQTORCV	0xFA
#define FTL_RETRYDENY	0xFB
#define FTL_SENDBLOCK	0xFC
#define FTL_OKTOSEND	0xFD
#define FTL_REQTOSEND	0xFE
#define DIAGNOSTICS	0xFF

// responses
#define JUST_RESET	0x00
#define JUST_RESET_CHK 0x100	// pre calculated CHK
#define READER_CONFIG_DATA	0x01
#define DISPLAY_REQUEST	0x02
#define BEGIN_SESSION	0x03
#define SESSION_CANCEL_REQUEST	0x04
#define VEND_APPROVED	0x05
#define VEND_DENIED	0x06
#define VEND_DENIED_CHK	0x106
#define END_SESSION	0x07
#define CANCELLED	0x08
#define PERIPHERAL_ID	0x09
#define MALFUNCTION_ERROR	0x0A
#define CMD_OUT_OF_SEQUENCE	0x0B
#define CMD_OUT_OF_SEQUENCE_CHK	0x10B
#define REVALUE_APPROVED	0x0D
#define REVALUE_DENIED	0x0E
#define REVALUE_LIMIT_AMOUNT	0x0F
#define USER_DATA_FILE	0x10
#define TIMEDATE_REQUEST	0x11
#define DATA_ENTRY_REQUEST	0x12
#define DATA_ENTRY_CANCEL	0x13
#define FTL_REQTORCV 0x1B
#define FTL_RETRYDENY	0x1C
#define FTL_SENDBLOCK	0x1D
#define FTL_OKTOSEND	0x1E
#define FTL_REQTOSEND	0x1F
#define DIAGNOSTICS_RESPONSE	0xFF

// error codes
#define PAYMENT_MEDIA_ERROR	0x00
#define INVALID_PAYMENT_MEDIA 0x10
#define TAMPER_ERROR	0x20
#define MANUFACTURER_DEFINED_ERROR1	0x30
#define COMMUNICATIONS_ERROR	0x40
#define READER_REQUIRES_SERVICE	0x50
#define MANUFACTURER_DEFINED_ERROR2	0x70
#define READER_FAILURE	0x80
#define COMMUNICATIONS_ERROR3	0x90
#define PAYMENT_MEDIA_JAMMED	0xA0
#define MANUFACTURER_DEFINED_ERROR3	0xB0
#define REFUND_ERROR	0xC0

#endif




