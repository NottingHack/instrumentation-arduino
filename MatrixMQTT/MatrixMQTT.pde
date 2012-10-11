
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
 History
    000 - Started 27/06/2011
 	001 - Initial release
	002 - changes to work with libary rewirte
	003 - changes to genric nh/status usage and added checkMQTT() 
	004 - adding temp dallas
	005 - adding twitter 
			LT1441M libray updated to support 6 message buffers
            TODO PubSubClient needs update to support more than 128 packet

			lines defined as
			0 irc nick
			1 irc msg
			2 twitter screen_name
			3 twitter text
			4 google from (TODO 006)
			5 google subject (TODO 006)
			
			twitter can get it direct from 
			S_TWITTER nh/twitter/rx/hs/<screen_name>
			
			should irc still clear display?
			
			setup running toggle between lines
			1>3>5>
			2>4>6>
			libary tweeked to sync changes
    006 - adding basic XRF gateway stuff    07/03/2012
            new topics 
            P_XRF   XRF>MQTT
            S_XRF   MQTT>XRF
            adding pushXRF() & pollXRF()
    007 - added door button handeling   11/10/2012
            now subscribes and publishes to 
            nh/gk/DoorButton            
			
 
 Known issues:
	All code is based on official Ethernet library not the nanode's ENC28J60, we need to port the MQTT PubSubClient
	PubSubClient by default only supports 128 bytes yet LT1441M buffer is 140 char's
 
 Future changes:

 
 ToDo:
    Change irc message display acknowledge 
	Add Last Will and Testament
	
	
 Authors:
 'RepRap' Matt      dps.lwk at gmail.com
  John Crouchley    johng at crouchley.me.uk
 
 */

#define VERSION_NUM 007
#define VERSION_STRING "MatrixMQTT ver: 007"

// Uncomment for debug prints
#define DEBUG_PRINT

// Uncomment for LT1441M debug prints
// needs to be done in LT1441M.h
//#define DEBUG_PRINT1


#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <LT1441M.h>			// Class for LT1441M
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"

// function prototypes
void irc_process(byte*, int);
void twitter_process(byte*, int);
void mail_process(char*, byte*, int);
void pushXRF(char*, byte*, int);
void callbackMQTT(char*, byte*, int);
void checkMQTT();
void poll();
void pollXRF();
uint8_t bufferNumber(unsigned long, uint8_t, uint8_t);
uint8_t bufferFloat(double, uint8_t, uint8_t);
void getTemps();
void findSensors();
void printAddress(DeviceAddress);
void setupToggle();
void doorButton();
void pollDoorBell();


// compile on holly need this befor callbackMQTT
LT1441M myMatrix(GSI, GAEO, LATCH, CLOCK, RAEO, RSI);
// compile on holly needs this after callbackMQTT
PubSubClient client(server, MQTT_PORT, callbackMQTT);
char pmsg[DMSG];
char LLAPmsg[LLAP_BUFFER_LENGTH];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature dallas(&oneWire);

// Number of temperature devices found
int numberOfDevices;
// array to store found address
DeviceAddress tempAddress[10]; 

/**************************************************** 
 * icr_process
 * given a payload set diaply lines 0 & 1
 * 
 ****************************************************/
