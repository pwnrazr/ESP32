#include "settings.h"
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#define roomclock_pin 32

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
