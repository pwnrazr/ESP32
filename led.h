#include "FastLED.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    25
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    45
CRGB leds[NUM_LEDS];

#define FRAMES_PER_SECOND  120

// -- The core to run FastLED.show()
#define FASTLED_SHOW_CORE 1

#define START_COLOR 8900346
#define START_BRIGHTNESS 20

// -- Task handles for use in the notifications
static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

unsigned long rgbval = START_COLOR;
bool ledState = true;

// Brightness fade
byte trueBrightness = START_BRIGHTNESS;
int brightness = START_BRIGHTNESS;
byte otwBrightness; // Received brightness to fade to
boolean fadeBrightness = false;

// Color fade
byte curR;
byte curG;
byte curB;

byte otwR;
byte otwG;
byte otwB;

boolean fadeColor = false;

/** show() for ESP32
 *  Call this function instead of FastLED.show(). It signals core 0 to issue a show, 
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
 *  This function runs on core 0 and just waits for requests to call FastLED.show()
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
  //delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(trueBrightness);

  int core = xPortGetCoreID();
  //Serial.print("Main code running on core ");
  //Serial.println(core);

  // -- Create the FastLED show task
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = rgbval;
  }
  FastLEDshowESP32();
}
  
void ledloop()
{
  EVERY_N_MILLISECONDS(15)
  {
    if(fadeBrightness)
    {
      if(brightness != otwBrightness)
      {
        if(brightness > otwBrightness)
        {
          brightness--;
          FastLED.setBrightness(brightness);
        }
        else if(brightness < otwBrightness)
        {
          brightness++;
          FastLED.setBrightness(brightness);
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
      if(curR != otwR || curG != otwG || curB != otwG)
      {
        if(curR > otwR)
        {
          curR--;
        }
        else if(curR < otwR)
        {
          curR++;
        }
        if(curG > otwG)
        {
          curG--;
        }
        else if(curG < otwG)
        {
          curG++;
        }
        if(curB > otwB)
        {
          curB--;
        }
        else if(curB < otwB)
        {
          curB++;
        }
        setColor(curR, curG, curB);
      }
      else
      {
        fadeColor = false;
      }
    }
  }
}