void irc_process(byte* payload, int length) {
    if (strncmp(DISPLAY_STRING, (char*)payload, strlen(DISPLAY_STRING)) == 0) {
        // strip DISPLAY_STRING, send rest to Matrix
        //char msg[length - sizeof(DISPLAY_STRING) + 2];
        memset(pmsg, 0, DMSG);
        for(int i = 0; i < (length - strlen(DISPLAY_STRING)); i++) {
            pmsg[i] = (char)payload[i + sizeof(DISPLAY_STRING) -1];
        } // end for
        
        myMatrix.selectFont(1, DEFAULT_FONT);
        myMatrix.setLine(1, pmsg, 0, 1, 1);
        myMatrix.showLine(1);
        
        
        // return msg that has been displayed 
        client.publish(P_TX, pmsg);
#ifdef DEBUG_PRINT
        Serial.print("Displayed Message: ");
        Serial.print(pmsg);
        Serial.print(" sizeof:");
        Serial.println(strlen(pmsg), DEC);
#endif
        
    } else if (strncmp(NICK_STRING, (char*)payload, strlen(NICK_STRING)) == 0) {
        // strip DISPLAY_STRING, send rest to Matrix
        //char msg[length - sizeof(NICK_STRING) + 2];
        memset(pmsg, 0, DMSG);
        pmsg[0] = '<';
        for(int i = 0; i < (length - strlen(NICK_STRING)); i++) {
            pmsg[i + 1] = (char)payload[i + sizeof(NICK_STRING) -1];
        } // end for
        pmsg[length - strlen(NICK_STRING)+1] = '>';
        
        myMatrix.selectFont(0,DEFAULT_FONT);
        myMatrix.setLine(0, pmsg, 0, 0, 1);
        myMatrix.showLine(0);
        
        // return msg that has been displayed 
        client.publish(P_TX, pmsg);
#ifdef DEBUG_PRINT
        Serial.print("Displayed Nick: ");
        Serial.print(pmsg);
        Serial.print(" sizeof nick:");
        Serial.println(strlen(pmsg), DEC);
#endif
        
    } else if (strcmp(CLRSCREEN_STRING, (char*)payload) <= 0) {
        if (length > strlen(CLRSCREEN_STRING)) {
            // stript out which line to clear
            //myMatrix.clrLine(line);
        } else {
            myMatrix.hideLine(0);
            myMatrix.hideLine(1);
            // return mag to say screen cleard
            client.publish(P_TX, CLRSCREEN_DONE_STRING);
#ifdef DEBUG_PRINT
            Serial.println("Screen Cleared");
#endif
        }
        // } else if (strncmp(XXX, (char*)payload, sizeof(XXX)) == 0) {
    } else {
        // send the whole payload to 
        
    }// end if else

} // end void irc_process(byte* playload, int length)

/**************************************************** 
 * twitter_process
 * given a payload set diaply lines 2 & 3
 * strip <screen_name> from payload
 ****************************************************/
void twitter_process(byte* payload, int length) {
    // twitter stuff
    // strip <screen_name> from payload
    // find first :
    // strip <screen_name>, add@ and setLine(2...
    int slen;
    char delim[] = ":";
    slen = strcspn((char*)payload, delim);
    
    memset(pmsg, 0, DMSG);
    
    pmsg[0] = '@';
    /*
    for(int i = 0; i < (slen); i++) {
        pmsg[1+i] = (char)payload[i];
    } // end for
    */
    memcpy(pmsg+1, payload, slen);
    
    // screen_name to line 2
    myMatrix.selectFont(2, DEFAULT_FONT);
    myMatrix.setLine(2, pmsg, 0, 0, 1);
    myMatrix.showLine(2);
    
    // payload into pmsg
    memset(pmsg, 0, DMSG);
    
    if ((length - slen + 1) > DMSG-1) {
        length = DMSG-1;
    }
    
    memcpy(pmsg, payload + slen + 1, length - slen - 1);
    // rest to line 3
	myMatrix.selectFont(3, DEFAULT_FONT);
    myMatrix.setLine(3, pmsg, 0, 1, 1);
    myMatrix.showLine(3);
    
} // end void twitter_process(byte* payload, int length)

/**************************************************** 
 * mail_process
 * given a payload set diaply lines 4 & 5
 * strip <screen_name> from topic
 ****************************************************/
