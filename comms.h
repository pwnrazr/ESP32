#include "settings.h"
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

void wifiSetup() 
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        //Serial.printf("WiFi Failed!\n");
        return;
    }

    //Serial.print("IP Address: ");
    //Serial.println(WiFi.localIP());
}

// WebServ
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "ESP32: ERROR");
}

void webServSetup() {   // webServ - processing things go here
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Pwnrazr's ESP32 here");
    });

    server.on("/reqled", HTTP_GET, [](AsyncWebServerRequest *request){
      if(ledState) {
        request->send(200, "text/plain", "LED,ON");
      } else {
        request->send(200, "text/plain", "LED,OFF");
      }
    });
    
    server.on("/led=1", HTTP_GET, [](AsyncWebServerRequest *request){
      FastLED.setBrightness(255);
      ledState = true;
      request->send(204);
    });
    server.on("/led=0", HTTP_GET, [](AsyncWebServerRequest *request){
      FastLED.setBrightness(0);
      ledState = false;
      request->send(204);
    });

    server.onNotFound(notFound);

    server.begin();
}
// WebServ
