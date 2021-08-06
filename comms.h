#include "settings.h"
#include <Arduino.h>
#include <WiFi.h>
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

#define roomclock_pin 32

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

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
  switch (event) {
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
  /*
    mqttClient.subscribe("esp32/beepamount", 2);
    mqttClient.subscribe("esp32/forcestopbeep", 2);
  */

  mqttClient.subscribe("esp32/clock", 2);
  mqttClient.subscribe("esp32/led", 2);
  mqttClient.subscribe("esp32/restart", 2);
  char CUR_IP[20];
  snprintf(
    CUR_IP,
    20,
    "%d.%d.%d.%d",
    WiFi.localIP()[0],
    WiFi.localIP()[1],
    WiFi.localIP()[2],
    WiFi.localIP()[3]
  );  // convert string to char array

  mqttClient.publish("esp32/connect", 0, false, CUR_IP);  // Publish IP on connect
  mqttClient.publish("esp32/clockState", 2, false, "true");  // Publish clock state as ON
  mqttClient.publish("esp32/ledState", 2, false, "10,8900346,true");  // Publish ledState as set color
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String topicstr;
  String payloadstr;

  for (int i = 0; i < len; i++)
  {
    payloadstr = String(payloadstr + (char)payload[i]);  //convert payload to string
  }

  for (int i = 0; i <= 50; i++)
  {
    topicstr = String(topicstr + (char)topic[i]);  //convert topic to string
  }

  /* BEGIN PROCESSING PAYLOAD AND TOPIC */
  /*
    if(topicstr == "esp32/beepamount")  // Call for beeping
    {
    beep = 0; //stops beeping first to prevent nonstop beeping
    ledcWriteTone(0,0);
    beeping = false;
    beep = payloadstr.toInt();
    }
  */
  if(topicstr == "esp32/restart")
  {
    ESP.restart();
  }
  
  if (topicstr == "esp32/clock")
  {
    if (payloadstr == "true")
    {
      digitalWrite(roomclock_pin, HIGH);
    }
    else if (payloadstr == "false")
    {
      digitalWrite(roomclock_pin, LOW);
    }
  }

  if (topicstr == "esp32/led")
  {
    char * strtokIndx;

    int brightness;
    unsigned long rgbval;
    char state[8];

    strtokIndx = strtok(payload, ",");
    brightness = map(atoi(strtokIndx), 1, 100, 3, 255);

    strtokIndx = strtok(NULL, ",");
    rgbval = atol(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    strcpy(state, strtokIndx);

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = rgbval;
    }

    if (strcmp(state, "true") == 0)
    {
      FastLED.setBrightness(brightness);
    }
    else if (strcmp(state, "false") == 0)
    {
      FastLED.setBrightness(0);
    }
    FastLEDshowESP32();
  }
}

void wifiSetup()
{
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);

  connectToWifi();
}