void mail_process(char* topic, byte* payload, int length) {

    memset(pmsg, 0, DMSG);
    
    // strip sender from topic & copy all in one
    memcpy(pmsg, topic + strlen(S_MAIL_MASK), strlen(topic) - strlen(S_MAIL_MASK));
    
    // sender to line 4
    myMatrix.selectFont(4, DEFAULT_FONT);
    myMatrix.setLine(4, pmsg, 0, 0, 1);
    myMatrix.showLine(4);
    
    // payload into pmsg
    memset(pmsg, 0, DMSG);
    
    if (length > DMSG-1)
        length = DMSG-1;
    
    memcpy(pmsg, payload, length);
    // sender to line 5
	myMatrix.selectFont(5, DEFAULT_FONT);
    myMatrix.setLine(5, pmsg, 0, 1, 1);
    myMatrix.showLine(5);
    
} // end void mail_process(char* topic, byte* payload, int length)

/**************************************************** 
 * pushXRF 
 * Read incoming LLAP from MQTT and push to XRF 
 ****************************************************/
void pushXRF(char* topic, byte* payload, int length) {
    // pre fill buffer with padding
    memset(LLAPmsg, '-', LLAP_BUFFER_LENGTH);
	// add a null terminator to make it a string
    LLAPmsg[LLAP_BUFFER_LENGTH - 1] = 0;
    // start char for LLAP packet
    LLAPmsg[0] = 'a';
    
	// copy <DEVID> from topic
    memcpy(LLAPmsg +1, topic + strlen(S_XRF_MASK), LLAP_DEVID_LENGTH);
    
    // little memory overflow protection
    if ((length) > LLAP_DATA_LENGTH) {
        length = LLAP_DATA_LENGTH;
    }
    // copy mqtt payload into messgae
    memcpy(LLAPmsg+3, payload, length);
    
    // send it out via the XRF
    Serial1.print(LLAPmsg);
} 

/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handel as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, int length) {

  // handle message arrived
	if (!strcmp(S_RX, topic)) {
        // call irc process
        irc_process(payload, length);
	} else  if (!strcmp(S_STATUS, topic)) {
		// check for Status request,
		if (strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)) == 0) {
#ifdef DEBUG_PRINT
			Serial.println("Status Request");
#endif
			client.publish(P_STATUS, RUNNING);
		} // end if
    } else if (!strcmp(S_TWITTER, topic) ) {
        // handel twitter status update
#ifdef DEBUG_PRINT
        Serial.println("Twitter Status");
#endif
        twitter_process(payload, length);
    } else if (strncmp(S_MAIL_MASK, topic, strlen(S_MAIL_MASK)) == 0) {
        // handel twitter status update
#ifdef DEBUG_PRINT
        Serial.println("Mail Status");
#endif
        mail_process(topic, payload, length);
	} else if (strncmp(S_XRF_MASK, topic, strlen(S_XRF_MASK)) == 0) {
#ifdef DEBUG_PRINT
        Serial.println("Push XRF");
#endif        
        pushXRF(topic, payload, length);
    } else if (!strcmp(S_DOOR_BUTTON, topic)) {
        // check for door state messages
        if (strncmp(DOOR_INNER, (char*)payload, strlen(DOOR_INNER)) == 0) {
			doorButtonState = DOOR_STATE_INNER;
        } else if (strncmp(DOOR_REAR, (char*)payload, strlen(DOOR_REAR)) == 0) {
			doorButtonState = DOOR_STATE_REAR;
		} // end if
    } // end if else
  
} // end void callback(char* topic, byte* payload,int length)

/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *  
 ****************************************************/
void checkMQTT() {
  	if(!client.connected()){
		if (client.connect(CLIENT_ID)) {
			client.publish(P_STATUS, RESTART);
			client.subscribe(S_RX);
			client.subscribe(S_STATUS);
            client.subscribe(S_TWITTER);
            client.subscribe(S_MAIL);
            client.subscribe(S_XRF);
            client.subscribe(S_DOOR_BUTTON);
#ifdef DEBUG_PRINT
			Serial.println("MQTT Reconect");
#endif
		} // end if
	} // end if
} // end checkMQTT()

