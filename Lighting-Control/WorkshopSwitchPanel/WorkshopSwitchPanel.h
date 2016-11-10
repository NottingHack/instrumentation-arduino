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
 * Lighting Automation Swicth
 *
 * 
 ****************************************************/

/*
 These are the global config parameters
 Including Pin Outs, IP's, TimeOut's etc
 
 */

#ifndef  WORKSHOPSWITCHPANEL_h
#define  WORKSHOPSWITCHPANEL_h

/**************************************************** 
 * genral defines
 * use for staic config bits
 ****************************************************/
// baud rate
#define BAUD 9600

/**************************************************** 
 * global vars
 * 
 ****************************************************/


/**************************************************** 
 * Ethernet settings
 * Update these with values suitable for your network.
 *
 ****************************************************/
EthernetClient ethClient;
byte mac[]    = { 0xDE, 0xED, 0xB4, 0xFE, 0xFE, 0x4E };
byte ip[]     = { 10, 0, 0, 68 }; 


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
#define CLIENT_ID "WorkshopSwitchPanel"

// Subscribe to topics
#define S_RX		"nh/tx/#"
#define S_RX_MASK   "nh/tx/"

// Publish Topics
#define P_TX		"nh/rx/"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status"
#define P_STATUS "nh/status"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: WorkshopSwitchPanel"
#define RESTART "Restart: WorkshopSwitchPanel"

#endif