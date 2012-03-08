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
  These are the global config parameters for Gatekeeper
  Including Pin Outs, IP's, TimeOut's etc
  
  Arduino Wiznet sheild uses pings 10, 11, 12, 13 for the ethernet
  Nanode v5 uses pins 8, 11, 12, 13 for the ethernet
  
*/


// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED }; // ***LWK*** should randomise this, or use nanode on chip mac, if we have pins :/ and code
// Gatekeeper's Reserved IP
byte ip[]     = { 10, 0, 0, 60 }; 

// Door Bell Relay
// Volt Free switching
// ***LWK*** use pin 10 for nanode v5 or pin 8 for wiznet, dont forget to move soldered line on the proto sheild (needs a jumper)
#define DOOR_BELL 8 
#define DOOR_BELL_LENGTH 250


// Door Bell Button
// HIGH = PUSHED
// Interrupt PIN
#define DOOR_BUTTON 3
// timeout in mills for how often the doorbell can be rang
#define DOOR_BUTTON_TIMEOUT 5000
unsigned long doorTimeOut = 0;
boolean doorButtonState = 0;

// Status Indicator's 
// BLUE = UNLOCKED
// RED = LOCKED
#define BLUE_LED 5
#define RED_LED 6

// Magnetic Door Contact
#define MAG_CON 9
#define CLOSED HIGH
#define OPEN LOW
boolean magConState = CLOSED;
#define MAG_CON_TIMEOUT 1000
unsigned long magTimeOut = 0;

// Magnetic Door Release
#define MAG_REL 4
#define UNLOCK HIGH
#define LOCK LOW
#define UNLOCK_STRING "Unlock:"
#define UNLOCK_DELIM ":"

// timeout in millis for the how long the magnetic release will stay unlocked
#define MAG_REL_TIMEOUT 5000

// RFID module Serial 9600N1
#define RFID_TX 0
#define RFID_RX 1
// query to read a serial number
byte query[8] = {
  0xAA, 0x00, 0x03, 0x25, 0x26, 0x00, 0x00, 0xBB};
unsigned char rfidReturn[255] = {};
unsigned long lastCardNumber;
// timeout in mills for how often the same card is read
#define CARD_TIMEOUT 3000
unsigned long cardTimeOut = 0;

// Keypad INT
#define KEYPAD A3
#define EEPROMRESET "EEReset"

// LCD
// timeout in mills for how long a msg is displayed
#define LCD_DEFAULT_0 "     Welcome to     "
#define LCD_DEFAULT_1 "Nottingham Hackspace"
#define LCD_TIMEOUT 5000
unsigned long lcdTimeOut = 0;
byte lcdState = 0;
#define DEFAULT 0
#define CUSTOM 1

//Speaker
#define SPEAKER A2

//Last Man Out
#define LAST_MAN A1
#define IN HIGH
#define OUT LOW
boolean lastManState = OUT;
#define LAST_MAN_TIMEOUT 30000
unsigned long lastManTimeOut = 0;

// MQTT 

// MQTT server on holly
byte server[] = { 10, 0, 0, 2 };
#define MQTT_PORT 1883

// ClientId for connecting to MQTT
#define CLIENT_ID "Gatekeeper"

// Subscribe to topics
#define S_UNLOCK		"nh/gk/Unlock"

// Publish Topics

#define P_DOOR_STATE		"nh/gk/DoorState"
#define P_KEYPAD			"nh/gk/Keypad"
#define P_DOOR_BUTTON		"nh/gk/DoorButton"
#define P_RFID				"nh/gk/RFID"
#define P_LAST_MAN_STATE	"nh/gk/LastManState"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status"
#define P_STATUS "nh/status"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: Gatekeeper"
#define RESTART "Restart: Gatekeeper"












