#include <ArduinoOTA.h>

void otaSetup(){
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("pwn-ESP32");

  // No authentication by default
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      SerialBT.println("Start updating " + type);
    })
    .onEnd([]() {
      SerialBT.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      //SerialBT.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      SerialBT.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) SerialBT.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) SerialBT.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) SerialBT.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) SerialBT.println("Receive Failed");
      else if (error == OTA_END_ERROR) SerialBT.println("End Failed");
    });

  ArduinoOTA.begin();
}
