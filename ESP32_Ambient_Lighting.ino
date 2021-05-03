#include "bluetooth.h"
#include "led.h"
#include "comms.h"
#include "ota.h"

unsigned int dc_time = 0;

void setup()
{
  //Serial.begin(115200);
  BTsetup();
  ledsetup();
  wifiSetup();
  webServSetup();
  otaSetup();

  pinMode(roomclock_pin, OUTPUT);
  digitalWrite(roomclock_pin, HIGH);
}

void loop()
{
  EVERY_N_SECONDS(1) 
  { 
    if((WiFi.status() != WL_CONNECTED))
    {
      dc_time++;
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
  
  ArduinoOTA.handle();
  ledloop();

  if (SerialBT.available())
  {
    String btString = SerialBT.readString();
    btString.trim();
    SerialBT.print("Received:");
    SerialBT.println(btString);

    if(btString == "help")
    {
      SerialBT.println("led=0/1");
      SerialBT.println("clock=0/1");
      SerialBT.println("restart");
      SerialBT.println("status");
    }
    else if(btString == "clock=1")
    {
      digitalWrite(roomclock_pin, HIGH);
    }
    else if(btString == "clock=0")
    {
      digitalWrite(roomclock_pin, LOW);
    }
    else if(btString == "led=1")
    {
      FastLED.setBrightness(255);
    }
    else if(btString == "led=0")
    {
      FastLED.setBrightness(0);
    }
    else if(btString == "status")
    {
      SerialBT.print("WiFi Status: ");
      SerialBT.println(WiFi.status());
      SerialBT.print("IP: ");
      SerialBT.println(WiFi.localIP());
      SerialBT.print("Time disconnected(s):");
      SerialBT.println(dc_time);
    }
    else if(btString == "restart")
    {
      ESP.restart();
    }
    else
    {
      SerialBT.println("ERROR");
    }
  }
}
