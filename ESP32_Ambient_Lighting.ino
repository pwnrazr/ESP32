#include "led.h"
#include "comms.h"
#include "ota.h"
#include "time.h"

// Time related
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;  // Kuala Lumpur GMT+8 = 8 - 1 * (60 * 60) = 25200secs
const int   daylightOffset_sec = 3600;
bool haveOnClock = false; // To save whether the auto on/off has set the state or not. Prevent setting on/off multiple times

void setup()
{
  Serial.begin(115200);
  ledsetup();
  wifiSetup();
  otaSetup();

  pinMode(roomclock_pin, OUTPUT);
  digitalWrite(roomclock_pin, HIGH);

  // Time related config
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setSchedClock();
}

void loop()
{ 
  EVERY_N_SECONDS(60)
  {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
    }
    
    if(timeinfo.tm_hour == 22 && haveOnClock == false){ // Turn off at 10PM
      digitalWrite(roomclock_pin, LOW);
      mqttClient.publish("esp32/clockState", 2, false, "false");
      haveOnClock = true;
    }
    else if(timeinfo.tm_hour == 10 && haveOnClock == true){  // Turn on at 10AM
      digitalWrite(roomclock_pin, HIGH);
      mqttClient.publish("esp32/clockState", 2, false, "true");
      haveOnClock = false;
    }
  }
  
  EVERY_N_SECONDS(15) // Heartbeat
  {
    mqttClient.publish("esp32/heartbeat", 2, false, "Hi");
  }
  
  ArduinoOTA.handle();
  //ledloop();
}

void setSchedClock()  // To set haveOnClock properly so that scheduling thing works fine if esp32 restart after 10pm
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }

  if(timeinfo.tm_hour <= 10 || timeinfo.tm_hour >= 23)
  {
    haveOnClock = true;
  }
}
