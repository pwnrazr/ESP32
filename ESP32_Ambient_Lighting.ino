/*
 * ESP32 - Buzzer, Magenetic Door Switch and Room Ambient Lighting
*/

#include "led.h"
#include "comms.h"
#include "ota.h"

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;

const long interval1 = 100; // Beep timer
const long interval2 = 250; // Door switch polling

unsigned int beep = 0;
bool beeping = false;

int doorState = 1;         // current state of the button
int lastdoorState = 1;     // previous state of the button

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
    if(payloadstr == "1")
    {
       FastLED.setBrightness(BRIGHTNESS);
    }
    else if(payloadstr == "0")
    {
       FastLED.setBrightness(0);
    }
  }
  else if(topicstr == "esp32/brightness")
  {
    FastLED.setBrightness(payloadstr.toInt());
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
