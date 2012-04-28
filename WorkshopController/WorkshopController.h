/****************************************************	
 * sketch = WorkshopController
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

#ifndef  WORKSHOPCONTROLLER_h
#define  WORKSHOPCONTROLLER_h

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
byte mac[]    = { 0xDE, 0xED, 0xB4, 0xFE, 0xFE, 0x2E };
byte ip[]     = { 10, 0, 0, 66 }; 


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
#define CLIENT_ID "WorkshopController"

// Subscribe to topics
#define S_RX		"nh/tx/#"
#define S_RX_MASK   "nh/tx/"

// Publish Topics
#define P_TX		"nh/rx/"

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status"
#define P_STATUS "nh/status"
#define STATUS_STRING "STATUS"
#define RUNNING "Running: WorkshopController"
#define RESTART "Restart: WorkshopController"

#endif