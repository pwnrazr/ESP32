#include "bluetooth.h"
#include "led.h"
#include "comms.h"
#include "ota.h"
#include "time.h"

unsigned int dc_time = 0;

// Time related
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;  // Kuala Lumpur GMT+8 = 8 - 1 * (60 * 60) = 25200secs
const int   daylightOffset_sec = 3600;
bool haveOnClock = false; // To save whether the auto on/off has set the state or not. Prevent setting on/off multiple times

void setup()
{
  //Serial.begin(115200);
  BTsetup();
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
  EVERY_N_SECONDS(1) 
  { 
    if((WiFi.status() != WL_CONNECTED))
    {
      dc_time++;
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  EVERY_N_SECONDS(60)
  {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      SerialBT.println("Failed to obtain time");
    }
    //SerialBT.println(&timeinfo, "Hr:%H");
    
    if(timeinfo.tm_hour == 22 && haveOnClock == false){ // Turn off at 10PM
      digitalWrite(roomclock_pin, LOW);
      //SerialBT.println("Auto turn OFF clock");
      haveOnClock = true;
    }
    else if(timeinfo.tm_hour == 10 && haveOnClock == true){  // Turn on at 10AM
      digitalWrite(roomclock_pin, HIGH);
      //SerialBT.println("Auto turn ON clock");
      haveOnClock = false;
    }
  }
  
  ArduinoOTA.handle();
  ledloop();

  if (SerialBT.available())
  {
    String btString = SerialBT.readString();
    btString.trim();
    SerialBT.print("Received:");
    SerialBT.println(btString);

    if(btString == "help")
    {
      SerialBT.println("led=0/1");
      SerialBT.println("clock=0/1");
      SerialBT.println("restart");
      SerialBT.println("status");
      SerialBT.println("time");
      SerialBT.println("nextledeffect");
    }
    else if(btString == "clock=1")
    {
      digitalWrite(roomclock_pin, HIGH);
    }
    else if(btString == "clock=0")
    {
      digitalWrite(roomclock_pin, LOW);
    }
    else if(btString == "led=1")
    {
      FastLED.setBrightness(255);
    }
    else if(btString == "led=0")
    {
      FastLED.setBrightness(0);
    }
    else if(btString == "status")
    {
      SerialBT.print("WiFi Status: ");
      SerialBT.println(WiFi.status());
      SerialBT.print("IP: ");
      SerialBT.println(WiFi.localIP());
      SerialBT.print("Time disconnected(s):");
      SerialBT.println(dc_time);
    }
    else if(btString == "restart")
    {
      ESP.restart();
    }
    else if(btString == "time")
    {
      printLocalTime();
    }
    else if(btString == "nextledeffect")
    {
      changeledEffect();
      SerialBT.print("curEffect:");
      SerialBT.println(curEffect);
    }
    else
    {
      SerialBT.println("ERROR");
    }
  }
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    SerialBT.println("Failed to obtain time");
    return;
  }
  SerialBT.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  SerialBT.print("Day of week: ");
  SerialBT.println(&timeinfo, "%A");
  SerialBT.print("Month: ");
  SerialBT.println(&timeinfo, "%B");
  SerialBT.print("Day of Month: ");
  SerialBT.println(&timeinfo, "%d");
  SerialBT.print("Year: ");
  SerialBT.println(&timeinfo, "%Y");
  SerialBT.print("Hour: ");
  SerialBT.println(&timeinfo, "%H");
  SerialBT.print("Hour (12 hour format): ");
  SerialBT.println(&timeinfo, "%I");
  SerialBT.print("Minute: ");
  SerialBT.println(&timeinfo, "%M");
  SerialBT.print("Second: ");
  SerialBT.println(&timeinfo, "%S");

  SerialBT.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  SerialBT.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  SerialBT.println(timeWeekDay);
}

void setSchedClock()  // To set haveOnClock properly so that scheduling thing works fine if esp32 restart after 10pm
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    SerialBT.println("Failed to obtain time");
  }
  //SerialBT.println(&timeinfo, "Hr:%H");

  if(timeinfo.tm_hour <= 10 || timeinfo.tm_hour >= 23)
  {
    haveOnClock = true;
  }
}
