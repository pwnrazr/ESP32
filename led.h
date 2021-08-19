#include "FastLED.h"

FASTLED_USING_NAMESPACE

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
  
#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    25
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    45
CRGB leds[NUM_LEDS];

// -- The core to run FastLED.show()
#define FASTLED_SHOW_CORE 1

#define START_COLOR 8900346 // 135, 206, 250
#define START_COLOR_R 135
#define START_COLOR_G 206
#define START_COLOR_B 250

#define START_BRIGHTNESS 20

// -- Task handles for use in the notifications
static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

unsigned long rgbval = START_COLOR;
bool ledState = true;

// Brightness fade
byte trueBrightness = START_BRIGHTNESS; // Actual set brightness for sync to HA and GoogleHome
int curBrightness = START_BRIGHTNESS;   // Stores current brightness when fading
byte otwBrightness;                     // Received brightness to fade to
boolean fadeBrightness = false;

// Color fade
byte curR = START_COLOR_R;
byte curG = START_COLOR_G;
byte curB = START_COLOR_B;

byte otwR;  // Received color to fade to
byte otwG;
byte otwB;

boolean fadeColor = false;

/** show() for ESP32
 *  Call this function instead of FastLED.show(). It signals core FASTLED_SHOW_CORE to issue a show, 
 *  then waits for a notification that it is done.
 */
void FastLEDshowESP32()
{
  if (userTaskHandle == 0) {
    // -- Store the handle of the current task, so that the show task can
    //    notify it when it's done
    userTaskHandle = xTaskGetCurrentTaskHandle();

    // -- Trigger the show task
    xTaskNotifyGive(FastLEDshowTaskHandle);

    // -- Wait to be notified that it's done
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
    ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    userTaskHandle = 0;
  }
}

/** show Task
 *  This function runs on core FASTLED_SHOW_CORE and just waits for requests to call FastLED.show()
 */
void FastLEDshowTask(void *pvParameters)
{
  // -- Run forever...
  for(;;) {
    // -- Wait for the trigger
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // -- Do the show (synchronously)
    FastLED.show();

    // -- Notify the calling task
    xTaskNotifyGive(userTaskHandle);
  }
}

void setColor(byte inR, byte inG, byte inB)
{
  for (int i = 0; i < NUM_LEDS; i++) 
  {
    leds[i].red = inR;
    leds[i].green = inG;
    leds[i].blue = inB;
  }
  FastLEDshowESP32();
}

void ledsetup() {
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // LED brightness on startup
  FastLED.setBrightness(pgm_read_byte(&gamma8[map(START_BRIGHTNESS, 1, 255, 73, 255)]));

  int core = xPortGetCoreID();
  //Serial.print("Main code running on core ");
  //Serial.println(core);

  // -- Create the FastLED show task
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);
  
  setColor(curR, curG, curB); // Sets starting led color
}
  
void ledloop()
{
  EVERY_N_MILLISECONDS(15)
  {
    if(fadeBrightness)
    {
      if(curBrightness != otwBrightness)
      {
        if(curBrightness >= otwBrightness)
        {
          curBrightness--;
          FastLED.setBrightness(pgm_read_byte(&gamma8[map(curBrightness, 1, 255, 73, 255)]));
        }
        else if(curBrightness <= otwBrightness)
        {
          curBrightness++;
          FastLED.setBrightness(pgm_read_byte(&gamma8[map(curBrightness, 1, 255, 73, 255)]));
        }
        FastLEDshowESP32();
      }
      else
      {
        fadeBrightness = false;
      }
    }
  }

  EVERY_N_MILLISECONDS(5)
  {
    if(fadeColor)
    {
      if(curR != otwR)
      {
        if(curR >= otwR)
        {
          curR--;
        }
        else if(curR <= otwR)
        {
          curR++;
        }
      }
      
      if(curG != otwG)
      {
        if(curG >= otwG)
        {
          curG--;
        }
        else if(curG <= otwG)
        {
          curG++;
        }
      }
      
      if(curB != otwB)
      {
        if(curB >= otwB)
        {
          curB--;
        }
        else if(curB <= otwB)
        {
          curB++;
        }
      }
      setColor(curR, curG, curB);
      if(curR == otwR && curG == otwG && curB == otwB)
      {
        fadeColor = false;
      }
    }
  }
}
