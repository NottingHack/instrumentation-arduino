#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <avr/wdt.h>
#include "Config.h"
#include "AirQuality.h"
#include <SoftwareSerial.h>

#include <SparkFun_SCD4x_Arduino_Library.h>
#include <Adafruit_SGP40.h>
#include <PMS.h>
#include <INA219.h>
#include <Wire.h>

EthernetClient _ethClient;
PubSubClient *_client;
SCD4x scd4x;
Adafruit_SGP40 sgp;

SoftwareSerial pmsSerial(PM_RX, PM_TX);
PMS pms(pmsSerial);
PMS::DATA pmsValues;

INA219 INA(0x40);

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "nh/status/req") == 0 && strncmp((char*)payload, "STATUS", length) == 0) {
    char status[60];
    sprintf(status, "Running: AirQuality%s", _name);
    _client->publish((char*)"nh/status/res", status);
  }
}

void checkMQTT() {
  if (!_client->connected()) {
    digitalWrite(LED_MQTT, LOW);

    char name[40];
    sprintf(name, "AirQuality%s", _name);
    _client->connect(name);
    _client->subscribe((char*)"nh/status/req");
  } else {
    digitalWrite(LED_MQTT, HIGH);
  }
}

void publishValue(const char *property, char *value) {
  char topic[60];
  sprintf(topic, "%s/%s/%s", _topic, _name, property);
  _client->publish(topic, value);
}

void setup() {
  wdt_enable(WDTO_8S);

  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_MQTT, OUTPUT);
  pinMode(PM_SLEEP, OUTPUT);
  pinMode(PM_RESET, OUTPUT);
  pinMode(PM_TX, OUTPUT);
  pinMode(PM_RX, INPUT);

  digitalWrite(PM_SLEEP, HIGH);

  digitalWrite(PM_RESET, LOW);
  delay(100);
  digitalWrite(PM_RESET, HIGH);

  digitalWrite(LED_PWR, LOW);

  for (int i = 0; i < 6; i++)
    _mac[i] = EEPROM.read(EEPROM_MAC+i);

  for (int i = 0; i < 4; i++)
    _ip[i] = EEPROM.read(EEPROM_IP+i);

  for (int i = 0; i < 4; i++)
    _host[i] = EEPROM.read(EEPROM_SERVER_IP+i);

  for (int i = 0; i < 40; i++)
    _topic[i] = EEPROM.read(EEPROM_BASE_TOPIC+i);
  _topic[40] = '\0';

  for (int i = 0; i < 20; i++)
    _name[i] = EEPROM.read(EEPROM_NAME+i);
  _name[20] = '\0';

  Ethernet.init(4);
  Ethernet.begin(_mac, _ip);
  _client = new PubSubClient(_host, _port, callbackMQTT, _ethClient);

  pmsSerial.begin(9600);
  pms.passiveMode();
  pms.wakeUp();

  checkMQTT();

  Wire.begin();

  if (! sgp.begin()) {
    publishValue("debug", (char*)"Failed to init SGP");
  }
  if (! scd4x.begin()) {
    publishValue("debug", (char*)"Failed to init SCD4x");
  }
  if (! INA.begin()) {
    publishValue("debug", (char*)"Failed to init INA");
  }


}

long lastPublish = 0;
void loop()
{
  wdt_reset();

  char sresult[20];

  checkMQTT();
  _client->loop();

  // INA219
  float voltage = INA.getBusVoltage();
  digitalWrite(LED_PWR, voltage > 3.25 && voltage < 3.35);

  if ((millis() - lastPublish) < PUBLISH_INTERVAL) return;
  lastPublish = millis();

  sprintf(sresult, "%d.%02d", (int)voltage, (int)(voltage*100)%100);
  publishValue("Power/Voltage", sresult);

  float current = INA.getShuntVoltage_mV() / 0.1;
  sprintf(sresult, "%d.%02d", (int)current, (int)(current*100)%100);
  publishValue("Power/mA", sresult);

  // CO2 (and Temperature and Humidity)
  scd4x.readMeasurement();
  uint16_t co2 = scd4x.getCO2();
  float temperature = scd4x.getTemperature();
  float humidity = scd4x.getHumidity();
  sprintf(sresult, "%i", co2);
  publishValue("CO2", sresult);

  sprintf(sresult, "%d.%02d", (int)temperature, (int)(temperature*100)%100);
  publishValue("Temperature", sresult);

  sprintf(sresult, "%d.%02d", (int)humidity, (int)(humidity*100)%100);
  publishValue("Humidity", sresult);

  // VOC
  int32_t voc_index;
  voc_index = sgp.measureVocIndex((int)temperature, (int)humidity);

  sprintf(sresult, "%li", voc_index);
  publishValue("VOC", sresult);

  // PMS
  while (pmsSerial.available()) { pmsSerial.read(); }
  pms.requestRead();
  if (pms.readUntil(pmsValues)) {
    sprintf(sresult, "%i", pmsValues.PM_AE_UG_1_0);
    publishValue("PM1.0", sresult);
    sprintf(sresult, "%i", pmsValues.PM_AE_UG_2_5);
    publishValue("PM2.5", sresult);
    sprintf(sresult, "%i", pmsValues.PM_AE_UG_10_0);
    publishValue("PM10", sresult);
  }
}
