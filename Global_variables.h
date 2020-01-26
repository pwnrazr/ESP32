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
unsigned int LED_MODE = 1;  // 1 for usual solid color RGB

bool rgbReady = false;
bool ledUser = false; // to determine if led is turned on by user or by door response and current status of led
bool pendingBrightness = false; // to determine whether to update brightness or not
bool SET_LED = false; // whether to set RGB values after changing modes
