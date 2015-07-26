#include <DueTimer.h>
#include <MatrixText.h>
#include <System5x7.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Callback function header
void mqtt_callback(char* topic, byte* payload, unsigned int length);

char json_test[] = "{ \"now\": { \"display_time\": \"now\", \"display_name\": \"none\" }, \"next\": { \"display_time\": \"09:00\", \"display_name\": \"James Fowkes\" } }";

byte _buf[2][16][24];

volatile uint8_t current_buf = 0; 
volatile unsigned long _last_refresh_start;
volatile bool _do_net;

MatrixText *mt1, *mt2; // MatrixText string
byte mac[]    = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte server[] = { 10, 0, 0, 1 };
byte ip[]     = { 10, 0, 0, 100 };

EthernetClient ethClient;
PubSubClient _client(server, 1883, mqtt_callback, ethClient);

char _json_message[300];
bool _got_json_message;
StaticJsonBuffer<500> _json_buffer;
char _display_time_now[8];
char _display_name_now[50];
char _display_time_next[8];
char _display_name_next[50];


#define W5100_RESET_PIN 21
#define S_STATUS    "nh/status/req"
#define P_STATUS    "nh/status/res"
#define S_BOOKINGS  "nh/tools/laser/BOOKINGS"

#define DEV_NAME    "LaserDisplay"
#define STATUS_STRING "STATUS"

void setup() 
{
  Serial.begin(115200);
  Serial.println("Init");
  _got_json_message = false;
  
  mqtt_callback(S_BOOKINGS, (byte*)json_test, sizeof(json_test));


  
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
  

  
  // start with a blank buffer
  memset(_buf, 0, sizeof(_buf));
  
  // Init matrix text
  mt1 = new MatrixText(set_xy);
  mt1->show_text("TEST", 0, 0, 192, 8);
  mt1->set_scroll_speed(0); 
  
  mt2 = new MatrixText(set_xy);
  // mt2->show_text("-=QWERTYUIOPASDFGHJKL=-", 0, 8, 192, 16);
  mt2->show_text("123456789012345678901234567890", 0, 8, 192, 16, false);
  mt2->set_scroll_speed(20); 


  current_buf = 1; /* mt1-loop() will draw to buffer[0], i.e. not the current one */
  mt1->loop();
  memcpy(_buf[1], _buf[0], sizeof(_buf[1]));

 Timer3.attachInterrupt(DisplayRefresh).start(25000); // 25ms
  Serial.println(F("Init done.")); 
}

void DisplayRefresh()
{
  _last_refresh_start = micros();
  
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
  _do_net = true;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  char buf[30];

  if (!strcmp(topic, S_STATUS))
  {
    if (!strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)))
    {
//      dbg_println(F("Status Request"));
      sprintf(buf, "Running: %s", DEV_NAME);
      _client.publish(P_STATUS, buf);
    }
  }

  if (strcmp(topic, S_BOOKINGS) == 0)
  {
    int size = sizeof(_json_message)-1;
    if ((length+1) < size)
      size = (length+1);
    
    _got_json_message = true;
    strlcpy(_json_message, (char*)payload, size);
  }


}


void loop() 
{
  /*
  mt1->loop();
  mt2->loop();
  current_buf = !current_buf;
*/
  
  uint8_t buf_updated = false;
  //_client.loop();
  buf_updated = mt1->loop();
 // _client.loop();
  buf_updated |= mt2->loop();
  
  if (buf_updated)
  {
    current_buf = !current_buf;
    
    memcpy(_buf[!current_buf], _buf[current_buf], sizeof(_buf[current_buf]));
  }  
  
  if (micros() - _last_refresh_start < 10000)
  //if (_do_net)
  {
   // Serial.println(".");
    _client.loop();
    checkMQTT();
    _do_net = false;
  } //else
   // Serial.println("!");
   
  if ((_got_json_message) && 0)
  {
    _got_json_message = false;
    
    JsonObject& root = _json_buffer.parseObject(_json_message);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return;
    }
    
    const char* display_time_now  = root["now" ]["display_time"];
    const char* display_name_now  = root["now" ]["display_name"];
    const char* display_time_next = root["next"]["display_time"];
    const char* display_name_next = root["next"]["display_name"];
    
    strlcpy(_display_name_next, display_name_next, sizeof(_display_name_next));
    strlcpy(_display_name_now , display_name_now , sizeof(_display_name_now ));
    
    strlcpy(_display_time_next, display_time_next, sizeof(_display_time_next));
    strlcpy(_display_time_now , display_time_now , sizeof(_display_time_now ));
    
    Serial.println(_display_time_now);
    Serial.println(_display_name_now);
    Serial.println(_display_time_next);
    Serial.println(_display_name_next); 
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
    if (_client.connect(DEV_NAME)) 
    {
      Serial.println("Now connected");
      char buf[30];
      
      sprintf(buf, "Restart: %s", DEV_NAME);
      _client.publish(P_STATUS, buf);
      Serial.println("after pub");

      // Subscribe
      _client.subscribe(S_BOOKINGS);

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
