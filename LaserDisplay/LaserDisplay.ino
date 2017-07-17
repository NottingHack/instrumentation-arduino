

/*   
 * Copyright (c) 2016, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.



 * Target controller = Arduino Due
 * Development platform = Arduino IDE 1.6.5$

 */


#include <climits> 
#include <ThermistorTemperature.h>
#include <DueTimer.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClientLD.h>
#include <ArduinoJson.h>

#include <MatrixText.h>
#include <System5x7.h>
#include "MsNowNext.h"
#include "MsAlert.h"

enum screen_t {SCREEN_ALERT, SCREEN_NOW_NEXT};

// Callback function header
void mqtt_callback(char* topic, byte* payload, unsigned int length);

byte _buf[2][16][24];


MsNowNext *nn;
MsAlert *alert;

volatile uint8_t current_buf = 0; 
volatile unsigned long _last_refresh_start;
volatile bool _net_access;


byte mac[]    = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte server[] = { 192, 168, 0, 1 };
byte ip[]     = { 192, 168, 0, 24 };

EthernetClient ethClient;
PubSubClientLD _client(server, 1883, mqtt_callback, ethClient);

char _json_message[300];
bool _got_json_message;
bool _connected;

unsigned long _alert_end;

MatrixScreen *_current_screen;
ThermistorTemperature *TT;

#define THERMISTOR_PIN  A0
#define W5100_RESET_PIN 14
#define S_STATUS        "nh/status/req"
#define P_STATUS        "nh/status/res"
#define S_BOOKINGS      "nh/bookings/laser/nownext"
#define P_BOOKINGS      "nh/bookings/poll"
#define P_TEMPERATURE   "nh/temp"
#define TEMP_FAKE_ID    "9999000000000001"
#define S_LOCAL_ALERT   "nh/tools/laser/ALERT"
#define S_GLOBAL_ALERT  "nh/ALERT"

#define DEV_NAME    "LaserDisplay"
#define STATUS_STRING "STATUS"

// Because millis() doesn't increase whilst interupt handlers are executing... and we
// spend a long time inside interupt handlers, we need to be able to compensate for this
// when delays of x seconds are required.
#define TIMING_MULTIPLIER 0.441f

void setup() 
{
  Serial.begin(115200);
  Serial.println("Init");
  _got_json_message = false;

  nn = new MsNowNext(set_xy, 192, 16);
  nn->init();

  alert = new MsAlert(set_xy, 192, 16);
  alert->init();

  TT = new ThermistorTemperature(THERMISTOR_PIN, 10000, 10000, 3435);

  _connected = false;

  
  // SPI init
  SPI.begin(4) ;
  SPI.setClockDivider(4, SPI_CLOCK_DIV32);
  SPI.setDataMode(4, SPI_MODE3);
  SPI.setBitOrder(4, MSBFIRST);
 
  Serial.println(F("Init..."));
  pinMode(W5100_RESET_PIN, OUTPUT);
  digitalWrite(W5100_RESET_PIN, LOW);
  delay(100);
  digitalWrite(W5100_RESET_PIN, HIGH);
  delay(1000); 

  Ethernet.begin(mac, ip);
  Serial.print(F("local IP:"));
  Serial.println(Ethernet.localIP());
  Serial.println(F("node started."));
  _client.loop();
  _client.loop();
  Serial.println(F("After client l")); 
  checkMQTT();
  Serial.println(F("After mqtt")); 
  _client.loop();

  current_buf = 1;

  SetScreen(SCREEN_NOW_NEXT);

 Timer3.attachInterrupt(DisplayRefresh).start(25000); // 25ms.
 Serial.println(F("Init done."));
 
 Serial.println(millis());
}

void SetScreen(int scn)
{
  memset(_buf, 0, sizeof(_buf));

  switch (scn)
  {
    case SCREEN_ALERT:
      _current_screen = alert;
      break;

    case SCREEN_NOW_NEXT:
      _current_screen = nn;
      break;

    default:
      Serial.print(F("Unknown screen: "));
      Serial.println(scn);
      break;
  }
}

