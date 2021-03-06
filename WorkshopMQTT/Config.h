/****************************************************	
 * sketch = WorkshopMQTT
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/...
 * Target controller = Arduino 328 (Nanode v5)
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 1.0.3
 * C compiler = WinAVR from Arduino IDE 1.0.3
 * 
 * 
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * 
 ****************************************************/

/*
 These are the global config parameters
 Including Pin Outs, IP's, TimeOut's etc
 
 Arduino Wiznet sheild uses pings 10, 11, 12, 13 for the ethernet 
 */

// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0x65, 0xEF }; // ***LWK*** should randomise this
// MartixMQTT's Reserved IP
byte ip[]     = { 192, 168, 0, 19 };

// Door Bell Relay
// Volt Free switching
// ***LWK*** use pin 10 for nanode v5 or pin 8 for wiznet, dont forget to move soldered line on the proto sheild (needs a jumper)
#define DOOR_BELL 8
#define DOOR_BELL_LENGTH 1000

#define LDR_PIN A5
#define LDR_TIMEOUT 150000

unsigned long doorTimeOut = 0;
boolean doorButtonState = 0;

// MQTT 

// MQTT server on holly
byte server[] = { 192, 168, 0, 1 };
#define MQTT_PORT 1883

// buffer size
#define DMSG	50

// ClientId for connecting to MQTT
#define CLIENT_ID "Workshop"

// Subscribe to topics
#define S_DOOR_BELL    "nh/gk/bell/Workshop"

// Publish Topics
#define P_LIGHT_LEVEL           "nh/lightlevel/workshop"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: WorkshopMQTT"
#define RESTART "Restart: WorkshopMQTT"

// temp sensor configs
#define ONE_WIRE_PWR	6
#define ONE_WIRE_BUS	5
#define ONE_WIRE_GND	4
#define TEMPERATURE_PRECISION 11
#define TEMPREATURE_TIMEOUT 150000	// one min (60000 * 1)
unsigned long tempTimeout = 0;
unsigned long lightTimeout = 0;

#define P_TEMP_STATUS	"nh/temp"

