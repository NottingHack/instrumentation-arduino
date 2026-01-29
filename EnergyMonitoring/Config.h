#ifndef CONFIG_H
#define CONFIG_H

// Pin assigments
#define PIN_485_DI       8  // Driver Input
#define PIN_485_DE       9  // Driver Enable, Receive Disable
#define PIN_485_RO      10  // Receiver Output
#define PIN_ETH_CS       2  // WizNet Chip Select

#define SDM_DEVICE_ID    1  // SDM Modbus Address
#define SDM_BAUD      9600  // Modbus Baud Rate

// Locations in EEPROM of various settings
#define EEPROM_MAC           0 //  6 bytes
#define EEPROM_IP            6 //  4 bytes
#define EEPROM_BASE_TOPIC   10 // 40 bytes   e.g. "nh/energy/distribution"
#define EEPROM_NAME         50 // 20 bytes   e.g. "energy-F6"
#define EEPROM_SERVER_IP    70 //  4 bytes

#define MQTT_PORT 1883

// Status Topic, use to say we are alive or DEAD (will)
#define S_STATUS "nh/status/req"

#define P_STATUS "nh/status/res"
#define STATUS_STRING "STATUS"

// Debug output destinations
//#define DEBUG_SERIAL
//#define DEBUG_MQTT

#endif
