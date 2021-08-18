#include "led.h"
#include "comms.h"
#include "ota.h"

void setup()
{
  Serial.begin(115200);
  wifiSetup();
  otaSetup();
  ledsetup();

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
}
