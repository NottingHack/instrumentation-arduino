byte _ip[4];
byte _mac[7];
int _port = 1883;
byte _host[4];
char _topic[40];
char _name[20];

#define EEPROM_MAC           0 //  6 bytes
#define EEPROM_IP            6 //  4 bytes
#define EEPROM_BASE_TOPIC   10 // 40 bytes   e.g. "nh/air"
#define EEPROM_NAME         50 // 20 bytes   e.g. "Comfy"
#define EEPROM_SERVER_IP    70 //  4 bytes

#define ETH_CS 4
#define LED_PWR A0
#define LED_MQTT A1
#define PM_SLEEP 2
#define PM_RESET 3
#define PM_RX 0
#define PM_TX 1

#define PUBLISH_INTERVAL 15000
