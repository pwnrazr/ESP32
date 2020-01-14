/*
 * ESP32 - Buzzer, Magenetic Door Switch and Room Ambient Lighting
*/

#include "led.h"
#include "comms.h"
#include "ota.h"

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillisLED = 0;

const long interval1 = 100; // Beep timer
const long interval2 = 250; // Door switch polling
const long intervalLED = 17; // LED update speed (ms)

unsigned int beep = 0;
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
  else if(topicstr == "esp32/brightness")
  {
    curBrightness = payloadstr.toInt();
    pendingBrightness = true;
  }
  else if(topicstr == "esp32/R")
  {
    ledR = payloadstr.toInt();
  }
  else if(topicstr == "esp32/G")
  {
    ledG = payloadstr.toInt();
  }
  else if(topicstr == "esp32/B")
  {
    ledB = payloadstr.toInt();
    rgbReady = true;
  }
  else if(topicstr == "esp32/alert")  // on door state function
  {
    //mqttClient.publish("esp32/debug", 0, false, "received alert");
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
        mqttClient.publish("esp32/doorState", 0, false, "0");
        //beep = 1;
      }
      else 
      {
        Serial.println("DoorState HIGH");
        mqttClient.publish("esp32/doorState", 0, false, "1");
        //beep = 2;
      }
  }
    lastdoorState = doorState;
  }

  // LED update
  if (currentMillis - previousMillisLED >= intervalLED) 
  {
    previousMillisLED = currentMillis;
    
    if(pendingBrightness == true && ledUser == true)  //only set brightness when led is turned on
    {
      FastLED.setBrightness(curBrightness);
      pendingBrightness == false;
    }
    
    if(rgbReady == true)
    {
      for(int i = 0; i < NUM_LEDS; i++) 
    {
      leds[i].setRGB(ledR, ledG, ledB); //Set colors
    }

    FastLEDshowESP32(); //update LED
    rgbReady = false;
    }
  }
}
