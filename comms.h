#include "settings.h"
#include <Arduino.h>
#include <WiFi.h>
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

#define roomclock_pin 32

#define MQTT_QOS 0

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void ledStateSync();

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

  mqttClient.subscribe("esp32/clock", MQTT_QOS);
  mqttClient.subscribe("esp32/led", MQTT_QOS);
  mqttClient.subscribe("esp32/restart", MQTT_QOS);
  mqttClient.subscribe("esp32/sync", MQTT_QOS);
  mqttClient.subscribe("esp32/ambient_light/switch", MQTT_QOS);
  mqttClient.subscribe("esp32/ambient_light/brightness/set", MQTT_QOS);
  mqttClient.subscribe("esp32/ambient_light/rgb/set", MQTT_QOS);
  
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

  mqttClient.publish("esp32/connect", MQTT_QOS, false, CUR_IP);  // Publish IP on connect
  mqttClient.publish("esp32/clockState", MQTT_QOS, false, "true");  // Publish clock state as ON
  mqttClient.publish("esp32/ledState", MQTT_QOS, false, "10,8900346,true");  // Publish ledState as set color
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
  if(topicstr == "esp32/restart")
  {
    ESP.restart();
  }

  if (topicstr == "esp32/sync")
  {
    if(digitalRead(roomclock_pin) == HIGH)
    {
      mqttClient.publish("esp32/clockState", MQTT_QOS, false, "true");
    }
    else if(digitalRead(roomclock_pin) == LOW)
    {
      mqttClient.publish("esp32/clockState", MQTT_QOS, false, "false");
    }

    ledStateSync();
  }
  
  if (topicstr == "esp32/clock")
  {
    if (payloadstr == "true")
    {
      digitalWrite(roomclock_pin, HIGH);
      mqttClient.publish("esp32/clockState", MQTT_QOS, false, "true");
    }
    else if (payloadstr == "false")
    {
      digitalWrite(roomclock_pin, LOW);
      mqttClient.publish("esp32/clockState", MQTT_QOS, false, "false");
    }
  }

  if (topicstr == "esp32/led")
  {
    char * strtokIndx;

    char state[8];

    strtokIndx = strtok(payload, ",");
    trueBrightness = map(atoi(strtokIndx), 1, 100, 3, 255);

    strtokIndx = strtok(NULL, ",");
    rgbval = atol(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    strcpy(state, strtokIndx);
    /*
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = rgbval;
    }*/

    byte R = rgbval >> 16 ;

    byte G = (rgbval & 0x00ff00) >> 8;
    
    byte B = (rgbval & 0x0000ff);
    
    if (strcmp(state, "true") == 0)
    {
      //FastLED.setBrightness(brightness);
      otwBrightness = trueBrightness;
      fadeBrightness = true;

      otwR = R;
      otwG = G;
      otwB = B;
      
      fadeColor = true;
      ledState = true;
    }
    else if (strcmp(state, "false") == 0)
    {
      //FastLED.setBrightness(0);
      //FastLEDshowESP32();
      otwBrightness = 0;
      fadeBrightness = true;
      ledState = false;
    }
    //FastLEDshowESP32();
    ledStateSync();
  }

  if (topicstr == "esp32/ambient_light/switch")
  {
    if (payloadstr == "true")
    {
      FastLED.setBrightness(brightness);
      ledState = true;
    }
    else if (payloadstr== "false")
    {
      FastLED.setBrightness(0);
      ledState = false;
    }
    FastLEDshowESP32();
    ledStateSync();
  }

  if (topicstr == "esp32/ambient_light/brightness/set")
  {
    brightness = map(payloadstr.toInt(), 1, 100, 3, 255);
    FastLED.setBrightness(brightness);
    FastLEDshowESP32();
    ledStateSync();
  }

  if (topicstr == "esp32/ambient_light/rgb/set")
  {
    char * strtokIndx;
    byte R, G, B;
    
    strtokIndx = strtok(payload, ",");
    R = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    G = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    B = atoi(strtokIndx);

    rgbval = ((long)R << 16L) | ((long)G << 8L) | (long)B;

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = rgbval;
    }
    
    FastLEDshowESP32();
    ledStateSync();
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

void ledStateSync()
{
    int brightnessConv = map(trueBrightness, 3, 255, 1, 100);
    char ledStateChar[8];
    String state;
    char message[30];
    
    if(ledState)
    {
      state = "true";
    }
    else
    {
      state = "false";
    } 
    
    state.toCharArray(ledStateChar, 8);
    
    snprintf(
      message,
      30,
      "%d,%ld,%s",
      brightnessConv,
      rgbval,
      ledStateChar
    );
    
    mqttClient.publish("esp32/ledState", MQTT_QOS, false, message);

    char brightnessChar[4];
    char rgbvalChar[16];

    byte red = rgbval >> 16;

    byte green = (rgbval & 0x00ff00) >> 8;

    byte blue = (rgbval & 0x0000ff);
    
    snprintf(brightnessChar, 4, "%d", brightnessConv);
    snprintf(rgbvalChar, 16, "%d,%d,%d", red, green, blue);
    
    mqttClient.publish("esp32/ambient_light/status", MQTT_QOS, false, ledStateChar);
    mqttClient.publish("esp32/ambient_light/brightness/status", MQTT_QOS, false, brightnessChar);
    mqttClient.publish("esp32/ambient_light/rgb/status", MQTT_QOS, false, rgbvalChar);
}
