
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
 History
	000 - Started 28/05/2011
	001 - Initial release 
 
 Known issues:
	DEBUG_PRTINT and USB Serial Will NOT WORK if the RFID module is plugged in as this uses the hardware serial
	All code is based on official Ethernet library not the nanode's ENC28J60, we need to port the MQTT PubSubClient
	
 
 Future changes:
	Find a better way than 3 sec delay to get keys in buffer
		Make keypad interactive with LCD and use ent/clr to build pin numbers? 
		Requires rewrite of 001 lcd default handling
	Better Handling of the Backdoor
		Allow change via mqtt, store in eeprom
 
 ToDo:
	Add Last Will and Testament
	Add msg unlock(); 
		Something like unlock(char* who); so we can publish with the door open/timeout?
	
	
 Authors:
	'RepRap' Matt      dps.lwk at gmail.com
	John Crouchley    johng at crouchley.me.uk
 
 */

#define VERSION_NUM 001
#define VERSION_STRING "Gatekeeper Ver: 001"

// Uncomment for debug prints
// This will not work if the RFID module IS plugged in!!
//#define DEBUG_PRINT

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Config.h"
#include "Backdoor.h"

void callbackMQTT(char* topic, byte* payload, int length) {

	// handle message arrived
	if (!strcmp(S_UNLOCK, topic)) {
		// check for unlock, rest of payload is msg for lcd
		if (strcmp(UNLOCK_STRING, (char*)payload) < 0) {
			// strip UNLOCK_STRING, send rest to LCD
			char msg[length - sizeof(UNLOCK_STRING) + 2];
			memset(msg, 0, sizeof(msg));
			for(int i = 0; i < (sizeof(msg) - 1); i++) {
				msg[i] = (char)payload[i + sizeof(UNLOCK_STRING) -1];
			} // end for
			
			updateLCD(msg);
			// unlock the door
			unlock();
		} else if (strncmp(EEPROMRESET, (char*)payload, sizeof(EEPROMRESET)) == 0) {
			// updateLCD("EEPROM");
                        // resetEEPROM();
			// remapKeyp();
			// modeResetLCD();
		} else {
			char msg[length + 1];
			memset(msg, 0, sizeof(msg));
			for(int i = 0; i < (sizeof(msg)-1); i++) {
				msg[i] = (char)payload[i];
			} // end for
			
			// send the whole payload to LCD
			updateLCD(msg);
			
		}// end if else
	} // end if
	
} // end void callback(char* topic, byte* payload,int length)

PubSubClient client(server, MQTT_PORT, callbackMQTT);


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
	pinMode(LAST_MAN, INPUT);
	pinMode(SPEAKER, OUTPUT);
	
	// Set default output's
	digitalWrite(DOOR_BELL, LOW);
	digitalWrite(SPEAKER, HIGH);
	lockDoor();
	
	// Start I2C and display system detail
	Wire.begin();
	clearLCD();
	updateLCD(VERSION_STRING);
	
	// Start RFID Serial
	Serial.begin(9600);
        
        // delay to make sure ethernet is awake
	delay(100);
	// Start MQTT and say we are alive
	if (client.connect(CLIENT_ID)) {
		client.publish(P_STATUS,"Gatekeeper Restart");
		client.subscribe(S_UNLOCK);
	} // end if
	
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
	// has the door been opened likely some one left
	pollMagCon();
	
	// Poll MQTT
	// should cause callback if theres a new message
	client.loop();
	
	// Poll Keypad
	// do we have a pin number
	pollKeypad();
	
	// Poll Last Man Out
	// anyone in the building
	pollLastMan();
	
	// Poll LCD
	// updates to default msg after time out
	updateLCD(NULL);
	
	// Poll Door Bell
	// has the button been press
	pollDoorBell();

	// are we still connected to MQTT
	checkMQTT();
	
} // end void loop()


/**************************************************** 
 * Poll RFID
 *  
 ****************************************************/
void pollRFID()
{
	// quickly send query to read cards serial
	for (int i=0; i < 8; i++){
		Serial.print(query[i]) ;
	} // end for
	
	// get the serial number
	read_response(rfidReturn);
	// check status flag is clear and that we have a serial 
	if (rfidReturn[3] == 0 && rfidReturn[2] >= 6) {
	    unsigned long time = millis();
		// garb first digit of serial
		unsigned long cardNumber = rfidReturn[5];
		// loop for the rest
		for (int j=1; j < (rfidReturn[2] - 2); j++) {
			cardNumber = (cardNumber << 8) | rfidReturn[j+5];
		} // end for
		
		// check we are not reading the same card again or if we are its been a sensible time since last read it
		if (cardNumber != lastCardNumber || (millis() - cardTimeOut) > CARD_TIMEOUT) {
			lastCardNumber = cardNumber;
			cardTimeOut = millis();
			// convert cardNumber to a string array
			char cardBuf[20];
			ultoa(cardNumber, cardBuf, 10);
			client.publish(P_RFID, cardBuf);
			digitalWrite(SPEAKER, LOW);
			delay(50);
			digitalWrite(SPEAKER, HIGH);
		} // end if
		
		// theres a card but we cant read it
	} else if (rfidReturn[3] == 1 && rfidReturn[4] != 131 && (millis() - cardTimeOut) > CARD_TIMEOUT) {
		cardTimeOut = millis();
		client.publish(P_RFID, "Unknown Card Type");
	} // end else if 
} // end void pollRFID()


