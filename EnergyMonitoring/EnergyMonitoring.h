void callbackMQTT(char *topic, byte *payload, unsigned int length);
bool checkMQTT();
void queryEnergy();
void setup();
void loop();
void dbg_println(const char *msg);
void preTX();
void postTX();
void readNextRegister();
void incrRegister();

byte _server[4];
char _base_topic[40];
char _dev_name[20];
byte _mac[6];
byte _ip[4];

int currentRegister = 0;

// Register to topic mapping. All registers on the SDM are 4 bytes long.
#define MAP_SIZE 64
typedef struct {
  uint16_t address;
  const char *subtopicA;
  const char *subtopicB;
} Mapping[MAP_SIZE];

// The topic is split up into two parts so the compiler can reduce memory consumption
const Mapping mappings PROGMEM = {
  { 0x0000, "Voltage", "L1"},
  { 0x0002, "Voltage", "L2"},
  { 0x0004, "Voltage", "L3"},
  { 0x0006, "Current", "L1"},
  { 0x0008, "Current", "L2"},
  { 0x000A, "Current", "L3"},
  { 0x000C, "Power", "L1"},
  { 0x000E, "Power", "L2"},
  { 0x0010, "Power", "L3"},
  { 0x0012, "ApparentPower", "L1"},
  { 0x0014, "ApparentPower", "L2"},
  { 0x0016, "ApparentPower", "L3"},
  { 0x0018, "ReactivePower", "L1"},
  { 0x001A, "ReactivePower", "L2"},
  { 0x001C, "ReactivePower", "L3"},
  { 0x001E, "PowerFactor", "L1"},
  { 0x0020, "PowerFactor", "L2"},
  { 0x0022, "PowerFactor", "L3"},
  { 0x0024, "PhaseAngle", "L1"},
  { 0x0026, "PhaseAngle", "L2"},
  { 0x0028, "PhaseAngle", "L3"},
  { 0x002A, "Voltage", "Average"},
  { 0x002E, "Current", "Average"},
  { 0x0030, "Current", "Total"},
  { 0x0034, "Power", "Total"},
  { 0x0038, "ApparentPower", "Total"},
  { 0x003C, "ReactivePower", "Total"},
  { 0x003E, "PowerFactor", "Total"},
  { 0x0042, "PhaseAngle", "Total"},
  { 0x0046, "Frequency", NULL},
  { 0x0048, "Import_Power", "Total"},
  { 0x004A, "Export_Power", "Total"},
  { 0x004C, "Import_ReactivePower", "Total"},
  { 0x004E, "Export_ReactivePower", "Total"},
  { 0x0056, "Power", "Max"},
  { 0x0064, "ApparentPower", "Total"},
  { 0x0066, "ApparentPower", "Max"},
  { 0x0068, "Current", "Neutral"},
  { 0x006A, "Current", "Neutral/Max"},
  { 0x00C8, "Voltage", "L1-L2"},
  { 0x00CA, "Voltage", "L2-L3"},
  { 0x00CC, "Voltage", "L3-L1"},
  { 0x00CE, "Voltage", "L2LAverage"},
  { 0x00E0, "Current", "Neutral/Average"},
  { 0x00EA, "THD_Voltage", "L1"},
  { 0x00EC, "THD_Voltage", "L2"},
  { 0x00EE, "THD_Voltage", "L3"},
  { 0x00F0, "THD_Current", "L1"},
  { 0x00F2, "THD_Current", "L2"},
  { 0x00F4, "THD_Current", "L3"},
  { 0x00F8, "THD_Voltage", "Average"},
  { 0x00FA, "THD_Current", "Average"},
  { 0x014E, "THD_Voltage", "L1-L2"},
  { 0x0150, "THD_Voltage", "L2-L3"},
  { 0x0152, "THD_Voltage", "L3-L1"},
  { 0x0154, "THD_Voltage", "L2LAverage"},
  { 0x0156, "Metering_kWh", "Total"},
  { 0x0158, "Metering_kVArh", "Total"},
  { 0x0166, "Metering_kWh", "L1"},
  { 0x0168, "Metering_kWh", "L2"},
  { 0x016A, "Metering_kWh", "L3"},
  { 0x0178, "Metering_kVArh", "L1"},
  { 0x017A, "Metering_kVArh", "L2"},
  { 0x017C, "Metering_kVArh", "L3"}
};
