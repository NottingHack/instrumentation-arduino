
/****************************************************	
 * sketch = Gatekeeper
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/Gatekeeper
 * Target controller = Arduino 328 
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 002.
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
 History
 	001 - Initial release 
 
 Known issues:
 DEBUG_PRTINT and USB Serial Will NOT WORK if the RFID module is plugged in as this uses the hardware serial
 
 Future changes:
 	None
 
 ToDo:
 	Lots
	Add Last Will and Testament
	Better Handling of the Backdoor
	
 Authors:
 'RepRap' Matt      dps.lwk at gmail.com
  John Crouchley    johng at crouchley.me.uk
 
 */

#define VERSION_NUM 001

// Uncomment for debug prints
// This will not work if the RFID module IS plugged in!!
//#define DEBUG_PRINT

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Config.h"
#include "Backdoor.h"

void callback(char* topic, byte* payload,int length) {
  // handle message arrived\
  //TODO
} // end void callback(char* topic, byte* payload,int length)

PubSubClient client(server, MQTT_PORT, callback);


boolean magConState = CLOSED;
unsigned long doorTimeOut = 0;

void setup()
{
	// Start ethernet
	Ethernet.begin(mac, ip);
	
	// Setup Pins
	pinMode(DOOR_BELL, OUTPUT);
	pinMode(DOOR_BUTTON, INPUT);
	pinMode(BLUE_LED, OUTPUT);
	pinMode(RED_LED, OUTPUT);
	pinMode(MAG_CON, INPUT);
	pinMode(MAG_REL, OUTPUT);
	pinMode(KEYPAD, INPUT);
	
	
	// Set default output's
	digitalWrite(DOOR_BELL, LOW);
	lockDoor();
	
	
	// Start I2C and display system detail
	Wire.begin();
	clearLCD();
	sendStr("Gatekeeper Version: VERSION_NUM");
	
	// Start RFID Serial
	Serial.begin(9600);
	
	// Start MQTT and say we are alive
	if (client.connect(CLIENT_ID)) {
		client.publish(P_STATUS,"Gatekeeper Restart");
		client.subscribe(S_UNLOCK);
	}
	
	// Setup Interrupt
	// let everything else settle
	delay(100);
	attachInterrupt(1, doorButton, HIGH);
  
} // end void setup()

void loop()
{
	
	// Poll RFID
	pollRFID();
	
	// Poll Magnetic Contact
	pollMagCon();
	
	// Poll MQTT
	// should cause callback if theres a new message
	client.loop();
	
	// Poll Keypad
	pollKeypad();
	
} // end void loop()


/**************************************************** 
 * Poll RFID
 *  
 ****************************************************/
void pollRFID()
{
	// quickly send query to read cards serial
	for (int i=0 ; i<8 ; i++){
		Serial.print(query[i]) ;
	} // end for
	
	//TODO
	
} // end void pollRFID()


/**************************************************** 
 * Poll Magnetic Contact
 *  
 ****************************************************/
void pollMagCon()
{
	boolean state = digitalRead(MAG_CON);
	
	// check see if the magnetic contact has changed
	if(magConState != state) {
		// yes it has so publish to MQTT
		switch (state) {
			case CLOSED:
			client.publish(P_DOOR_STATE, "Door Closed");
			break;
			case OPEN:
			client.publish(P_DOOR_STATE, "Door Opened");
			break;
			default:
			break;
		} // end switch
		
		// Update State
		magConState = state;
		
	} // end if
	
	
} // end void pollMagCon()


/**************************************************** 
 * Poll Keypad
 *  
 ****************************************************/
void pollKeypad()
{
	if(digitalRead(KEYPAD)) {
		// find out how many keys were pressed
		byte n = readNumKeyp();
		char pin[n+1];
		
		for(byte i = 0; i < n; i++){
			pin[i] =readKeyp();
		} // end for
		
		//add null terminator
		pin[n+1] = 0;
		
		//compair to backdoor
		if(pin == BACKDOOR){
			// just unlock the door
			unlock();	
					
		} else {
			//publish key pad output to MQTT
			client.publish(P_KEYPAD, pin);
			
		}// end else
	} // end if
} // end void pollKeypad()


/**************************************************** 
 * Interrupt method for door bell button
 *  
 ****************************************************/
void doorButton()
{
	if((millis() - doorTimeOut) > DOOR_BUTTON_TIMEOUT) {
		client.publish(P_DOOR_BUTTON, "BING");
	} // end if
} // end void doorButton()


/**************************************************** 
 * Main Door unlock routine
 *  
 ****************************************************/
void unlock()
{
	boolean o = 0;

	// store current time
	unsigned long timeOut = millis();
	
	// unlock the door
	unlockDoor();
	
	// keep it open untill we timeout or someone open the door
	do {
		// check to see if someone opened the door
		if(digitalRead(MAG_CON) == OPEN) {
			o = 1;
			break;
		} // end if
		
	}while((millis() - timeOut) < MAG_REL_TIMEOUT);
	
	// lock the door 
	lockDoor();
	
	// Publish to MQTT if we timed out or the door was opened
	if(o == 1) {
		// the door was opened
		client.publish(P_DOOR_STATE, "Door Opened");
		
	} else {
		// the door timed out
		client.publish(P_DOOR_STATE, "Door Time Out");
		
	} // end else

} // end void unlock()


/**************************************************** 
 * Lock the door and update LED's
 *  
 ****************************************************/
void lockDoor()
{
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(MAG_REL, LOW);
} // end void lockDoor()

/**************************************************** 
 * Unlock the door and update LED's
 *  
 ****************************************************/
void unlockDoor()
{
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(MAG_REL, HIGH);
} // end void unlockDoor()