/**************************************************** 
 * Poll Magnetic Contact
 *  
 ****************************************************/
void pollMagCon()
{
	boolean state = digitalRead(MAG_CON);
	
	// check see if the magnetic contact has changed
	// timeout should kill bounce
	if(magConState != state && (millis() - magTimeOut) > MAG_CON_TIMEOUT) {
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
		magTimeOut = millis();
		
	} // end if
	
	
} // end void pollMagCon()


/**************************************************** 
 * Poll Keypad
 *  
 ****************************************************/
void pollKeypad()
{
	if(digitalRead(KEYPAD)) {
		// HACK to get keys in buffer, ToDo find a better way than 3 sec delay
		delay(3000);
		// find out how many keys were pressed
		byte n = readNumKeyp();
		char pin[n+1];
		//zero the array to make sure we get the NULL terminator
		memset(pin, 0, sizeof(pin));
		char c;
		for(byte i = 0; i < n; i++){
			// filter ent and clr keys as not used rite now
			c = readKeyp();
			if ( c != 0x2A && c != 0x23 )
				pin[i] = c;
			
			//pin[i] =readKeyp();
		} // end for
		
		//add null terminator <-- this didn't work when tested
		//pin[n+1] = 0;
		
		//check backdoor, note strcmp() returns 0 if true
		if(!strcmp(pin, BACKDOOR)){
			// just unlock the door
			unlock();	
			
		} else {
			//publish key pad output to MQTT
			client.publish(P_KEYPAD, pin);
			
		}// end else
	} // end if
} // end void pollKeypad()


/**************************************************** 
 * Poll Keypad
 *  
 ****************************************************/
void pollLastMan()
{
	boolean state = digitalRead(LAST_MAN);
	
	// check see if the magnetic contact has changed
	if(lastManState != state) {
		// yes it has so publish to MQTT
		switch (state) {
			case OUT:
			client.publish(P_LAST_MAN_STATE, "Last Out");
			break;
			case IN:
			client.publish(P_LAST_MAN_STATE, "First In");
			break;
			default:
			break;
		} // end switch
		
		// Update State
		lastManState = state;
		
	} // end if
} // end void pollLastMan()

/**************************************************** 
 * Has the door bell button been pressed
 * state is set from interupt routine
 *
 ****************************************************/
void pollDoorBell() 
{
	if(doorButtonState) {
	    // clear state 
		doorButtonState = 0;
		digitalWrite(DOOR_BELL, HIGH);
		client.publish(P_DOOR_BUTTON, "BING");
		delay(DOOR_BELL_LENGTH);
		digitalWrite(DOOR_BELL, LOW);
	} // end if
} // end void pollDoorBell()

/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *  
 ****************************************************/
void checkMQTT()
{
  	if(!client.connected()){
		if (client.connect(CLIENT_ID)) {
			client.publish(P_STATUS,"Gatekeeper Restart");
			client.subscribe(S_UNLOCK);
		} // end if
	} // end if
} // end checkMQTT()

/**************************************************** 
 * Interrupt method for door bell button
 * time out stop button being pushhed to often
 * arduino timers dont run inside interrupt's so 
 * millis() will return same value and delay() wont work
 *  
 ****************************************************/
void doorButton()
{
	if((millis() - doorTimeOut) > DOOR_BUTTON_TIMEOUT) {
	    // reset time out
	    doorTimeOut = millis();
		doorButtonState = 1;
	} // end if
} // end void doorButton()


/**************************************************** 
 * update the LCD output
 *  
 ****************************************************/
void updateLCD(char* msg)
{
	if (msg != NULL) {
	    lcdTimeOut = millis();
		lcdState = CUSTOM;
		clearLCD();
		sendStr(msg);
	} else if ((millis() - lcdTimeOut) > LCD_TIMEOUT && lcdState != DEFAULT) {
	    lcdTimeOut = millis();
		lcdState = DEFAULT;
		clearLCD();
		setCursorLCD(0,0);
		sendStr(LCD_DEFAULT_0);
		setCursorLCD(0,1);
		sendStr(LCD_DEFAULT_1);
	} // end else if
} // end void 

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
	
	// keep it open until we timeout or someone open the door
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
		client.publish(P_DOOR_STATE, "Door Opened by:");
		magConState = OPEN;
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




