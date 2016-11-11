/****************************************************	
 * sketch = Gatekeeper
 *
 * Nottingham Hackspace
 * CC-BY-SA
 *
 * Source = http://wiki.nottinghack.org.uk/wiki/Gatekeeper
 * Target controller = Arduino 328 with Wiznet shield
 * Clock speed = 16 MHz
 * Development platform = Arduino IDE 1.0.1
 * C compiler = WinAVR from Arduino IDE 1.0.1
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
        002 - Minor Updates 08/03/2012
        003 - Move to Arduino 1.x + add LCD wordwrap
 
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

#define VERSION_NUM 003
#define VERSION_STRING "Gatekeeper Ver: 004"

// Uncomment for debug prints
// This will not work if the RFID module IS plugged in!!
//#define DEBUG_PRINT

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "Config.h"
#include "Backdoor.h"
#include "RDM880.h"

typedef RDM880::Uid rfid_uid;
rfid_uid lastCardNumber = {0};

void callbackMQTT(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, MQTT_PORT, callbackMQTT, ethClient);
RDM880 _rdm_reader(&Serial);

void callbackMQTT(char* topic, byte* payload, unsigned int length) {

	// handle message arrived
	if (!strcmp(S_UNLOCK, topic)) {
		// check for unlock, rest of payload is msg for lcd
		if (strncmp(UNLOCK_STRING, (char*)payload, strlen(UNLOCK_STRING)) == 0) {
			// strip UNLOCK_STRING, send rest to LCD
			char msg[length - sizeof(UNLOCK_STRING) + 2];
			memset(msg, 0, sizeof(msg));
			for(int i = 0; i < (sizeof(msg) - 1); i++) {
				msg[i] = (char)payload[i + sizeof(UNLOCK_STRING) -1];
			} // end for
			
			updateLCD(msg);
			// unlock the door
			unlock();
		} else if (strncmp(EEPROMRESET, (char*)payload, strlen(EEPROMRESET)) == 0) {
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
	} else  if (!strcmp(S_STATUS, topic)) {
	  // check for Status request,
		if (strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)) == 0) {
			client.publish(P_STATUS, RUNNING);
		} // end if
	} else if (!strcmp(S_DOOR_BELL, topic)) 
        {
          // Request to ring door bell - either outer or rear, but not inner, as that button is connected to this arduino
          if (!strncmp(DOOR_OUTER, (char*)payload, strlen(DOOR_OUTER)))        // Outer door bell
            doorButtonState = DOOR_STATE_OUTER;
          else if (!strncmp(DOOR_REAR, (char*)payload, strlen(DOOR_REAR)))     //  Rear door bell
            doorButtonState = DOOR_STATE_REAR;
        }
          
	
} // end void callback(char* topic, byte* payload,int length)


void setup()
{
    // Enable watchdog timer
    wdt_enable(WDTO_8S);
        
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
	checkMQTT();
    /*
    if (client.connect(CLIENT_ID)) {
		client.publish(P_STATUS,RESTART);
		client.subscribe(S_UNLOCK);
		client.subscribe(S_STATUS);
	} // end if
	*/
	// Setup Interrupt
	// let everything else settle
	delay(100);
	attachInterrupt(1, doorButton, RISING);
	
} // end void setup()

void loop()
{
    // Reset watchdog
    wdt_reset();
        
    // are we still connected to MQTT
	checkMQTT();
    
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

	
} // end void loop()


/**************************************************** 
 * Poll RFID
 *  
 ****************************************************/
