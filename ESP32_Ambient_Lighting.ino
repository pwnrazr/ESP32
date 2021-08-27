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
  
  ArduinoOTA.handle();
  ledloop();
  sensorloop();
}
