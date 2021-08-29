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

// Dust Sensor
#define DUST_ADDR 8
union u_tag {
   float density; 
   byte density_byte[4];
} u;

void sendSensorData();

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
  sgp.setIAQBaseline(37856, 38117);  // Will vary for each sensor! (eCO2_baseline, TVOC_baseline)
}

void sensorloop()
{
  EVERY_N_SECONDS(3)
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
    Serial.println(u.density);
    
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
    sendSensorData();
  }
}

void sendSensorData()
{
  if(sgpReady)
  {
    char eco2Char[10];
    char tvocChar[10];
    char h2Char[10];
    char ethChar[10];
    char temperatureChar[10];
    char humidityChar[10];
    char dustChar[10];
    
    snprintf(temperatureChar, 10, "%.2f", temperature);
    snprintf(humidityChar, 10, "%.2f", humidity);
    snprintf(dustChar, 10, "%.2f", u.density);
    itoa(eco2, eco2Char, 10);
    itoa(tvoc, tvocChar, 10);
    itoa(h2, h2Char, 10);
    itoa(eth, ethChar, 10);
  
    mqttClient.publish("esp32/sensor/eco2", MQTT_QOS, false, eco2Char);
    mqttClient.publish("esp32/sensor/tvoc", MQTT_QOS, false, tvocChar);
    mqttClient.publish("esp32/sensor/h2", MQTT_QOS, false, h2Char);
    mqttClient.publish("esp32/sensor/ethanol", MQTT_QOS, false, ethChar);
    mqttClient.publish("esp32/sensor/temperature", MQTT_QOS, false, temperatureChar);
    mqttClient.publish("esp32/sensor/humidity", MQTT_QOS, false, humidityChar);
    mqttClient.publish("esp32/sensor/dust", MQTT_QOS, false, dustChar);
    
    if(baselineReady)
    {
      char eco2BaselineChar[10];
      char tvocBaselineChar[10];

      itoa(eCO2_base, eco2BaselineChar, 10);
      itoa(TVOC_base, tvocBaselineChar, 10);
      
      mqttClient.publish("esp32/sensor/eco2Baseline", MQTT_QOS, false, eco2BaselineChar);
      mqttClient.publish("esp32/sensor/tvocBaseline", MQTT_QOS, false, tvocBaselineChar);

      baselineReady = false;
    }
  }
  else
  {
    mqttClient.publish("esp32", MQTT_QOS, false, "NOT READY");
  }
}
