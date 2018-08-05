
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

 */

#ifndef VENDCONFIG_H
#define VENDCONFIG_H

#include "Arduino.h"

enum cState {NO_CARD, NET_WAIT, GOOD, IN_USE};


/***************
 *** Network ***
 ***************/

// MQTT stuff
#define MQTT_PORT 1883
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"

// Maximum time between received network messages before resetting
#define NET_RESET_TIMEOUT 48000 // (400 * 120)
#define PING_INTERVAL (400 * 60)

#define NET_BASE_TOPIC_SIZE 41
#define NET_DEV_NAME_SIZE   21

/***************
 ***  RFID   ***
 ***************/
#define PIN_RFID_RST      6
#define PIN_RFID_SS       7

// timeout in mills for how often the same card is read
#define CARD_TIMEOUT 1000


/***************
 ***   IO    ***
 ***************/
#define RETURN_BUTTON 2
#define LED_ACTIVE 8
#define LED_CARD 3


/***************
 ***  LCD    ***
 ***************/
#define LCD_I2C_ADDRESS 0x27
#define LCD_WIDTH 16


/***************
 *** EEPROM  ***
 ***************/
// Locations in EEPROM of various settings
#define EEPROM_MAC           0 //  6 bytes
#define EEPROM_IP            6 //  4 bytes
#define EEPROM_BASE_TOPIC   10 // 40 bytes   e.g. "nh/vend/"
#define EEPROM_NAME         50 // 20 bytes   e.g. "CanVend"
#define EEPROM_SERVER_IP    70 //  4 bytes
#define EEPROM_DEBUG        74 //  1 byte


/***************
 ***  MDB    ***
 ***************/
// State trackers taken in reference to page 94 of 3.0 spec pdf
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
