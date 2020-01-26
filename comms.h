#include <AsyncMqttClient.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WiFi.h>

#define WIFI_SSID "Ayam Goreng"
#define WIFI_PASSWORD "pwnrazr1234"
#define MQTT_HOST IPAddress(192, 168, 1, 184)
#define MQTT_PORT 1883
#define MQTT_USER "pwnrazr"
#define MQTT_PASS "pwnrazr123"

AsyncMqttClient mqttClient;

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

bool haveRun = false;

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
  mqttClient.subscribe("esp32/brightness", 2);
  mqttClient.subscribe("esp32/R", 2);
  mqttClient.subscribe("esp32/G", 2);
  mqttClient.subscribe("esp32/B", 2);
  mqttClient.subscribe("esp32/alert", 2);
  mqttClient.subscribe("esp32/beepFreq", 2);
  mqttClient.subscribe("esp32/reboot", 2);
  mqttClient.subscribe("esp32/reqstat", 2);
  mqttClient.subscribe("/esp32/mode", 2);

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
  
  if(haveRun == false)  // Run only once
  {
    mqttClient.publish("esp32/bootESP", 0, false, "ESP32 hard reset");  //  make sure to set retain to FALSE else it messes up deployment in nodered
    haveRun = true;
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

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) { //not yet moved due to its current nature
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
  
  /* BEGIN PROCESSING PAYLOAD AND TOPIC */
  if(topicstr == "esp32/beepamount")  // Call for beeping
  {
    beep = 0; //stops beeping first to prevent nonstop beeping
    ledcWriteTone(0,0);
    beeping = false;
    beep = payloadstr.toInt();
  }
  
  if(topicstr == "esp32/led")  // Call for on/off LED
  {
    if(payloadstr == "1") //on
    {
      {
        FastLED.setBrightness(curBrightness);
        ledUser = true;
      }
    }
    else if(payloadstr == "0")  //off
    {
       FastLED.setBrightness(0);
       ledUser = false;
    }
  }
  
  if(topicstr == "esp32/brightness") // Call for brightness setting of LED
  {
    curBrightness = payloadstr.toInt();
    pendingBrightness = true;
  }
  
  if(topicstr == "esp32/R")  // Call for RGB
  {
    ledR = payloadstr.toInt();
  }
  
  if(topicstr == "esp32/G")  // Call for RGB
  {
    ledG = payloadstr.toInt();
  }
  
  if(topicstr == "esp32/B")  // Call for RGB
  {
    ledB = payloadstr.toInt();
    rgbReady = true;
  }
  
  if(topicstr == "esp32/alert")  // Call for door alert function
  {
    //mqttClient.publish("esp32/debug", 0, false, "received alert");  // Use when debugging
    if(ledUser == false) // only run if leds are turned off
    {
      if(payloadstr == "1") // on
      {
        FastLED.setBrightness(curBrightness);
      }
      else if(payloadstr == "0")  // off
      {
        FastLED.setBrightness(0);
      }
    }
  }
  
  if(topicstr == "esp32/beepFreq") // Exposes beep frequency to user
  {
    beepFreq = payloadstr.toInt();
  }
  
  if(topicstr == "esp32/reboot") // Exposes reboot function
  {
    ESP.restart();
  }
  
  if(topicstr == "esp32/reqstat")  // Request statistics function
  {
    unsigned long REQ_STAT_CUR_MILLIS = millis(); // gets current millis
    
    char REQ_STAT_CUR_TEMPCHAR[60];
    
    snprintf(
      REQ_STAT_CUR_TEMPCHAR,
      60, 
      "%d.%d.%d.%d,%lu", 
      WiFi.localIP()[0], 
      WiFi.localIP()[1],
      WiFi.localIP()[2], 
      WiFi.localIP()[3],
      (int)REQ_STAT_CUR_MILLIS
    );  // convert string to char array for Millis. Elegance courtesy of Shahmi Technosparks
    
    mqttClient.publish("esp32/curstat", 0, false, REQ_STAT_CUR_TEMPCHAR); //publish to topic and tempchar as payload
  }
  
  if(topicstr == "/esp32/mode")
  {
    LED_MODE = payloadstr.toInt();
    if(LED_MODE == 1){
      SET_LED = true;
    }
  }
  /* END PROCESSING PAYLOAD AND TOPIC */
}

void mqttSetup() {
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);
}
