#include <ThermistorTemperature.h>

ThermistorTemperature *TT;

void setup()
{
  Serial.begin(9600);

  // Pin                = A0
  // Series resistor    = 10k
  // Thermistor nominal = 10k
  // B coefficient      = 3435
  TT = new ThermistorTemperature(A0, 10000, 10000, 3435);
}

void loop()
{
  Serial.println(TT->GetTemperature());
  delay(1000);
}
