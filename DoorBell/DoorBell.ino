
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
 History
    000 - Started 06/10/2018, based on WorkshopMQTT. Static/hardcoded config for
          now, maybe serial menu based config if another is needed.
 
 */

// Uncomment for debug prints
#define DEBUG_PRINT

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"

// function prototypes
void callbackMQTT(char*, byte*, unsigned int);
void checkMQTT();
uint8_t bufferNumber(unsigned long, uint8_t, uint8_t);
uint8_t bufferFloat(double, uint8_t, uint8_t);
void getTemps();
void findSensors();
void printAddress(DeviceAddress);
void pollDoorBell();

EthernetClient ethClient;
PubSubClient client(server, MQTT_PORT, callbackMQTT, ethClient);

char pmsg[DMSG];
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature dallas(&oneWire);

// Number of temperature devices found
int numberOfDevices;

// array to store found address
DeviceAddress tempAddress[10]; 


/**************************************************** 
 * callbackMQTT
 * called when we get a new MQTT
 * work out which topic was published to and handle as needed
 ****************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length) 
{  
  // handle message arrived
  if (!strncmp(S_STATUS, topic, sizeof(S_STATUS)-1))
  {
    // check for Status request,
    if (strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)) == 0) 
    {
#ifdef DEBUG_PRINT
      Serial.println(F("Status Request"));
#endif
      client.publish(P_STATUS, RUNNING);
    } // end if
  } else if (!strncmp(S_DOOR_BELL, topic, sizeof(S_DOOR_BELL)-1)) 
  {
    char paybuf[2];
    paybuf[1] = '\0';
    
    if (length >= 1)
      paybuf[0] = payload[0];
    else
      paybuf[1] = '1';
    
    doorButtonState = atoi(paybuf);
  }
  
  payload[0]='\0';
} // end void callback(char* topic, byte* payload,int length)
 
/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *  
 ****************************************************/
void checkMQTT() {
    if(!client.connected()) {
    if (client.connect(CLIENT_ID)) {
      client.publish(P_STATUS, RESTART);
      client.subscribe(S_DOOR_BELL);
      client.subscribe(S_STATUS);
#ifdef DEBUG_PRINT
      Serial.println(F("MQTT Reconnect"));
#endif
    } // end if
  } // end if
} // end checkMQTT()


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
      Serial.print(F("Temp sent: "));
      Serial.println(pmsg);
#endif
    }
  }
  
} // end void getTemps()


void findSensors() 
{
  // locate devices on the bus
#ifdef DEBUG_PRINT
  Serial.print(F("Locating devices..."));
#endif
  // Grab a count of devices on the wire
  numberOfDevices = dallas.getDeviceCount();
  
#ifdef DEBUG_PRINT    
  Serial.print(F("Found "));
  Serial.print(numberOfDevices, DEC);
  Serial.println(F(" devices."));
#endif

  // Loop through each device, store address
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(dallas.getAddress(tempAddress[i], i))
    {
#ifdef DEBUG_PRINT
      Serial.print(F("Found device "));
      Serial.print(i, DEC);
      Serial.print(F(" with address: "));
      printAddress(tempAddress[i]);
      Serial.println();
      
      Serial.print(F("Setting resolution to "));
      Serial.println(TEMPERATURE_PRECISION, DEC);
#endif
      // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      dallas.setResolution(tempAddress[i], TEMPERATURE_PRECISION);
      
#ifdef DEBUG_PRINT
      Serial.print(F("Resolution actually set to: "));
      Serial.print(dallas.getResolution(tempAddress[i]), DEC); 
      Serial.println();
#endif
    }else{
#ifdef DEBUG_PRINT
      Serial.print(F("Found ghost device at "));
      Serial.print(i, DEC);
      Serial.print(F(" but could not detect address. Check power and cabling"));
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
} // end void printAddress()

void ringBell(uint8_t rings)
{
  if (rings == 0)
    rings = 1;
  if (rings > 5)
    rings = 5;

  Serial.print("Ring bell - ");
  Serial.println(rings);

  for (uint8_t i = 0; i < rings; i++)
  {
    if (i)
      delay(DOOR_BELL_LENGTH/rings);
    analogWrite(DOOR_BELL, DOOR_BELL_VOLUME);
    delay(DOOR_BELL_LENGTH/rings);
    analogWrite(DOOR_BELL, 0);
  }
}

/**************************************************** 
 * Has the door bell button been pressed
 * state is set from interupt routine
 *
 ****************************************************/
void pollDoorBell() 
{
  if (doorButtonState != 0) 
  {
    ringBell(doorButtonState);
    doorButtonState = 0;
  }
} // end void pollDoorBell()

void setup()
{
  // Start Serial
  Serial.begin(9600);
  Serial.print("FW ver: ");
  Serial.println(BUILD_IDENT);

  // Start ethernet
  Ethernet.begin(mac, ip);

  // Setup Pins
  pinMode(DOOR_BELL, OUTPUT);
  digitalWrite(DOOR_BELL, LOW);
  
  // start the one wire bus and dallas stuff
  dallas.begin();
  findSensors();

  // Start MQTT and say we are alive
  checkMQTT();

  // let everything else settle
  delay(100);
  Serial.println("Setup done");

} // end void setup()


void loop()
{
  // Poll MQTT
  // should cause callback if theres a new message
  client.loop();

  // Get Latest Temps
  getTemps();

  // Poll Door Bell
  // has the button been press
  pollDoorBell();

  // are we still connected to MQTT
  checkMQTT();

} // end void loop()


