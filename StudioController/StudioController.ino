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

#include <avr/pgmspace.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "StudioController.h"

/**************************************************** 
 * publishState
 * sends out the current state of zones
 *
 ****************************************************/
void publishState(unsigned int zones) {
    char pTopic[30];    // [strlen(P_TX) + strlen(EL1) + 1]
    //memset(pTopic, 0, 30);
    strcpy(pTopic, P_TX);
    
    char pState[10]; // [strlen(TOGGLE)+1]
    //memset(pState, 0, 10);

    switch (zones) {
        case AL0_PINMASK:
            //need to send all states, so just call ourself multiple times ;)
            // note fall-though
            publishState(CO1_PINMASK);
            publishState(CO2_PINMASK);
            publishState(BO1_PINMASK);
            publishState(SO1_PINMASK);
            publishState(BL1_PINMASK);
            publishState(KI1_PINMASK);
        case ST0_PINMASK:
            //need to send 3 states, so just call ourself mutiple times ;)
            publishState(ST3_PINMASK);
            publishState(ST2_PINMASK);
        case ST1_PINMASK:        
            strcat_P(pTopic, ST1);
            if (lightState & ST1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case ST2_PINMASK:
            strcat_P(pTopic, ST2);
            if (lightState & ST2_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case ST3_PINMASK:
            strcat_P(pTopic, ST3);
            if (lightState & ST3_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case CO0_PINMASK:
            //need to send 2 states, so just call ourself multiple times ;)
            publishState(CO2_PINMASK);
        case CO1_PINMASK:
            strcat_P(pTopic, CO1);
            if (lightState & CO1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case CO2_PINMASK:
            strcat_P(pTopic, CO2);
            if (lightState & CO2_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case HA1_PINMASK:
            strcat_P(pTopic, HA1);
            if (lightState & HA1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case BO1_PINMASK:
            strcat_P(pTopic, BO1);
            if (lightState & BO1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case SO1_PINMASK:
            strcat_P(pTopic, SO1);
            if (lightState & SO1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case BL1_PINMASK:
            strcat_P(pTopic, BL1);
            if (lightState & BL1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        case KI1_PINMASK:
            strcat_P(pTopic, KI1);
            if (lightState & KI1_PINMASK) {
                strcpy_P(pState, ON);
            } else {
                strcpy_P(pState, OFF);
            }
            break;
        default:
            // notthing to do
            return;
            break;
    }

//#ifdef DEBUG_PRINT
    Serial.print(F("Light State Sent: "));
    Serial.flush();
    Serial.print(pTopic);
    Serial.print(F(" : "));
    Serial.println(pState);
    Serial.flush();
//#endif
    mqttClient.publish(pTopic, (byte*)pState, strlen(pState), 1);
}

/**************************************************** 
 * stateUpdate
 * shifting out lightState
 *
 ****************************************************/
void stateUpdate() {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, (lightState >> 8));
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, lightState);
    digitalWrite(LATCH_PIN, HIGH);
#ifdef DEBUG_PRINT
    Serial.println(F("Shift Register Update"));
#endif    
}

/**************************************************** 
 * matchZone
 * decodes ROOM/ZONE from topic and returns PINMASK
 *
 ****************************************************/
unsigned int matchZone(char* topic) {
    //strip down topic to make match easier?
    //just point a little futher along 
    topic += strlen(S_RX_MASK);
    
    // we cant not use a switch on strings so...
    if (strcmp_P(topic, AL0) == 0) {
        return AL0_PINMASK;
    } else if (strcmp_P(topic, ST0) == 0) {
        return ST0_PINMASK;
    } else if (strcmp_P(topic, ST1) == 0) {
        return ST1_PINMASK;
    } else if (strcmp_P(topic, ST2) == 0) {
        return ST2_PINMASK;
    } else if (strcmp_P(topic, ST3) == 0) {
        return ST3_PINMASK;
    } else if (strcmp_P(topic, CO0) == 0) {
        return CO0_PINMASK;
    } else if (strcmp_P(topic, CO1) == 0) {
        return CO1_PINMASK;
    } else if (strcmp_P(topic, CO2) == 0) {
        return CO2_PINMASK;
    } else if (strcmp_P(topic, HA1) == 0) {
        return HA1_PINMASK;
    } else if (strcmp_P(topic, BO1) == 0) {
        return BO1_PINMASK;
    } else if (strcmp_P(topic, SO1) == 0) {
        return SO1_PINMASK;
    } else if (strcmp_P(topic, BL1) == 0) {
        return BL1_PINMASK;
    } else if (strcmp_P(topic, KI1) == 0) {
        return KI1_PINMASK;
    } else {
        return 0;
    }
}

/**************************************************** 
 * decodeLLAP
 * decodes MQTT topic to get DEVID
 * payload will be zone
 * 
 ****************************************************/
void decodeLLAP(char* topic, byte* payload, int length) {
    //match topic to DEVID
    unsigned int zone;
    
    // if topic ends with L1 and payload A 
           if (strstr(topic, DEVID_2) != NULL && strncmp((char*)payload, BUTTONB, strlen(BUTTONB)) == 0) {
        zone = KI1_PINMASK;
    } else if (strstr(topic, DEVID_3) != NULL && strncmp((char*)payload, BUTTONA, strlen(BUTTONA)) == 0) {
        zone = BL1_PINMASK;
    } else if (strstr(topic, DEVID_4) != NULL && strncmp((char*)payload, BUTTONA, strlen(BUTTONA)) == 0) {
        zone = SO1_PINMASK;
    } else if (strstr(topic, DEVID_4) != NULL && strncmp((char*)payload, BUTTONB, strlen(BUTTONA)) == 0) {
        zone = BO1_PINMASK;
    } else {
        // no zone match so not for us
        return;
    }

    // LLAP always toggles
    lightState &= ~zone;
   
    stateUpdate();
    //publish new state for changed zones
    publishState(zone);
}

/**************************************************** 
 * setLight
 * decodes MQTT topic and COMMAND 
 * then sets the approprate lights
 * this does all the heavy lifting
 ****************************************************/
void setLight(char* topic, byte* payload, int length) {
    //match topic to zone
    unsigned int zone = matchZone(topic);
    
    if (zone == 0){
        // no zone match so not for us
        return;
    }
    //match command
    if (strncmp_P((char*)payload, TOGGLE, strlen_P(TOGGLE)) == 0){
        // toggle bits
        lightState ^= zone;
    } else if (strncmp_P((char*)payload, ON, strlen_P(ON)) == 0) {
        // set bits
        lightState |= zone;
    } else if (strncmp_P((char*)payload, OFF, strlen_P(OFF)) == 0) {
        // clear bits
        lightState &= ~zone;
    } else {
        //unknown command
        return;
    }
        
    stateUpdate();
    //publish new state for changed zones
    publishState(zone);
}

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
	} else if (strncmp(S_RX_MASK, topic, strlen(S_RX_MASK)) == 0) {
#ifdef DEBUG_PRINT
		Serial.println(F("Got set request"));
        Serial.flush();
#endif        
		setLight(topic, payload, length);
	} else if (strncmp(S_XRF_L_MASK, topic, strlen(S_XRF_L_MASK)) == 0) {
#ifdef DEBUG_PRINT
		Serial.println(F("Got lighting LLAP"));
        Serial.flush();
#endif        
		decodeLLAP(topic, payload, length);
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
			mqttClient.subscribe(S_XRF);
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
	
	// Start ethernet
    Ethernet.begin(mac, ip);
    
    
#ifdef DEBUG_PRINT
	Serial.println("Ethernet Up");
#endif	
    lightState = 0;
    // Setup Pins
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    
	// Set default output's
	stateUpdate();
    
	// Start MQTT and say we are alive
	checkMQTT();
	
	// let everything else settle
	delay(100);
    
    //rebootState for lights
    // should we publish them all?
}

void loop() {
	
	// Poll MQTT
	// should cause callback if theres a new message
	mqttClient.loop();
    
	// are we still connected to MQTT
	checkMQTT();
    
    
}


