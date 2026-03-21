#include <Ethernet.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <avr/wdt.h>
#include "Config.h"
#include "EnergyMonitoring.h"

SoftwareSerial serial485(PIN_485_RO, PIN_485_DI);
ModbusMaster modbus;
EthernetClient eth;
PubSubClient *mqtt;

void setup() {
  wdt_disable();

  pinMode(PIN_485_DI, OUTPUT);
  pinMode(PIN_485_DE, OUTPUT);
  pinMode(PIN_485_RO, INPUT);
  pinMode(PIN_ETH_CS, OUTPUT);

  digitalWrite(PIN_485_DE, LOW);

  Serial.begin(9600);

  for (int i = 0; i < 6; i++)
    _mac[i] = EEPROM.read(EEPROM_MAC + i);

  for (int i = 0; i < 4; i++)
    _ip[i] = EEPROM.read(EEPROM_IP + i);

  for (int i = 0; i < 4; i++)
    _server[i] = EEPROM.read(EEPROM_SERVER_IP + i);

  for (int i = 0; i < 40; i++)
    _base_topic[i] = EEPROM.read(EEPROM_BASE_TOPIC + i);
  _base_topic[40] = '\0';

  for (int i = 0; i < 20; i++)
    _dev_name[i] = EEPROM.read(EEPROM_NAME + i);
  _dev_name[20] = '\0';

  dbg_println("Init Ethernet");
  Ethernet.init(PIN_ETH_CS);
  Ethernet.begin(_mac, _ip);

  delay(200);
  dbg_println("Check MQTT");
  mqtt = new PubSubClient(_server, MQTT_PORT, callbackMQTT, eth);
  checkMQTT();

  dbg_println("Setting up Modbus");
  serial485.begin(SDM_BAUD);

  modbus.begin(SDM_DEVICE_ID, serial485);
  modbus.preTransmission(preTX);
  modbus.postTransmission(postTX);

  dbg_println("Setup done!");

  wdt_enable(WDTO_8S);
}

void loop() {
  wdt_reset();

  checkMQTT();
  mqtt->loop();

  // Frustratingly something in the ModbusMaster library prevents the
  // Watchdog from noticing if it's gotten stuck.
  readNextRegister();
  delay(15000 / MAP_SIZE); // We want a new reading for each prometheus pull
}

void callbackMQTT(char *topic, byte *payload, unsigned int length) {
  char buf[30];

  if (!strcmp(topic, S_STATUS)) {
    if (!strncmp(STATUS_STRING, (char *)payload, strlen(STATUS_STRING))) {
      dbg_println("Status Request");
      sprintf(buf, "Running: %s", _dev_name);
      mqtt->publish(P_STATUS, buf);
    }
  }
}

bool checkMQTT() {
  if (!mqtt->connected()) {
    if (mqtt->connect(_dev_name)) {
      char buf[30];

      dbg_println("Connected to MQTT");
      mqtt->subscribe(S_STATUS);
      sprintf(buf, "Restart: %s", _dev_name);
      mqtt->publish(P_STATUS, buf);
    } else {
      delay(250);
    }
  }

  return true;
}

void preTX() {
  digitalWrite(PIN_485_DE, HIGH);
}

void postTX() {
  digitalWrite(PIN_485_DE, LOW);
}

void readNextRegister() {
  modbus.clearResponseBuffer();

  uint8_t result = modbus.readInputRegisters(mappings[currentRegister].address, 2);
  if (result != modbus.ku8MBSuccess) {
    incrRegister();
    return;
  }

  uint32_t high = (uint32_t)modbus.getResponseBuffer(0);
  uint32_t low = (uint32_t)modbus.getResponseBuffer(1);

  union {
    uint32_t i;
    float f;
  } value;
  value.i = (high << 16) | low;

  char topic[64];
  if (mappings[currentRegister].subtopicB == NULL) {
    sprintf(topic, "%s/%s/%s", _base_topic, _dev_name, mappings[currentRegister].subtopicA);
  } else {
    sprintf(topic, "%s/%s/%s/%s", _base_topic, _dev_name, mappings[currentRegister].subtopicA, mappings[currentRegister].subtopicB);
  }

  char svalue[10] = {0};
  dtostrf(value.f, 2, 2, &svalue[strlen(svalue)]);
  mqtt->publish(topic, svalue);

  incrRegister();
}

void incrRegister() {
  currentRegister++;
  if (currentRegister > (MAP_SIZE-1))
    currentRegister = 0;
}

void dbg_println(const char *msg) {
#ifdef DEBUG_SERIAL
  Serial.println(msg);
#endif
#ifdef DEBUG_MQTT
  mqtt->publish("debug", msg);
#endif
}
