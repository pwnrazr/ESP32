#include <Adafruit_AHTX0.h>
#include "SparkFun_SGP30_Arduino_Library.h"

Adafruit_AHTX0 aht;
SGP30 sgp30;

void sensorSetup()
{
  /*
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 or AHT20 found");
  if (!sgp30.begin()) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }*/
  aht.begin();
  sgp30.begin();
  sgp30.initAirQuality();
}

void sensorloop()
{
  EVERY_N_SECONDS(3)
  {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
    Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
    }

  EVERY_N_SECONDS(1)
  {
  sgp30.measureAirQuality();
  Serial.print("CO2: ");
  Serial.print(sgp30.CO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(sgp30.TVOC);
  Serial.println(" ppb");
  }
}