/**************************************************** 
 * Poll 
 *  
 ****************************************************/
void poll()
{

} // end void poll()

/**************************************************** 
 * pollXRF 
 * Read incoming LLAP from XRF and push to MQTT 
 ****************************************************/
void pollXRF() {
    if (Serial1.available() >= 12){
		delay(5);
        if (Serial1.read() == 'a') {
			char devid[3];
			devid[2] = 0;
            // build full topic by read in <DEVID>
			char llapTopic[strlen(P_XRF) + LLAP_DEVID_LENGTH + 1];
			memset(llapTopic, 0, strlen(P_XRF) + LLAP_DEVID_LENGTH + 1);
			strcpy(llapTopic, P_XRF);
			devid[0] = Serial1.read();
			devid[1] = Serial1.read();
			Serial1.print('a');
			Serial1.print(devid);
			Serial1.print("ACK------");
			Serial1.flush();
			llapTopic[strlen(P_XRF)] = devid[0];
			llapTopic[strlen(P_XRF)+1] = devid[1];
			
            //clear the buffer
            memset(LLAPmsg, 0, LLAP_BUFFER_LENGTH);
            char t;
            // read in rest of message for mqtt payload
            for(int pos=0; pos < LLAP_DATA_LENGTH; pos++) {
            	t = Serial1.read();

            	if(t != '-')
                	LLAPmsg[pos] = t;
            }
        
            client.publish(llapTopic, LLAPmsg);
        }
    }
}

/**************************************************** 
 * bufferNumber
 * tweaked from print.cpp to buffer ascii in pmsg
 * and pushed stright out over mqtt
 ****************************************************/
uint8_t bufferNumber(unsigned long n, uint8_t base, uint8_t p)
{
	unsigned char buf[8 * sizeof(long)]; // Assumes 8-bit chars. 
	unsigned long i = 0;
	
	if (n == 0) {
		pmsg[p++] = '0';
		return p;
	} 
	
	while (n > 0) {
		buf[i++] = n % base;
		n /= base;
	}
	
	for (; i > 0; i--) 
    pmsg[p++] = (char) (buf[i - 1] < 10 ?
				  '0' + buf[i - 1] :
				  'A' + buf[i - 1] - 10);
	
	return p;
} // end void bufferNumber(unsigned long n, uint8_t base, uint8_t *p)

/**************************************************** 
 * bufferNumber
 * tweaked from print.cpp to buffer ascii in pmsg
 * and pushed stright out over mqtt
 ****************************************************/
uint8_t bufferFloat(double number, uint8_t digits, uint8_t p) 
{ 
	// Handle negative numbers
	if (number < 0.0)
	{
		pmsg[p++] = '-';
		number = -number;
	}
	
	// Round correctly so that print(1.999, 2) prints as "2.00"
	double rounding = 0.5;
	for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
	
	number += rounding;
	
	// Extract the integer part of the number and print it
	unsigned long int_part = (unsigned long)number;
	double remainder = number - (double)int_part;
	p = bufferNumber(int_part, 10, p);
	
	// Print the decimal point, but only if there are digits beyond
	if (digits > 0)
		pmsg[p++] = '.';
	
	// Extract digits from the remainder one at a time
	while (digits-- > 0)
	{
		remainder *= 10.0;
		int toPrint = int(remainder);
		p = bufferNumber(toPrint, 10, p);
		remainder -= toPrint; 
	} 
	return p;
} // end void bufferFloat(double number, uint8_t digits, uint8_t *p) 


/**************************************************** 
 * getTemps
 * grabs all the temps from the dallas devices
 * and pushed stright out over mqtt
 ****************************************************/
