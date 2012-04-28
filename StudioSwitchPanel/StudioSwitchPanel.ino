/****************************************************	
 * sketch = StudioSwitchPanel
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
 * Lighting Automation Switch
 *
 * 
 ****************************************************/
/*  
 History
     000 - Started 28/04/2012
     001 - Initial release


 Known issues:
 
 
 Future changes:
 
 
 ToDo:
 
 
 Authors:
 'RepRap' Matt	  dps.lwk at gmail.com
 
 */

#define VERSION_NUM 001
#define VERSION_STRING "StudioController ver: 001"

// Uncomment for debug prints
//#define DEBUG_PRINT

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "StudioSwitchPanel.h"


/**************************************************** 
 * statusUpdate
 * given MQTT request for STATUS respond as needed
 *
 ****************************************************/
void statusUpdate(byte* payload, int length) {
	// check for Status request
	if (strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)) == 0) {
#ifdef DEBUG_PRINT
		Serial.println("Status Request");
#endif
		mqttClient.publish(P_STATUS, RUNNING);
	}
}

/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
	if (!strcmp(S_STATUS, topic)) {
		statusUpdate(payload, length);
	}
}


/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *  
 ****************************************************/
void checkMQTT()
{
  	if(!mqttClient.connected()){
		if (mqttClient.connect(CLIENT_ID)) {
			mqttClient.publish(P_STATUS, RESTART);
			mqttClient.subscribe(S_RX);
			mqttClient.subscribe(S_STATUS);
#ifdef DEBUG_PRINT
			Serial.println("MQTT Reconnect");
#endif
		}
	}
} 

void setup() {
	// Start Serial
	Serial.begin(BAUD);
#ifdef DEBUG_PRINT
	Serial.println(VERSION_STRING);
#endif
	
	// Start ethernet on the OpenKontrol Gateway
	// handles static ip or DHCP automatically
	if(ip){
		Ethernet.begin(mac, ip);
	} else {
		if (Ethernet.begin(mac) == 0) {
#ifdef DEBUG_PRINT
			Serial.println("Failed to configure Ethernet using DHCP");
#endif
			// no point in carrying on, so do nothing forevermore:
			for(;;)
				;
		}
	}
    
#ifdef DEBUG_PRINT
	Serial.println("Ethernet Up");
#endif	
	// Setup Pins
    
	
	// Set default output's
	
	// Start MQTT and say we are alive
	checkMQTT();
	
	// let everything else settle
	delay(100);
}

void loop() {
	
	// Poll MQTT
	// should cause callback if theres a new message
	mqttClient.loop();
    
	// are we still connected to MQTT
	checkMQTT();
    
    
}


