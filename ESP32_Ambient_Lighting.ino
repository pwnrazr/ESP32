/*
 * ESP32 - Buzzer, Magenetic Door Switch and Room Ambient Lighting
*/

#include "led.h"
#include "comms.h"
#include "ota.h"

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillisLED = 0;
unsigned long PREV_MILLIS_HEARTBEAT = 0;

const long interval1 = 100; // Beep timer
const long interval2 = 250; // Door switch polling
const long intervalLED = 17; // LED update speed (ms)
const long INTERVAL_HEARTBEAT = 15000;  // Heartbeat every 15 secs

unsigned int beep = 0;
unsigned int beepFreq = 5000; //default beep frequency is 5000hz
bool beeping = false;

int doorState = 1;         // current state of the button
int lastdoorState = 1;     // previous state of the button

unsigned int ledR = 0;  //For RGB
unsigned int ledG = 0;
unsigned int ledB = 0;
unsigned int curBrightness = 255;

bool rgbReady = false;
bool ledUser = false; // to determine if led is turned on by user or by door response and current status of led
bool pendingBrightness = false; // to determine whether to update brightness or not

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
  else if(topicstr == "esp32/led")  // Call for on/off LED
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
  else if(topicstr == "esp32/brightness") // Call for brightness setting of LED
  {
    curBrightness = payloadstr.toInt();
    pendingBrightness = true;
  }
  else if(topicstr == "esp32/R")  // Call for RGB
  {
    ledR = payloadstr.toInt();
  }
  else if(topicstr == "esp32/G")  // Call for RGB
  {
    ledG = payloadstr.toInt();
  }
  else if(topicstr == "esp32/B")  // Call for RGB
  {
    ledB = payloadstr.toInt();
    rgbReady = true;
  }
  else if(topicstr == "esp32/alert")  // Call for door alert function
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
  else if(topicstr == "esp32/beepFreq") // Exposes beep frequency to user
  {
    beepFreq = payloadstr.toInt();
  }
  else if(topicstr == "esp32/reboot") // Exposes reboot function
  {
    ESP.restart();
  }
  else if(topicstr == "esp32/reqstat")  // Request statistics function
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
  /* END PROCESSING PAYLOAD AND TOPIC */
}

void setup() {
  //Serial.begin(115200); //uncomment to enable debugging

  pinMode(32, INPUT_PULLUP);  // Set pin 32 where door switch is connected to as Input and pulled-up with internal resistors

  ledcSetup(0,1E5,12);  // Setup for buzzer
  ledcAttachPin(33,0);  // Pin 33 is connected to Gate of mosfet controlling buzzer
  
  ledsetup();
  
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttSetup();
  mqttClient.onMessage(onMqttMessage);  //not yet moved
  
  connectToWifi();
  
  otaSetup();
}

void loop() 
{
  ledloop();
  
  ArduinoOTA.handle();
  
  unsigned long currentMillis = millis();

  /* BEGIN BEEP FUNCTION */
  if (currentMillis - previousMillis1 >= interval1) 
  {
    previousMillis1 = currentMillis;

    if(beep > 0 && beeping == false)
    {
      ledcWriteTone(0,beepFreq);
      beeping = true;
    }
    else if(beep != 0 && beeping == true)
    {
      ledcWriteTone(0,0);
      beeping = false;
      beep--;
    }
  }
  /* END BEEP FUNCTION */

  /* BEGIN DOORSWITCH POLLING */
  if (currentMillis - previousMillis2 >= interval2) 
  {
    previousMillis2 = currentMillis;

    doorState = digitalRead(32);  //get current door state

    if (doorState != lastdoorState) 
    {
      if (doorState == LOW) 
      {
        Serial.println("DoorState LOW");
        mqttClient.publish("esp32/doorState", 0, false, "0");
      }
      else 
      {
        Serial.println("DoorState HIGH");
        mqttClient.publish("esp32/doorState", 0, false, "1");
      }
  }
    lastdoorState = doorState;
  }
  /* END DOORSWITCH POLLING */

  /* BEGIN LED REFRESH */
  if (currentMillis - previousMillisLED >= intervalLED) 
  {
    previousMillisLED = currentMillis;
    
    if(pendingBrightness == true && ledUser == true)  //only set brightness when led is turned on
    {
      FastLED.setBrightness(curBrightness);
      pendingBrightness == false;
    }
    
    if(rgbReady == true)  // Only set RGB colors when received. Not update RGB all the time
    {
      for(int i = 0; i < NUM_LEDS; i++) 
    {
      leds[i].setRGB(ledR, ledG, ledB); //Set colors
    }

    FastLEDshowESP32(); //update LED
    rgbReady = false;
    }
  }
  /* END LED REFRESH */

  /* BEGIN HEARTBEAT */
  if (currentMillis - PREV_MILLIS_HEARTBEAT >= INTERVAL_HEARTBEAT) 
  {
    PREV_MILLIS_HEARTBEAT = currentMillis;
    mqttClient.publish("/esp32/heartbeat", 0, false, "esp32 heartbeat");
  }
  /* END HEARTBEAT */

  /* BEGIN AUTO RESTART FUNCTION */
  if(currentMillis > 4094967296)
  {
    char tempchar[40];

    snprintf (tempchar, 40, "ESP32 auto reboot on millis: %lu", (int)currentMillis);  // convert string to char array
    
    mqttClient.publish("esp32/warn", 0, false, tempchar); //publish to topic and tempchar as payload
    
    delay(1000);
    ESP.restart();
  }
  /* END AUTO RESTART FUNCTION */
}
