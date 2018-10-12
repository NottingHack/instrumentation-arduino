/****************************************************	
 * sketch = DoorBell
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/...
 * Target controller = Arduino 328
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 1.0.5
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
 
 Arduino Wiznet shield uses pins 10, 11, 12, 13 for the ethernet 
 */

// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0x62, 0x12 };
byte ip[]     = { 192, 168, 0, 32 };

// Door Bell mosfet
#define DOOR_BELL 9
#define DOOR_BELL_VOLUME 180  // (0-255)
#define DOOR_BELL_LENGTH 600


unsigned long doorTimeOut = 0;
boolean doorButtonState = 0;


// MQTT

// MQTT server on holly
byte server[] = { 192, 168, 0, 1 };
#define MQTT_PORT 1883

// buffer size
#define DMSG  50

// ClientId for connecting to MQTT
#define CLIENT_ID "DoorBell-G5"

// Subscribe to topics
#define S_DOOR_BELL    "nh/gk/bell/G5"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"
#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: DoorBellG5"
#define RESTART "Restart: DoorBellG5"

// temp sensor configs
#define ONE_WIRE_BUS  5
#define TEMPERATURE_PRECISION 11
#define TEMPREATURE_TIMEOUT 150000
unsigned long tempTimeout = 0;

#define P_TEMP_STATUS  "nh/temp"