void pollRFID()
{
    if (_rdm_reader.mfGetSerial()) {
        // check we are not reading the same card again or if we are its been a sensible time since last read it
        if (!eq_rfid_uid(cardNumber, lastCardNumber) || (millis() - cardTimeOut) > CARD_TIMEOUT) {
            cpy_rfid_uid(lastCardNumber, cardNumber);
            cardTimeOut = millis();
            // convert cardNumber to a string and send out
            char cardBuf[21];
            uid_to_hex(cardBuf, cardNumber);
            client.publish(P_RFID, cardBuf);

            // beep to say we read a card
            digitalWrite(SPEAKER, LOW);
            delay(50);
            digitalWrite(SPEAKER, HIGH);
        } // end if
    }

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
 * Poll LastMan
 *  
 ****************************************************/
void pollLastMan()
{
	boolean state = digitalRead(LAST_MAN);
	
	// check see if the magnetic contact has changed
	if(lastManState != state) {
        // reset debounce timmmer
        lastManTimeOut = millis();

		// Update State
		lastManState = state;
        lastManStateSent = false;
		
	} // end if
    
    if (lastManStateSent == false && lastManState != lastManStateOld && (millis() - lastManTimeOut) > LAST_MAN_TIMEOUT) {
        lastManStateSent = true;
        lastManStateOld = lastManState;
        // stable for at least LAST_MAN_TIMEOUT so publish to MQTT
		switch (lastManStateOld) {
			case OUT:
                client.publish(P_LAST_MAN_STATE, "Last Out");
                break;
			case IN:
                client.publish(P_LAST_MAN_STATE, "First In");
                break;
			default:
                break;
		} // end switch
    }
} // end void pollLastMan()

/**************************************************** 
 * Has either the door bell button been pressed
 * (state is set from interupt routine), or an 
 * MQTT message received to ring the bell
 *
 ****************************************************/
void pollDoorBell()
{
  if(doorButtonState == DOOR_STATE_INNER)
  {
    // clear state
    doorButtonState = DOOR_STATE_NONE;
    
    delay(10);
    if (digitalRead(DOOR_BUTTON) == HIGH)
    {
      // Inner button is connected to this arduino, so publish ring of inner bell
      client.publish(P_DOOR_BUTTON, DOOR_INNER);

      digitalWrite(DOOR_BELL, HIGH);
      delay(DOOR_BELL_LENGTH);
      digitalWrite(DOOR_BELL, LOW);
    }

  } else if(doorButtonState == DOOR_STATE_OUTER)
  {
    // clear state
    doorButtonState = DOOR_STATE_NONE;

    digitalWrite(DOOR_BELL, HIGH);
    delay(DOOR_BELL_LENGTH/2);
    digitalWrite(DOOR_BELL, LOW);
    delay(DOOR_BELL_LENGTH/2);
    digitalWrite(DOOR_BELL, HIGH);
    delay(DOOR_BELL_LENGTH/2);
    digitalWrite(DOOR_BELL, LOW);
  } else if(doorButtonState == DOOR_STATE_REAR)
  {
    // clear state
    doorButtonState = DOOR_STATE_NONE;

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
      client.publish(P_STATUS, RESTART);
      client.subscribe(S_UNLOCK);
      client.subscribe(S_STATUS);
      client.subscribe(S_DOOR_BELL);
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
          doorButtonState = DOOR_STATE_INNER;
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
    print_wrapped(msg);
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
 * Output line to LCD, and increment lineno.  
 * If last line of LCD written, returns TRUE  
 ****************************************************/
boolean output_line(int *lineno, char *str, int typ)
{
  setCursorLCD(0, (*lineno)++);
  sendStr(str);
  
  if (*lineno >= LCD_Y)
    return true;
  else
    return false;
}

/**************************************************** 
 * Word wrap *msg and output to LCD
 *  
 ****************************************************/
void print_wrapped(char *msg)
{
  int word_len, space_remain;
  int line_no = 0;
  char msg_line[LCD_X+1];
  char *mptr = msg;
 
  space_remain = LCD_X;
  msg_line[0] = '\0';
  while (*mptr!='\0')
  {
    word_len = 0;
    while (*mptr!='\0' && *mptr++!=' ')
      word_len++;
    
    // Special case - the word is longer than the display width
    if (word_len > LCD_X)
    {
      // output current line
      if (msg_line[0] != '\0')
        if(output_line(&line_no, msg_line, 1)) return;
      
      // Start new line
      msg_line[0] = '\0'; 
      space_remain = LCD_X;  
      
      while (word_len > 0)
      {
        memset(msg_line, 0, sizeof(msg_line));
        strncpy(msg_line, (mptr-word_len), (word_len >= LCD_X ? LCD_X : word_len));
        msg_line[LCD_X] = '\0';
        if (word_len > LCD_X)
          if(output_line(&line_no, msg_line, 2)) return;
          
        //word_len -= LCD_X;
        word_len -= (word_len >= LCD_X ? LCD_X : word_len);
      }
      strcat(msg_line, " ");

      space_remain = LCD_X - strlen(msg_line);
      
    }
    else if (word_len <= space_remain) // space left on current line -> add to line
    {
      if (*mptr)
      {
        strncat(msg_line, (mptr-word_len-1), word_len);
        if (word_len < space_remain) 
         strcat(msg_line, " ");
        space_remain -= word_len+1;
      }
      else
      {
        strncat(msg_line, (mptr-word_len), word_len);
        space_remain -= word_len;
      }
    } 
    else // Not enough space left - output current line, and add word to new line
    {
      // output
      if(output_line(&line_no, msg_line, 3)) return;
      
      // Start new line
      msg_line[0] = '\0';
      space_remain = LCD_X;
      
      strncat(msg_line, (mptr-word_len - ((*(mptr-word_len-1) == ' ') ? 0 : 1) ), word_len);
      
      strcat(msg_line, " ");
    }
  }
  
  if (msg_line[0] != '\0')
    if (output_line(&line_no, msg_line, 4)) return;

}

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


/**************************************************** 
 * RFID Helpers
 *  
 ****************************************************/

void cpy_rfid_uid(rfid_uid *dst, rfid_uid *src)
{
  memcpy(dst, src, sizeof(rfid_uid));
}

bool eq_rfid_uid(rfid_uid u1, rfid_uid u2)
{
  if (memcmp(&u1, &u2, sizeof(rfid_uid)))
    return false;
  else
    return true;
}

void uid_to_hex(char *uidstr, rfid_uid uid)
{
  switch (uid.size)
  {
    case 4:
      sprintf(uidstr, "%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3]);
      break;

    case 7:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6]);
      break;

    case 10:
      sprintf(uidstr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", uid.uidByte[0], uid.uidByte[1], uid.uidByte[2], uid.uidByte[3], uid.uidByte[4], uid.uidByte[5], uid.uidByte[6], uid.uidByte[7], uid.uidByte[8], uid.uidByte[9]);
      break;

    default:
      sprintf(uidstr, "ERR:%d", uid.size);
      break;
  }
}



