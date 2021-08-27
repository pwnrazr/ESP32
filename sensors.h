#include <Adafruit_AHTX0.h>
#include <Adafruit_SGP30.h>

Adafruit_AHTX0 aht;
Adafruit_SGP30 sgp;

//  sgp30
int eco2, tvoc, h2, eth;
uint16_t TVOC_base, eCO2_base;
byte sgpCount = 0;
bool sgpReady = false;   // For 15 second initial warmup
bool baselineReady = false;

// aht10
sensors_event_t rh, temp;
float humidity, temperature;

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void sensorSetup()
{
  aht.begin();
  sgp.begin();

  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  sgp.setIAQBaseline(37644, 37704);  // Will vary for each sensor! (eCO2_baseline, TVOC_baseline)
}

void sensorloop()
{
  EVERY_N_SECONDS(3)
  {
    aht.getEvent(&rh, &temp);// populate temp and humidity objects with fresh data

    temperature = temp.temperature;
    humidity = rh.relative_humidity;

    sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));
    
    sgp.IAQmeasure();

    eco2 = sgp.eCO2;  // ppb
    tvoc = sgp.TVOC;  // ppm

    sgp.IAQmeasureRaw();

    h2 = sgp.rawH2;
    eth = sgp.rawEthanol;

    sgpCount += 3;  // For 3 seconds
    if(sgpCount >= 30)
    {
      sgpReady = true;
      sgpCount = 0;
      sgp.getIAQBaseline(&eCO2_base, &TVOC_base);
      baselineReady = true;
    }
  }
}