void getTemps()
{
	if ( (millis() - tempTimeout) > TEMPREATURE_TIMEOUT ) {
		tempTimeout = millis();
		dallas.requestTemperatures(); // Send the command to get temperatures
		
		for (int i=0; i < numberOfDevices; i++) {
			// build mqtt message in pmsg[] buffer, address:temp
			memset(pmsg, 0, DMSG);
			uint8_t p = 0;
			
			// address to pmsg first
			for(int j=0; j < 8; j++) {
				if (tempAddress[i][j] < 16)
					pmsg[p++] = '0';
				p = bufferNumber(tempAddress[i][j], 16, p);
			} 
			
			// seprator char
			pmsg[p++] = ':';
			
			// copy float to ascii char array
			p = bufferFloat(dallas.getTempC(tempAddress[i]), 2, p);
			
			// push each stright out via mqtt
			client.publish(P_TEMP_STATUS, pmsg);
#ifdef DEBUG_PRINT
			Serial.print("Temp sent: ");
			Serial.println(pmsg);
#endif
		}
	}
	
} // end void getTemps()

void findSensors() 
{
	// locate devices on the bus
#ifdef DEBUG_PRINT
	Serial.print("Locating devices...");
#endif
	// Grab a count of devices on the wire
	numberOfDevices = dallas.getDeviceCount();
	
#ifdef DEBUG_PRINT    
	Serial.print("Found ");
	Serial.print(numberOfDevices, DEC);
	Serial.println(" devices.");
#endif

	// Loop through each device, store address
	for(int i=0;i<numberOfDevices; i++)
	{
		// Search the wire for address
		if(dallas.getAddress(tempAddress[i], i))
		{
#ifdef DEBUG_PRINT
			Serial.print("Found device ");
			Serial.print(i, DEC);
			Serial.print(" with address: ");
			printAddress(tempAddress[i]);
			Serial.println();
			
			Serial.print("Setting resolution to ");
			Serial.println(TEMPERATURE_PRECISION, DEC);
#endif
			// set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
			dallas.setResolution(tempAddress[i], TEMPERATURE_PRECISION);
			
#ifdef DEBUG_PRINT
			Serial.print("Resolution actually set to: ");
			Serial.print(dallas.getResolution(tempAddress[i]), DEC); 
			Serial.println();
#endif
		}else{
#ifdef DEBUG_PRINT
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.print(" but could not detect address. Check power and cabling");
#endif
		}
	}
	
	
} // end void findSensors()

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++) {
#ifdef DEBUG_PRINT
	if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
#endif
	}
} // end void printAdress()



/**************************************************** 
 * setupToggle
 * setup toggle between the buffers
 * 
 ****************************************************/
void setupToggle() {
    myMatrix.toggleStart(0,2);
    myMatrix.toggleStart(1,3);
    // myMatrix.toggleStart(2,0);
    // myMatrix.toggleStart(3,1);
    // TODO 006
    //myMatrix.toggleStart(4,0);    
    //myMatrix.toggleStart(5,0); 

} // end void setupToggle()

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
		doorButtonState = DOOR_STATE_OUTER;
	} // end if
} // end void doorButton()

/**************************************************** 
 * Has the door bell button been pressed
 * state is set from interupt routine
 *
 ****************************************************/
