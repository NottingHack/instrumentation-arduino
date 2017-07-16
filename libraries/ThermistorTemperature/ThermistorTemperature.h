

// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5

#include <Arduino.h>

class ThermistorTemperature
{
private:
  int _pin;
  uint16_t _thermisterNominial;
  uint16_t _bCoefficient;
  uint16_t _seriesResistor;

public:
  ThermistorTemperature(int pin, uint16_t seriesResistor, uint16_t thermisterNominial, uint16_t bCoefficient);
  float GetTemperature();
};

