#include "bluetooth.h"
#include "led.h"
#include "comms.h"
#include "ota.h"

int num = 0;

void setup() 
{
  Serial.begin(115200);
  Serial.println("boot");
  
  BTsetup();
  
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

  EVERY_N_SECONDS( 2 ) 
  {
    SerialBT.println(num);
    num++;  // Test only 
  }
}
