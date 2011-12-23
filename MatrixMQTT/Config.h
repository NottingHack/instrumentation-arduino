/****************************************************	
 * sketch = MatrixMQTT
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
 * Large LED Matrix Display Board at 
 * Nottingham Hackspace and is part of the 
 * Hackspace Instrumentation Project
 *
 * 
 ****************************************************/


/*
 These are the global config parameters
 Including Pin Outs, IP's, TimeOut's etc
 
 Arduino Wiznet sheild uses pings 10, 11, 12, 13 for the ethernet
 Nanode v5 uses pins 8, 11, 12, 13 for the ethernet
 
 */


// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEE }; // ***LWK*** should randomise this, or use nanode on chip mac, if we have pins :/ and code
// MartixMQTT's Reserved IP
byte ip[]     = { 10, 0, 0, 61 }; 

/*
 * The LT1441M has a 7pin connector for the dasiy chained date lines 
 * 
 * GSI       Serial data yellow-green
 * /GAEO     Output enable for yellow-green
 * LATCH     Latch contents of shift register
 * GND1      Ground of IC
 * CLOCK     Clock Signal for data (read on L->H)
 * /RAEO     Output enable for red
 * RSI       Serial data red
 *
 * Power is via CN1
 * V1        Power for LED(red)
 * GND1      Ground for IC
 * VDD       Power for IC
 * GND2      Ground for LED
 * V2        Power for LED(yellow-green
 */

/* pin numbers for connection LT1441M */
/*
 #define GSI 8 
 #define GAEO 9 
 #define LATCH 10
 #define CLOCK 11
 #define RAEO 12
 #define RSI 13 
 */
/* pin map for mega with netowrk sheild */
#define GSI 34
#define GAEO 32
#define LATCH 30
#define CLOCK 28
#define RAEO 26
#define RSI 24
#define GROUND 22

#define DISPLAY_STRING "D:"
#define CLRSCREEN_STRING "CLEAR:"
#define CLRSCREEN_DONE_STRING "CLEARED"
#define NICK_STRING "N:"



// MQTT 

// MQTT server on holly
byte server[] = { 10, 0, 0, 2 };
#define MQTT_PORT 1883

// ClientId for connecting to MQTT
#define CLIENT_ID "Matrix"

// Subscribe to topics
#define S_RX		"nh/mb/tx"

// Publish Topics

#define P_TX		"nh/mb/rx"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status"
#define P_STATUS "nh/status"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: MatrixMQTT"
#define RESTART "Restart: MatrixMQTT"



// temp sensor configs
#define ONE_WIRE_PWR	44
#define ONE_WIRE_BUS	42
#define ONE_WIRE_GND	40
#define TEMPERATURE_PRECISION 9
#define TEMPREATURE_TIMEOUT 150000	// one min (60000 * 1)
unsigned long tempTimeout = 0;

#define P_TEMP_STATUS	"nh/temp"

// comfy 10 C3 28 29 02 08 00 21