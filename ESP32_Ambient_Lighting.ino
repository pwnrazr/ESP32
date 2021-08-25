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
  
  EVERY_N_SECONDS(3)
  {
    char eco2Char[10];
    char tvocChar[10];
    char h2Char[10];
    char ethChar[10];
    char temperatureChar[10];
    char humidityChar[10];
    
    sprintf(temperatureChar, "%lf", temperature);
    sprintf(humidityChar, "%lf", humidity);
    itoa(eco2, eco2Char, 10);
    itoa(tvoc, tvocChar, 10);
    itoa(h2, h2Char, 10);
    itoa(eth, ethChar, 10);
    
    if(sgpReady)
    {
      mqttClient.publish("esp32/sensor/eco2", MQTT_QOS, false, eco2Char);
      mqttClient.publish("esp32/sensor/tvoc", MQTT_QOS, false, tvocChar);
      mqttClient.publish("esp32/sensor/h2", MQTT_QOS, false, h2Char);
      mqttClient.publish("esp32/sensor/ethanol", MQTT_QOS, false, ethChar);
      mqttClient.publish("esp32/sensor/temperature", MQTT_QOS, false, temperatureChar);
      mqttClient.publish("esp32/sensor/humidity", MQTT_QOS, false, humidityChar);

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
  
  ArduinoOTA.handle();
  ledloop();
  sensorloop();
}
