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

// Register to topic mapping. All registers on the SDM are 2 bytes long.
#define MAP_SIZE 52
typedef struct {
  uint16_t address;
  const char *subtopicA PROGMEM;
  const char *subtopicB PROGMEM;
} Mapping[MAP_SIZE];

// The topic is split up into two parts so the compiler can reduce memory consumption.
// Some of these are commented out... because 328p.
const Mapping mappings = {
  { 0x0000, "V", "L1"}, // Voltage (L1)
  { 0x0002, "V", "L2"}, // Voltage (L2)
  { 0x0004, "V", "L3"}, // Voltage (L3)
  { 0x0006, "A", "L1"}, // Current (L1)
  { 0x0008, "A", "L2"}, // Current (L2)
  { 0x000A, "A", "L3"}, // Current (L3)
  { 0x000C, "W", "L1"}, // Power (L1)
  { 0x000E, "W", "L2"}, // Power (L2)
  { 0x0010, "W", "L3"}, // Power (L3)
  { 0x0012, "VA", "L1"}, // Apparent Power (L1)
  { 0x0014, "VA", "L2"}, // Apparent Power (L2)
  { 0x0016, "VA", "L3"}, // Apparent Power (L3)
  { 0x0018, "VAr", "L1"}, // Reactive Power (L1)
  { 0x001A, "VAr", "L2"}, // Reactive Power (L2)
  { 0x001C, "VAr", "L3"}, // Reactive Power (L3)
  { 0x001E, "PF", "L1"}, // Power Factor (L2)
  { 0x0020, "PF", "L2"}, // Power Factor (L2)
  { 0x0022, "PF", "L3"}, // Power Factor (L3)
  { 0x0024, "PA", "L1"}, // Phase Angle (L1)
  { 0x0026, "PA", "L2"}, // Phase Angle (L2)
  { 0x0028, "PA", "L3"}, // Phase Angle (L3)
  /* { 0x002A, "V", "Average"}, // Voltage (Average) */
  /* { 0x002E, "A", "Average"}, // Current (Average) */
  { 0x0030, "A", "Total"}, // Current (Total)
  { 0x0034, "W", "Total"}, // Power (Total)
  { 0x0038, "VA", "Total"}, // Apparent Power (Total)
  { 0x003C, "VAr", "Total"}, // Reactive Power (Total)
  { 0x003E, "PF", "Total"}, // Power Factor (Total)
  { 0x0042, "PA", "Total"}, // Phase Angle (Total)
  { 0x0046, "F", NULL}, // Frequency
  { 0x0048, "ImP", "Total"}, // Import Power (Total)
  /* { 0x004A, "ExP", "Total"} // Export Power (Total) */
  { 0x004C, "ImVAr", "Total"}, // Import Reactive Power (Total)
  /* { 0x004E, "ExVAr", "Total"}, // Export Reactive Power (Total) */
  /* { 0x0056, "W", "Max"}, // Power (Max) */
  { 0x0064, "VA", "Total"}, // Apparent Power (Total)
  /* { 0x0066, "VA", "Max"}, // Apparent Power (Max) */
  { 0x0068, "A", "Neutral"}, // Current (Neutral)
  /* { 0x006A, "A", "Neutral/Max"}, // Current (Neutral, Max) */
  { 0x00C8, "V", "L1-L2"}, // Voltage (Between L1 and L2)
  { 0x00CA, "V", "L2-L3"}, // Voltage (Between L2 and L3)
  { 0x00CC, "V", "L3-L1"}, // Voltage (Between L3 and L1)
  /* { 0x00CE, "V", "L2LAverage"}, // Voltage (Average between phases) */
  /* { 0x00E0, "A", "Neutral/Average"}, // Current (Neutral, Average) */
  { 0x00EA, "THDV", "L1"}, // Total Harmonic Distortion - Voltage (L1)
  { 0x00EC, "THDV", "L2"}, // Total Harmonic Distortion - Voltage (L2)
  { 0x00EE, "THDV", "L3"}, // Total Harmonic Distortion - Voltage (L3)
  { 0x00F0, "THDA", "L1"}, // Total Harmonic Distortion - Current (L1)
  { 0x00F2, "THDA", "L2"}, // Total Harmonic Distortion - Current (L2)
  { 0x00F4, "THDA", "L3"}, // Total Harmonic Distortion - Current (L3)
  /* { 0x00F8, "THDV", "Average"}, // Total Harmonic Distortion - Voltage (Average) */
  /* { 0x00FA, "THDA", "Average"}, // Total Harmonic Distortion - Current (Average) */
  { 0x014E, "THDV", "L1-L2"}, // Total Harmonic Distortion Voltage (Between L1 and L2)
  { 0x0150, "THDV", "L2-L3"}, // Total Harmonic Distortion Voltage (Between L2 and L3)
  { 0x0152, "THDV", "L3-L1"}, // Total Harmonic Distortion Voltage (Between L3 and L1)
  /* { 0x0154, "THDV", "L2LAverage"},  // Total Harmonic Distortion Voltage (Average between phases) */
  { 0x0156, "mkWh", "Total"}, // Metering (kWh)
  { 0x0158, "mkVArh", "Total"}, // Metering (Reactive Power kVAr)
  { 0x0166, "mkWh", "L1"}, // Metering (kWh for L1)
  { 0x0168, "mkWh", "L2"}, // Metering (kWh for L2)
  { 0x016A, "mkWh", "L3"}, // Metering (kWh for L3)
  { 0x0178, "mkVArh", "L1"}, // Metering (KVAr for L1)
  { 0x017A, "mkVArh", "L2"}, // Metering (KVAr for L2)
  { 0x017C, "mkVArh", "L3"} // Metering (KVAr for L3)
};
