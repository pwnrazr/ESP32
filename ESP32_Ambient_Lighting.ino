/*
 * ESP32 - Buzzer, Magenetic Door Switch and Room Ambient Lighting
 * 
 * To set OTA, network and MQTT credentials create a file "nodelogin" into root of C:\ and put the code below
 * 
 * #define WIFI_SSID "SSID"
 * #define WIFI_PASSWORD "NetPassword"
 * #define MQTT_HOST IPAddress(192, 168, 1, 000)
 * #define MQTT_PORT 1883
 * #define MQTT_USER "mqttUser"
 * #define MQTT_PASS "mqttPassword3"
 * #define OTA_PASS "otaPassword"
*/
#include "Global_variables.h"
#include "C:/nodelogin"
#include "led.h"
#include "comms.h"
#include "ota.h"

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
    switch(LED_MODE)
    {
      case 1:
        if(rgbReady == true)  // Only set RGB colors when received. Not update RGB all the time
        {
          for(int i = 0; i < NUM_LEDS; i++) 
        {
          leds[i].setRGB(ledR, ledG, ledB); //Set colors
        }
        FastLEDshowESP32(); //update LED
        rgbReady = false;
        }

        if(SET_LED == true) // Set LED colors to last set RGB values
        {
          for(int i = 0; i < NUM_LEDS; i++) 
        {
          leds[i].setRGB(ledR, ledG, ledB); //Set colors
        }
        FastLEDshowESP32(); //update LED
        SET_LED = false;
        }
        break;
      case 2:
        Fire2012();
        FastLEDshowESP32(); //update LED
        break;
      case 3:
        rainbow();
        FastLEDshowESP32(); //update LED
        break;
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