void DisplayRefresh()
{
  // Don't attempt to refresh the display if the wiznet module is being accessed
  if (_net_access)
    return;

  _last_refresh_start = micros();

  // If we're not connected to mosquitto, don't refresh the display. As the SPI interface
  // is tied up for so long during each connection attempt, the display is basically unreadable.
  if (!_connected)
    return;

  /* Refresh the display. This takes ~10ms */
  for (int i = 0; i <  16; i++)
  {
    delayMicroseconds(120);
    SPI.transfer(4, encode_row(i), SPI_CONTINUE) ;
    delayMicroseconds(15);
  
    for (int j = 0; j < 24; j++) 
    {
      if (j < 23)
        SPI.transfer(4, _buf[current_buf][i][j], SPI_CONTINUE);
      else
        SPI.transfer(4, _buf[current_buf][i][j]);
      delayMicroseconds(15);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  char buf[256];

  Serial.println(topic);
  
  if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
      sprintf(buf, "Running: %s", DEV_NAME);
      _client.publish(P_STATUS, buf);
    }
  }

  else if (strcmp(topic, S_BOOKINGS) == 0)
  {
    int size = sizeof(_json_message)-1;
    if ((length+1) < size)
      size = (length+1);

    _got_json_message = true;
    strlcpy(_json_message, (char*)payload, size);
  }

  else if ((strcmp(topic, S_GLOBAL_ALERT) == 0) ||
           (strcmp(topic, S_LOCAL_ALERT ) == 0))
  {
    uint32_t alert_duration;
    // Message is expected to be in the format:
    //   "nnnnn:message"
    // Where nnnnn is the number of seconds to display the message for. If 0 then
    // show message until a cancel message is received.
    // A 0 length message is treated as a cancel.
    if (length == 0)
    {
      SetScreen(SCREEN_NOW_NEXT);
      return;
    }

    if (length < 6)
    {
      Serial.println(F("Invalid message"));
      return;
    }

    // Get alert duration
    strncpy(buf, (char*)payload, 5);
    alert_duration = atoi(buf);

    if (alert_duration == 0)
      _alert_end = ULONG_MAX;
    else
      _alert_end = millis() + ((float)(alert_duration * 1000) * TIMING_MULTIPLIER);

    if (length == 6) // no message specified, treat as cancel.
    {
      SetScreen(SCREEN_NOW_NEXT);
      return;
    }

    // Get message
    strlcpy(buf, (char*)payload+6, length-5);

    // Show alert
    alert->set_message(buf);
    SetScreen(SCREEN_ALERT);
  }
}


void loop()
{

  // Check for an alert to cancel
  if (_current_screen == alert)
    if ((_alert_end != ULONG_MAX) && (_alert_end != 0))
      if (millis() > _alert_end)
      {
        Serial.println(F("Canceling alert"));
        _alert_end = 0;
        SetScreen(SCREEN_NOW_NEXT);
      }

  if (_current_screen->loop())
  {
    current_buf = !current_buf;

    memcpy(_buf[!current_buf], _buf[current_buf], sizeof(_buf[current_buf]));
  }


  // Don't access the wiznet module unless a display refresh has recently
  // finshed (i.e. a refresh hopfully won't be due mid net check)
  // Both share the SPI bus, and the display will flicker if it's
  // not regualary / consistanty refreshed, so that takes priority.
  if 
    (
      (micros() - _last_refresh_start < 10000) ||
      (!_connected) // display isn't refreshed if not connected, so bypass check
    )
  {
    _net_access = true;
    _client.loop();
    checkMQTT();
    _net_access = false;
  }


  if (_got_json_message)
  {
    _got_json_message = false;
      Serial.println(_json_message);
    nn->process_message(_json_message);
  }

  temperature_loop();
}

void temperature_loop()
{
  static unsigned long last_temp_reading = 0;
  char buf[100]="";

  if (millis()-last_temp_reading > (60 * 1000))
  {
    Serial.println("Get temp");
    last_temp_reading = millis();
    float temp = TT->GetTemperature();
    sprintf(buf, "%s:%.2f", TEMP_FAKE_ID, temp);
    _client.publish(P_TEMPERATURE, buf);
  }
}

byte encode_row(byte row)
{
  byte enc_row=0;
  byte nibble1, nibble2;

  nibble1 = row;
  nibble2 = ~row & 0x0F;
  
  enc_row = nibble1;
  enc_row |=  ((nibble2 << 4) & 0xF0);  
  return enc_row;
} 


/**************************************************** 
 * check we are still connected to MQTT
 * reconnect if needed
 *
 ****************************************************/
void checkMQTT() 
{
  char *pToolTopic;
  static boolean first_connect = true;
  
  if (!_client.connected()) 
  {
    Serial.println("NOT connected");
    _connected = false;
    if (_client.connect(DEV_NAME)) 
    {
      Serial.println("Now connected");
      _connected = true;
      char buf[30];

      sprintf(buf, "Restart: %s", DEV_NAME);
      _client.publish(P_STATUS, buf);
      Serial.println("after pub");

      // Subscribe
      _client.subscribe(S_BOOKINGS);
      _client.subscribe(S_GLOBAL_ALERT);
      _client.subscribe(S_LOCAL_ALERT);
      _client.subscribe(S_STATUS); 
      Serial.println("after sub");

    }
  }
}

void set_xy (uint16_t x, uint16_t y, byte val)
{
  if (val)
    _buf[!current_buf][y][x/8] |= 1 << (7-(x%8));    // Set bit
  else
    _buf[!current_buf][y][x/8] &= ~(1 << (7-(x%8))); // Clear bit
} 
