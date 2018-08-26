
#include "Config.h"
#include "net_transport.h"

// Network config populated on startup by serial_menu_init()
uint8_t _serverip[4];
byte    _mac[6];
uint8_t _myip[4];
char    _base_topic[NET_BASE_TOPIC_SIZE];
char    _dev_name [NET_DEV_NAME_SIZE];
char    _vend_topic[NET_BASE_TOPIC_SIZE + NET_DEV_NAME_SIZE + 6]; // "<base topic><device name>/tx/#" or "<base topic><device name>/rx"

EthernetClient _ethClient;
PubSubClient *_client;
char _mqttPayload [48];
struct net_msg_t net_msg;

extern unsigned long last_net_msg;
extern char rfid_serial[21];
extern char tran_id[10];
extern cState card_state;
extern byte allowVend;
extern LiquidCrystal_I2C lcd;


void net_transport_init()
{
  Ethernet.begin(_mac, _myip);
  _client = new PubSubClient(_serverip, MQTT_PORT, net_transport_mqtt_callback, _ethClient);

  // checkMQTT
  if (!_client->connected())
  {
    if (_client->connect(_dev_name))
    {
      sprintf(_mqttPayload, "Restart: %s", _dev_name);
      _client->publish(P_STATUS, _mqttPayload);
      _client->subscribe(S_STATUS);

      sprintf(_vend_topic, "%s%s/tx/#", _base_topic, _dev_name);
      _client->subscribe(_vend_topic);

      sprintf(_vend_topic, "%s%s/rx", _base_topic, _dev_name);
    }
  }
}

void net_transport_loop()
{
  _client->loop();
}

// Process incoming messages from mqtt
void net_transport_mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  // handle message that arrived
  if (!strncmp(S_STATUS, topic, sizeof(S_STATUS)-1))
  {
    // check for Status requests
    if (strncmp(STATUS_STRING, (char*)payload, strlen(STATUS_STRING)) == 0)
    {
      sprintf(_mqttPayload, "Running: %s", _dev_name);
      _client->publish(P_STATUS, _mqttPayload);
    }
  }

  else
  {
    // "normal" message
    if (length > 5)
    {
      unsigned int copy_len;

      memset(&net_msg, 0, sizeof(net_msg));
      memcpy(net_msg.msgtype, payload, 4);

      copy_len = length - 5;
      if (copy_len >= sizeof(net_msg.payload))
        copy_len = sizeof(net_msg.payload)-1;
      memcpy(net_msg.payload, payload + 5, copy_len);

      last_net_msg = millis();

      net_rx_message(&net_msg, rfid_serial, sizeof(rfid_serial), tran_id, sizeof(tran_id), &card_state, &allowVend, &lcd);
    }
  }

  payload[0]='\0';
}


void net_transport_send(struct net_msg_t *msg)
{
  memset(_mqttPayload, 0, sizeof(_mqttPayload));
  memcpy(_mqttPayload, msg->msgtype, 4);
  _mqttPayload[4] = ':';
  memcpy(_mqttPayload+5, msg->payload, sizeof(msg->payload));
  _client->publish(_vend_topic, _mqttPayload);

   dbg_println(_mqttPayload);
}


