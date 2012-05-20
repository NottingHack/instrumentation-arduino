/****************************************************	
 * sketch = StudioController
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/Lighting_Automation
 * Target controller = Arduino 328 
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 1.0
 * C compiler = WinAVR from Arduino IDE 1.0
 * 
 * 
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 * Lighting Automation Controler
 *
 * 
 ****************************************************/

/*
 These are the global config parameters
 Including Pin Outs, IP's, TimeOut's etc
 
 */

#ifndef  STUDIOCONTROLLER_h
#define  STUDIOCONTROLLER_h
#include <avr/pgmspace.h>

/**************************************************** 
 * genral defines
 * use for staic config bits
 ****************************************************/
// baud rate
#define BAUD 9600

// shift register pins
#define CLOCK_PIN 3
#define DATA_PIN 4
#define LATCH_PIN 2


/**************************************************** 
 * global vars
 * 
 ****************************************************/
int lightState;


/**************************************************** 
 * Ethernet settings
 * Update these with values suitable for your network.
 *
 ****************************************************/
EthernetClient ethClient;
byte mac[]    = { 0xDE, 0xED, 0xB4, 0xFE, 0xFE, 0x1E };
byte ip[]     = { 10, 0, 0, 65 }; 


/**************************************************** 
 * MQTT settings
 *
 ****************************************************/
byte mqttIp[] = {10,0,0,2};

#define MQTT_PORT 1883
// forward def of callback
void callbackMQTT(char*, byte*, unsigned int);
PubSubClient mqttClient(mqttIp, MQTT_PORT, callbackMQTT, ethClient);

// ClientId for connecting to MQTT
#define CLIENT_ID "StudioController"

// Subscribe to topics
#define S_RX        "nh/li/set/#"
#define S_RX_MASK   "nh/li/set/"
#define S_XRF       "nh/xrf/rx/#"
#define S_XRF_MASK  "nh/xrf/rx/"

// Publish Topics
#define P_TX    "nh/li/state/"
#define P_XRF  "nh/xrf/tx/"     // dont need this yet

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status"
#define P_STATUS "nh/status"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: StudioController"
#define RESTART "Restart: StudioController"

/**************************************************** 
 * Rooms Zones Commands Pins
 *
 ****************************************************/
char AL0[] PROGMEM = "all/0";
char ST0[] PROGMEM = "studio/0";
char ST1[] PROGMEM = "studio/1";
char ST2[] PROGMEM = "studio/2";
char ST3[] PROGMEM = "studio/3";
char CO0[] PROGMEM = "comfy/0";
char CO1[] PROGMEM = "comfy/1";
char CO2[] PROGMEM = "comfy/2";
char HA1[] PROGMEM = "hall/1";
char BO1[] PROGMEM = "box/1";
char SO1[] PROGMEM = "storgae/2";
char BL1[] PROGMEM = "blue/1";
char KI1[] PROGMEM = "kitchen/1";


char ON[] PROGMEM = "ON";
char OFF[] PROGMEM = "OFF";
char TOGGLE[] PROGMEM = "TOGGLE";

#define AL0_PINMASK 65535
#define ST0_PINMASK 7
#define CO0_PINMASK 24
#define ST1_PINMASK 1
#define ST2_PINMASK 2
#define ST3_PINMASK 4
#define CO1_PINMASK 8
#define CO2_PINMASK 16
#define HA1_PINMASK 32
#define BO1_PINMASK 64
#define SO1_PINMASK 128
#define BL1_PINMASK 256
#define KI1_PINMASK 512

/**************************************************** 
 * llap relateed
 * only two for the workshop controler 
 * L1 A
 + L2 A
 ****************************************************/
// mqtt topic mask to quicker filter the llap mesages
#define S_XRF_L_MASK "nh/xrf/rx/L"
#define DEVID_1 "L1"
#define DEVID_2 "L2"
#define DEVID_3 "L3"
#define DEVID_4 "L4"
#define BUTTONA "A"
#define BUTTONB "B"

#endif
