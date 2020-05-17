unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillisLED = 0;
unsigned long PREV_MILLIS_HEARTBEAT = 0;

#define interval1 100 // Beep timer
#define interval2 250 // Door switch polling
#define intervalLED 17 // LED update speed (ms)
#define INTERVAL_HEARTBEAT 15000  // Heartbeat every 15 secs

byte beep = 0;
unsigned int beepFreq = 5000; //default beep frequency is 5000hz
bool beeping = false;

byte doorState = 1;         // current state of the button
byte lastdoorState = 1;     // previous state of the button

byte ledR = 0;  //For RGB
byte ledG = 0;
byte ledB = 0;
byte curBrightness = 255;
byte LED_MODE = 1;  // 1 for usual solid color RGB

bool rgbReady = false;
bool ledUser = false; // to determine if led is turned on by user or by door response and current status of led
bool pendingBrightness = false; // to determine whether to update brightness or not
bool SET_LED = false; // whether to set RGB values after changing modes
