#include "bluetooth.h"
#include "led.h"
#include "comms.h"
#include "ota.h"

void setup()
{
  Serial.begin(115200);
  BTsetup();
  ledsetup();
  wifiSetup();
  webServSetup();
  otaSetup();
}

void loop()
{
  ArduinoOTA.handle();
  ledloop();

  while (SerialBT.available())
  {
    String btString = SerialBT.readString();
    btString.trim();
    SerialBT.print("Received:");
    SerialBT.println(btString);

    if(btString == "led=1")
    {
      FastLED.setBrightness(255);
    }
    else if(btString =="led=0")
    {
      FastLED.setBrightness(0);
    }
    else if(btString == "status")
    {
      SerialBT.println(WiFi.localIP());
    }
  }
}
