#include "Adafruit_SHT31.h"
#include <Adafruit_SGP30.h>
#include <Effortless_SPIFFS.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SGP30 sgp;

eSPIFFS fileSystem;

bool sensorsReady = false;  // For initial warmup of sensors
byte sensorCount = 0;

//  sgp30
int eco2, tvoc;
uint16_t TVOC_base, eCO2_base;

// sht31
float humidity, temperature;

// Dust Sensor
#define DUST_ADDR 8
union u_tag {
   float density; 
   byte density_byte[4];
} u;

void sendDustData();
void sendSHT31Data();
void sendSGP30Data();
void sendSGP30Baseline();
void spiffsSaveBaseline();
void spiffsLoadBaseline();

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
  sht31.begin(0x44);
  sgp.begin();

  spiffsLoadBaseline();   // Load previously saved baseline

  sendSHT31Data();  // We call this on setup as SGP30 gets temperature and humidity compensation from this sensor
}

void sensorloop()
{
  EVERY_N_SECONDS(5)    // Every N seconds because I want it to
  { // Dust
    sendDustData();
  }

  EVERY_N_SECONDS(10)    // Every 10 seconds because datasheet of SHT31 8-30 sec sample rate
  { // SHT31 Temp and Humidity
    sendSHT31Data();
  }

  EVERY_N_SECONDS(1)    // Every 1 second because datasheet says 1HZ sampling rate is best
  { // SGP30 eCO2 and TVOC
    sendSGP30Data();
  }
  
  EVERY_N_SECONDS(60)   // Every 60 seconds because I have no clue
  {
    sendSGP30Baseline();
  }

  EVERY_N_MINUTES(60)   // Saves current baseline every hour as stated in SGP30 datasheet
  {
    spiffsSaveBaseline();
  }
}

void sendSGP30Baseline()
{
  sgp.getIAQBaseline(&eCO2_base, &TVOC_base);
  
  char eco2BaselineChar[10];
  char tvocBaselineChar[10];

  itoa(eCO2_base, eco2BaselineChar, 10);
  itoa(TVOC_base, tvocBaselineChar, 10);
  
  if(sensorsReady)
  {
    mqttClient.publish("esp32/sensor/eco2Baseline", MQTT_QOS, false, eco2BaselineChar);
    mqttClient.publish("esp32/sensor/tvocBaseline", MQTT_QOS, false, tvocBaselineChar);
  }
}

void sendSGP30Data()
{
  if(sensorsReady == false)
  {
    if(sensorCount <= 30) // Wait for warmup
    {
      sensorCount++;
    }
    else
    {
      sensorsReady = true;
    }
  }
  
  sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));
  
  sgp.IAQmeasure();

  eco2 = sgp.eCO2;  // ppb
  tvoc = sgp.TVOC;  // ppm
  
  if(sensorsReady)
  {
    char eco2Char[10];
    char tvocChar[10];
    itoa(eco2, eco2Char, 10);
    itoa(tvoc, tvocChar, 10);
  
    mqttClient.publish("esp32/sensor/eco2", MQTT_QOS, false, eco2Char);
    mqttClient.publish("esp32/sensor/tvoc", MQTT_QOS, false, tvocChar);
  }
  else
  {
    mqttClient.publish("esp32", MQTT_QOS, false, "NOT READY");
  }
}

void sendSHT31Data()
{
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  
  if(h != 0)   // I keep getting readings of 0% RH which I doubt is correct.
  {            // So just don't set data if its 0%
    temperature = t;
    humidity = h;

    if(sensorsReady)
    {
      char temperatureChar[10];
      char humidityChar[10];
      
      //snprintf(temperatureChar, "%.2f", temperature);
      //snprintf(humidityChar, "%.2f", humidity);
      dtostrf(temperature, 4, 2, temperatureChar);
      dtostrf(humidity, 4, 2, humidityChar);
      
      mqttClient.publish("esp32/sensor/temperature", MQTT_QOS, false, temperatureChar);
      mqttClient.publish("esp32/sensor/humidity", MQTT_QOS, false, humidityChar);
    }
  }
}

void sendDustData()
{
  // Request dust sensor data from arduino nano
  Wire.requestFrom(DUST_ADDR, 4);
  if(Wire.available() == 4)
  {
    for(byte i = 0; i <= 4; i++)
    {
      u.density_byte[i] = Wire.read();
    }
  }
  
  if(sensorsReady)
  {
    char dustChar[10];
    //snprintf(dustChar, "%.2f", u.density);
    dtostrf(u.density, 4, 2,dustChar);
  
    mqttClient.publish("esp32/sensor/dust", MQTT_QOS, false, dustChar);
  }
}

void spiffsSaveBaseline()
{
  uint16_t eCO2_base_current, TVOC_base_current, eCO2_base_spiffs, TVOC_base_spiffs;
  
  sgp.getIAQBaseline(&eCO2_base_current, &TVOC_base_current);

  fileSystem.openFromFile("/eco2baseline.txt", eCO2_base_spiffs);
  fileSystem.openFromFile("/tvocbaseline.txt", TVOC_base_spiffs);

  if(eCO2_base_current != eCO2_base_spiffs)
  {
    fileSystem.saveToFile("/eco2baseline.txt", eCO2_base_current);
  }

  if(TVOC_base_current != TVOC_base_spiffs)
  {
    fileSystem.saveToFile("/tvocbaseline.txt", TVOC_base_current);
  }
}

void spiffsLoadBaseline()
{
  uint16_t eCO2_base_load, TVOC_base_load;
  
  fileSystem.openFromFile("/eco2baseline.txt", eCO2_base_load);
  fileSystem.openFromFile("/tvocbaseline.txt", TVOC_base_load);
  
  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  sgp.setIAQBaseline(eCO2_base_load, TVOC_base_load);  // Will vary for each sensor! (eCO2_baseline, TVOC_baseline)
}
