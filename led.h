#include "FastLED.h"

FASTLED_USING_NAMESPACE

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  0,  0,  0,  0,  0,  0,  0,  7,  8,  8,  8,  9,  9,  9, 10,
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

#define START_COLOR 8900346 // 135, 206, 250
#define START_COLOR_R 135
#define START_COLOR_G 206
#define START_COLOR_B 250

#define START_BRIGHTNESS 20

unsigned long rgbval = START_COLOR;
bool ledState = true;

// Brightness fade
byte trueBrightness = START_BRIGHTNESS; // Actual set brightness for sync to HA and GoogleHome
byte curBrightness = START_BRIGHTNESS;   // Stores current brightness when fading
byte otwBrightness;                     // Received brightness to fade to
bool fadeBrightness = false;

// Color fade
byte curR = START_COLOR_R;
byte curG = START_COLOR_G;
byte curB = START_COLOR_B;

int stepsR;
int stepsG;
int stepsB;

unsigned int loopCount = 0;

bool fadeColor = false;

void setColor(byte inR, byte inG, byte inB)
{
  for (int i = 0; i < NUM_LEDS; i++) 
  {
    leds[i].red = inR;
    leds[i].green = inG;
    leds[i].blue = inB;
  }
  FastLED.show();
}

void ledsetup() {
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // LED brightness on startup
  FastLED.setBrightness(pgm_read_byte(&gamma8[map(START_BRIGHTNESS, 1, 255, 66, 255)]));
  
  setColor(curR, curG, curB); // Sets starting led color
}

int calculateSteps(byte colCur, byte colOtw)
{
  int stepus = colOtw - colCur;
  if(stepus){
    stepus = 1020/stepus;
  }
  return stepus;
}

int calculateVal(int step, int current, int i)
{
  if(step && i % step == 0)  // if step is non-zero
  {
    if(step > 0) 
    {
      current += 1;
    }
    else if(step < 0)
    {
      current -= 1;
    }
  }

  if (current > 255) {
        current = 255;
    }
    else if (current < 0) {
        current = 0;
    }
  
  return current;
}

void ledloop()
{
  EVERY_N_MILLISECONDS(10)
  {
    if(fadeBrightness)
    {
      if(curBrightness != otwBrightness)
      {
        if(curBrightness >= otwBrightness)
        {
          curBrightness--;
          FastLED.setBrightness(pgm_read_byte(&gamma8[map(curBrightness, 1, 255, 66, 255)]));
        }
        else if(curBrightness <= otwBrightness)
        {
          curBrightness++;
          FastLED.setBrightness(pgm_read_byte(&gamma8[map(curBrightness, 1, 255, 66, 255)]));
        }
        FastLED.show();
      }
      else
      {
        fadeBrightness = false;
      }
    }
  }

  EVERY_N_MILLISECONDS(1)
  {
    if(fadeColor)
    {
      if(loopCount <= 1020)
      {
        curR = calculateVal(stepsR, curR, loopCount);
        curG = calculateVal(stepsG, curG, loopCount);
        curB = calculateVal(stepsB, curB, loopCount);
        
        setColor(curR, curG, curB);
        loopCount++;
      }
      else
      {
        fadeColor = false;
      }
    }
  }
}