void pollDoorBell() 
{
	if(doorButtonState == DOOR_STATE_INNER) {
	    // clear state 
		doorButtonState = DOOR_STATE_NONE;
/*
		digitalWrite(DOOR_BELL, HIGH);
		delay(DOOR_BELL_LENGTH);
		digitalWrite(DOOR_BELL, LOW);
*/
    } else if(doorButtonState == DOOR_STATE_OUTER) {
        // clear state
		doorButtonState = DOOR_STATE_NONE;
		client.publish(P_DOOR_BUTTON, DOOR_OUTER);
/*
        digitalWrite(DOOR_BELL, HIGH);
		delay(DOOR_BELL_LENGTH/2);
		digitalWrite(DOOR_BELL, LOW);
        delay(DOOR_BELL_LENGTH/2);
        digitalWrite(DOOR_BELL, HIGH);
		delay(DOOR_BELL_LENGTH/2);
		digitalWrite(DOOR_BELL, LOW);
*/
    } else if(doorButtonState == DOOR_STATE_REAR){
        // clear state
		doorButtonState = DOOR_STATE_NONE;
		client.publish(P_DOOR_BUTTON, DOOR_REAR);
/*
        digitalWrite(DOOR_BELL, HIGH);
		delay(DOOR_BELL_LENGTH/4);
		digitalWrite(DOOR_BELL, LOW);
        delay(DOOR_BELL_LENGTH/4);
        digitalWrite(DOOR_BELL, HIGH);
		delay(DOOR_BELL_LENGTH/4);
		digitalWrite(DOOR_BELL, LOW);
        delay(DOOR_BELL_LENGTH/4);
        digitalWrite(DOOR_BELL, HIGH);
		delay(DOOR_BELL_LENGTH/4);
		digitalWrite(DOOR_BELL, LOW);
*/
	} // end if
} // end void pollDoorBell()

void setup()
{
	// Start Serial
	Serial.begin(9600);
	Serial.println(VERSION_STRING);
	
	// Start ethernet
	Ethernet.begin(mac, ip);
  
	// Setup Pins
	pinMode(GROUND, OUTPUT);
    pinMode(XRF_POWER_PIN, OUTPUT);
	pinMode(DOOR_BUTTON, INPUT);

	// Set default output's
	digitalWrite(GROUND, LOW);
    // turn on XRF before start serial output
    digitalWrite(XRF_POWER_PIN, HIGH);
	
	// Start matrix and display version
	myMatrix.begin();  
	myMatrix.selectFont(0,DEFAULT_FONT); 
	myMatrix.setLine(0,VERSION_STRING,0,1); //default font is goin to cause issue here
    myMatrix.clrLine(1);
    myMatrix.clrLine(2);
    myMatrix.clrLine(3);
    myMatrix.clrLine(4);
    myMatrix.clrLine(5);
    
    
    myMatrix.cascadeSetup(0,2);
    myMatrix.cascadeSetup(2,4);
    myMatrix.cascadeSetup(4,0);
    
    myMatrix.cascadeSetup(1,3);
    myMatrix.cascadeSetup(3,5);
    myMatrix.cascadeSetup(5,1);
    
    //myMatrix.hideLine(1);
    myMatrix.hideLine(2);
    myMatrix.hideLine(3);
    myMatrix.hideLine(4);
    myMatrix.hideLine(5);
    
	myMatrix.loop();
	myMatrix.enable();
	
	// start the one wire bus and dallas stuff
	dallas.begin();
	findSensors();
	
    //Start XRF Serial
    Serial1.begin(XRF_BAUD);
    
    
	// Start MQTT and say we are alive
	if (client.connect(CLIENT_ID)) {
		client.publish(P_STATUS, RESTART);
		client.subscribe(S_RX);
		client.subscribe(S_STATUS);
        client.subscribe(S_TWITTER);
        client.subscribe(S_MAIL);
        client.subscribe(S_XRF);
	}
	// setupToggle();
    
	// let everything else settle
	delay(100);
	
	attachInterrupt(1, doorButton, HIGH);
	myMatrix.cascadeStart(0);
	myMatrix.cascadeStart(1);
	
} // end void setup()


void loop()
{
	
	// Poll 
	//poll();
    
    // poll XRF for incoming LLAP
	pollXRF();
    
	// Poll MQTT
	// should cause callback if theres a new message
	client.loop();
	
	// Scroll display, Push new frame
	myMatrix.loop();
	
	// Get Latest Temps
	getTemps();
	
	// Poll Door Bell
	// has the button been press
	pollDoorBell();
	
	// are we still connected to MQTT
	checkMQTT();
	
} // end void loop()


