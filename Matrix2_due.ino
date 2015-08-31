#include <climits> 

#include <DueTimer.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
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
PubSubClient _client(server, 1883, mqtt_callback, ethClient);

char _json_message[300];
bool _got_json_message;
bool _connected;

unsigned long _alert_end;

MatrixScreen *_current_screen;

#define W5100_RESET_PIN 14
#define S_STATUS       "nh/status/req"
#define P_STATUS       "nh/status/res"
#define S_BOOKINGS     "nh/tools/laser/BOOKINGS"
#define S_LOCAL_ALERT  "nh/tools/laser/ALERT"
#define S_GLOBAL_ALERT "nh/ALERT"

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
      
      // Also request the latest booking info
      _client.publish(S_BOOKINGS, "POLL");
      

      // Subscribe
      _client.subscribe(S_BOOKINGS);
      _client.subscribe(S_GLOBAL_ALERT);
      _client.subscribe(S_LOCAL_ALERT);
      _client.subscribe(S_STATUS); 
      Serial.println("after sub");

    }
  }
}

void set_xy (uint16_t x, byte y, byte val)
{
  if (val)
    _buf[!current_buf][y][x/8] |= 1 << (7-(x%8));    // Set bit
  else
    _buf[!current_buf][y][x/8] &= ~(1 << (7-(x%8))); // Clear bit
} 
