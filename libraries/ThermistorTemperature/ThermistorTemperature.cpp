
/* Ripped off from https://learn.adafruit.com/thermistor/using-a-thermistor */

#include <ThermistorTemperature.h>


ThermistorTemperature::ThermistorTemperature(int pin, uint16_t seriesResistor, uint16_t thermisterNominial, uint16_t bCoefficient)
{
  _pin = pin;
  _seriesResistor = seriesResistor;
  _thermisterNominial = thermisterNominial;
  _bCoefficient = bCoefficient;
  pinMode(_pin, INPUT);
}


float ThermistorTemperature::GetTemperature()
{
  uint8_t i;
  float average;
  int samples[NUMSAMPLES];

  // take N samples in a row, with a slight delay
  i=0;
  do
  {
   samples[i] = analogRead(_pin);
   if (++i < NUMSAMPLES)
     delay(10);
  } while (i < NUMSAMPLES);

  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) 
     average += samples[i];

  average /= NUMSAMPLES;

  // convert the value to resistance
  average = 1023 / average - 1;
  average = _seriesResistor / average;

  float steinhart;
  steinhart = average / _thermisterNominial;   // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= _bCoefficient;                  // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  return steinhart;
}
