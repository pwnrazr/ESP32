/*
 * ESP32 - Buzzer, Magenetic Door Switch and Room Ambient Lighting
*/

#include "led.h"
#include <WiFi.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define WIFI_SSID "Ayam Goreng"
#define WIFI_PASSWORD "pwnrazr1234"

#define MQTT_HOST IPAddress(192, 168, 1, 184)
#define MQTT_PORT 1883
#define MQTT_USER "pwnrazr"
#define MQTT_PASS "pwnrazr123"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;

const long interval1 = 100; // Beep timer
const long interval2 = 250; // Door switch polling

unsigned int beep = 0;
bool beeping = false;

int doorState = 1;         // current state of the button
int lastdoorState = 1;     // previous state of the button

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
		xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  mqttClient.subscribe("esp32/beepamount", 2);
  mqttClient.subscribe("esp32/forcestopbeep", 2);
  mqttClient.subscribe("esp32/led", 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);

  String topicstr;
  String payloadstr;
  
  for (int i = 0; i < len; i++) 
  {
    payloadstr = String(payloadstr + (char)payload[i]);  //convert payload to string
  }
  
  for(int i = 0; i <= 50; i++)
  {
    topicstr = String(topicstr + (char)topic[i]);  //convert topic to string
  }

  if(topicstr == "esp32/beepamount")
  {
    beep = payloadstr.toInt();
  }
  else if(topicstr == "esp32/forcestopbeep")
  {
    beep = 0;
    ledcWriteTone(0,0);
    beeping = false;
    Serial.println("Beep force stopped");
  }
  else if(topicstr == "esp32/led")
  {
    if(payloadstr == "1")
    {
       FastLED.setBrightness(BRIGHTNESS);
    }
    else if(payloadstr == "0")
    {
       FastLED.setBrightness(0);
    }
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  //Serial.begin(115200); //uncomment to enable debugging
  Serial.println();
  Serial.println();

  pinMode(32, INPUT_PULLUP);  // Set pin 32 where door switch is connected to as Input and pulled-up with internal resistors

  ledcSetup(0,1E5,12);  // Setup for buzzer
  ledcAttachPin(33,0);  // Pin 33 is connected to Gate of mosfet controlling buzzer
  
  ledsetup();
  
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);
  
  connectToWifi();

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("ESP32");

  // No authentication by default
  ArduinoOTA.setPassword("pwnrazr123");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}

void loop() 
{
  ledloop();
  
  ArduinoOTA.handle();
  
  unsigned long currentMillis = millis();

  // Beep function
  if (currentMillis - previousMillis1 >= interval1) 
  {
    previousMillis1 = currentMillis;

    if(beep > 0 && beeping == false)
    {
      Serial.println("Beep!");
      Serial.println(beep);
      ledcWriteTone(0,5000);
      beeping = true;
    }
    else if(beep != 0 && beeping == true)
    {
      Serial.println("Off Beep");
      Serial.println(beep);
      ledcWriteTone(0,0);
      beeping = false;
      beep--;
    }
  }

  // doorswitch polling
  if (currentMillis - previousMillis2 >= interval2) 
  {
    previousMillis2 = currentMillis;

    doorState = digitalRead(32);  //get current door state

    if (doorState != lastdoorState) 
    {
      if (doorState == LOW) 
      {
        Serial.println("DoorState LOW");
        mqttClient.publish("esp32/doorState", 0, true, "0");
        //beep = 1;
      }
      else 
      {
        Serial.println("DoorState HIGH");
        mqttClient.publish("esp32/doorState", 0, true, "1");
        //beep = 2;
      }
  }
    lastdoorState = doorState;
  }
}
