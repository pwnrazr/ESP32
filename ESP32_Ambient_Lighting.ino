/* To setup credentials create a settings.h with:
  #define WIFI_SSID "SSID"
  #define WIFI_PASSWORD "WiFi Password"
  
  #define OTA_PASS "OTA Password"
  
  #define MQTT_HOST IPAddress(192, 168, 1, 1)
  #define MQTT_PORT 1883
  #define MQTT_USER "MQTT user"
  #define MQTT_PASS "MQTT pass"
*/

#include "led.h"
#include "comms.h"
#include "ota.h"
#include "sensors.h"

void setup()
{
  Serial.begin(115200);
  wifiSetup();
  otaSetup();
  ledsetup();
  sensorSetup();
  
  pinMode(roomclock_pin, OUTPUT);
  digitalWrite(roomclock_pin, HIGH);
}

void loop()
{
  EVERY_N_SECONDS(15) // Heartbeat
  {
    mqttClient.publish("esp32/heartbeat", MQTT_QOS, false, "Hi");
  }
  
  EVERY_N_SECONDS(5)
  {
    sgp30.measureAirQuality();
    
    sensors_event_t rh, temp;
    aht.getEvent(&rh, &temp);// populate temp and humidity objects with fresh data

    //Serial.print(sgp30.CO2);
    //Serial.print(sgp30.TVOC);
    //temp.temperature
    //humidity.relative_humidity
    
    char eco2[6];
    char tvoc[6];
    char temperatureChar[10];
    char humidityChar[10];
    
    float temperature = temp.temperature;
    float humidity = rh.relative_humidity;

    sprintf(temperatureChar, "%lf", temperature);
    sprintf(humidityChar, "%lf", humidity);
    itoa(sgp30.CO2, eco2, 10);
    itoa(sgp30.TVOC, tvoc, 10);
    
    mqttClient.publish("esp32/sensor/eco2", MQTT_QOS, false, eco2);
    mqttClient.publish("esp32/sensor/tvoc", MQTT_QOS, false, tvoc);
    mqttClient.publish("esp32/sensor/temperature", MQTT_QOS, false, temperatureChar);
    mqttClient.publish("esp32/sensor/humidity", MQTT_QOS, false, humidityChar);
  }
  
  ArduinoOTA.handle();
  ledloop();
  //sensorloop();
}
