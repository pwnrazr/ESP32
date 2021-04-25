#include "led.h"
#include "comms.h"
#include "ota.h"

void setup() 
{
  Serial.begin(115200);
  Serial.println("boot");
  
  ledsetup();
  Serial.println("ledsetup");
  
  wifiSetup();
  Serial.println("wifisetup");
  
  webServSetup();
  Serial.println("webservsetup");
  
  otaSetup();
  Serial.println("otasetup");
}

void loop() 
{
  ArduinoOTA.handle();
  ledloop();
}
