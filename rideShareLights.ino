// Use if you want to force the software SPI subsystem to be used for some reason (generally, you don't)
// #define FASTLED_FORCE_SOFTWARE_SPI
// Use if you want to force non-accelerated pin access (hint: you really don't, it breaks lots of things)
// #define FASTLED_FORCE_SOFTWARE_SPI
// #define FASTLED_FORCE_SOFTWARE_PINS
#include <FastLED.h>
#include <time.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#define redpin 3
#define greenpin 5
#define bluepin 6

#define commonAnode true

byte gammatable[256];
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);


// How many leds are in the strip?
#define NUM_LEDS 150

// Data pin that led data will be written out over
#define DATA_PIN 5

// Clock pin only needed for SPI based chipsets when not using hardware SPI
//#define CLOCK_PIN 8

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];
CRGB thisColor = CRGB::White;
CRGBPalette16 currentPalette = RainbowColors_p;
TBlendType    currentBlending = LINEARBLEND;
#define UPDATES_PER_SECOND 100


float defILightHueMin = 200.0;
float defILightHueMax = 290.0;
bool lightsOn = true;
bool approachingPassenger = false;
int brightnessKnobPin = 0;
double lightModeFader = 0.0;
double lightModeFadeRate = 0.01;

CRGB aColor = CRGB::White;


typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static hsv   rgb2hsv(rgb in);
static rgb   hsv2rgb(hsv in);
rgb fromLightSensor;
hsv lightSensorHue;
hsv currentColor;
int timer = 0;
int modeChangeWaitTime = 40;
int msPerDegreeChange;
char state = 'x';


float hueShiftRate = 2.0;
float sensorRed;
float sensorGreen;
float sensorBlue;
double brightnessKnob;

//Function declarations
void setAllLEDs(CRGB color);
void FillLEDsFromPaletteColors( uint8_t colorIndex);
CRGB getCRGBFromRgb(rgb in);
rgb getRgbFromCRGB(CRGB in);
void FillLEDsFromPaletteColors( uint8_t colorIndex);
void transitionBetweenColors(CRGB color1, CRGB color2, int transitionTime);
hsv rgb2hsv(rgb in);
rgb hsv2rgb(hsv in);
double getPercentComplete(int number, int topNumber);
static uint8_t startIndex = 1;


// This function sets up the ledsand tells the controller about them
void setup() 
{
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
    Serial.begin(9600);
    delay(2000);

      // Uncomment one of the following lines for your leds arrangement.
      // FastLED.addLeds<TM1803, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<TM1804, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<TM1809, DATA_PIN, RGB>(leds, NUM_LEDS);
      FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
      // FastLED.addLeds<APA104, DATA_PIN>(leds, NUM_LEDS);
      // FastLED.addLeds<WS2811_400, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<GW6205, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<GW6205_400, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<UCS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<UCS1903B, DATA_PIN, RGB>(leds, NUM_LEDS);

      // FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<SM16716, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<LPD8806, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<P9813, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<APA102, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<DOTSTAR, RGB>(leds, NUM_LEDS);
      
      // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      //Set all LEDs to white for two seconds to confirm code upload
      setAllLEDs(CRGB::White);
      delay(2000);
        // use these three pins to drive an LED
#if defined(ARDUINO_ARCH_ESP32)
  ledcAttachPin(redpin, 1);
  ledcSetup(1, 12000, 8);
  ledcAttachPin(greenpin, 2);
  ledcSetup(2, 12000, 8);
  ledcAttachPin(bluepin, 3);
  ledcSetup(3, 12000, 8);
#else
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
#endif

  // thanks PhilB for this gamma table!
  // it helps convert RGB colors to what humans see
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    if (commonAnode) {
      gammatable[i] = 255 - x;
    } else {
      gammatable[i] = x;
    }
  }
}

// This function runs over and over, and is where you do the magic to light
// your leds.
void loop() {
    if (Serial.available() > 0)
    {
      state = Serial.read();
      Serial.print("State: "); Serial.print(state); Serial.print("\n");
    }
    
     brightnessKnob = (double)(1023 - analogRead(brightnessKnobPin))/1023.0;
    /* Serial.print("Light mode transition state: "); Serial.print(lightModeFader); Serial.print(" startIndex: "); Serial.print(startIndex); 
     Serial.print(" approachingPassenger: "); Serial.print(approachingPassenger);
     Serial.print("\n");
     Serial.print("aColor info:\n"); Serial.print("R: "); Serial.print(aColor.r); Serial.print(" G: "); Serial.print(aColor.g); Serial.print(" B: "); Serial.print(aColor.b);*/
     if (lightsOn)
     {

      if ((approachingPassenger) && (lightModeFader < 1.0)) lightModeFader += lightModeFadeRate;
      else if ((!approachingPassenger) && (lightModeFader > 0.0)) lightModeFader -= lightModeFadeRate;

      if (lightModeFader < 0.0) lightModeFader = 0.0;
      else if (lightModeFader > 1.0) lightModeFader = 1.0;

      startIndex++;
      updateLEDs();
      FastLED.show();
     
      
      //Get RGB data from sensor
      tcs.getRGB(&sensorRed, &sensorGreen, &sensorBlue);

      //Pushing RGB data into an RGB object for conversion to HSV
      fromLightSensor.r = (double) sensorRed;
      fromLightSensor.g = (double) sensorGreen;
      fromLightSensor.b = (double) sensorBlue;
      //    Serial.print("R:");Serial.print(sensorRed);Serial.print(" G:");Serial.print(sensorGreen);Serial.print(" B:");Serial.print(sensorBlue);Serial.print("\n");

      //converting to HSV
      lightSensorHue = rgb2hsv(fromLightSensor);
      //Serial.print("lightSensorHue: ");Serial.print(lightSensorHue.h); Serial.print("\n");

      if (!iLightIsDefaultColor(lightSensorHue))
      {
        //Serial.print("Detected non-default color Timer: "); Serial.print(timer); Serial.print("\n");
        if (!approachingPassenger)
        {
          if (timer == modeChangeWaitTime)
          {
              if (!iLightIsDefaultColor(lightSensorHue))
              {
                Serial.print("Detected prolonged non-default color\n");
                approachingPassenger = true;
                //aColor = getCRGBFromRgb(fromLightSensor);
                //aColor = CRGB((int) fromLightSensor.r, (int) fromLightSensor.g, (int) fromLightSensor.b);
                //aColor = CRGB(hsv2rgb_spectrum(lightSensorHue, 50, 50));
                hsv color; color.h = lightSensorHue.h; color.s = 1.0; color.v = 0.5; 
                rgb colorRgb = hsv2rgb(color);
                aColor = getCRGBFromRgb(colorRgb);
                //hsv2rgb_rainbow(CHSV(lightSensorHue.h,255,175),aColor);
                Serial.print("fromLightSensor color info:\n");
                Serial.print("R: "); Serial.print(fromLightSensor.r); Serial.print(" G: "); Serial.print(fromLightSensor.g); Serial.print(" B: "); Serial.print(fromLightSensor.b); Serial.print("\n");

                Serial.print("Light sensor hue: "); Serial.print(lightSensorHue.h); Serial.print("\n");
                
                Serial.print("aColor color info:\n");
                Serial.print("R: "); Serial.print(aColor.r); Serial.print(" G: "); Serial.print(aColor.g); Serial.print(" B: "); Serial.print(aColor.b); Serial.print("\n");
                delay(1000);
                
              }
              else
              {
                timer = 0;
              }
        }
        else
        {
          timer++;
        }
      }  
      }
      else
      {
        timer = 0;
        startIndex = 1;
        approachingPassenger = false;
      }
      //Serial.print("aColor:\n");
      //Serial.print("R: "); Serial.print(aColor.r); Serial.print(" G: "); Serial.print(aColor.g); Serial.print(" B: "); Serial.print(aColor.b); Serial.print("\n");
      /*DEPRECATED CODE
      if ((lightSensorHue.h >= defILightHueMin) && (lightSensorHue.h <= defILightHueMax))
      {
       
        
        //Code for color shifting lights during the ride goes here
        if (!approachingPassenger)
        {
          // Serial.print("Not approaching passenger\n");
          /*Serial.print("startIndex: ");Serial.print(startIndex);Serial.print("\n");
          FillLEDsFromPaletteColors( startIndex);
          startIndex = startIndex + 1; // motion speed 

          FastLED.show();
          FastLED.delay(1000 / UPDATES_PER_SECOND);
           
          /*
          currentColor.h = currentColor.h + hueShiftRate;
          currentColor.v = brightness;
          rgb currentRGB = hsv2rgb(currentColor);
          Serial.print("r: "); Serial.print(currentRGB.r); Serial.print(" g: "); Serial.print(currentRGB.g); Serial.print(" b: "); Serial.print(currentRGB.b); Serial.print("\n");
          setAllLEDs(getCRGBFromRgb(currentRGB));
        }
        else
        {
          Serial.print("About to enter 'not approaching passenger'");
          approachingPassenger = false;
          fadeFromSingleColorToPallet(200);
          
          static uint8_t startIndex = 0;
          startIndex = startIndex + 1; /* motion speed 
        
          FillLEDsFromPaletteColors( startIndex);
    
          FastLED.show();
          FastLED.delay(1000 / UPDATES_PER_SECOND);
        }
      }
      else
      {
        if (!approachingPassenger)
        {
          
          Serial.print("Approaching passenger\n");
          delay(1000);
          tcs.getRGB(&sensorRed, &sensorGreen, &sensorBlue);
          Serial.print("New color read");
          CRGB newColor = CRGB((int)sensorRed,(int)sensorGreen,(int)sensorBlue);
          rgb currentRGB = hsv2rgb(currentColor);
          fadeFromPaletteToSingleColor(newColor,200);
          //transitionBetweenColors(CRGB(getCRGBFromRgb(currentRGB)), newColor,100);
          currentColor = rgb2hsv(getRgbFromCRGB(newColor));
          approachingPassenger = true;
        }
        
      }*/
     }
     else
     {
     setAllLEDs(CRGB::Black);
     }
}

void updateLEDs()
{
  uint8_t index = startIndex;
  if (lightModeFader != 1.0)
  {
    for (int a = 0; a < NUM_LEDS; a++)
    {
      leds[a] = translateColorForLED(combineColors(ColorFromPalette( currentPalette, index, 1.0, currentBlending),aColor,lightModeFader));
      index += 3;
    }
  } 
}

CRGB translateColorForLED(CRGB inputColor)
{
  return CRGB(inputColor.g, inputColor.r, inputColor.b);
}
void setAllLEDs(CRGB color)
{
  for (int thisLed = 0; thisLed < NUM_LEDS; thisLed = thisLed + 1)
  {
    leds[thisLed] = CRGB(color.g, color.r, color.b);
  }
  FastLED.show();
}
void transitionBetweenColors(CRGB color1, CRGB color2, int transitionTime)
{
  double transitionAmount = 0.0;
  int red = 0, green = 0, blue = 0;
  int redDiff = color2.red - color1.red, greenDiff = color2.green - color1.green, blueDiff = color2.blue - color1.blue;
  for (int a = 0; a < transitionTime; a++)
  {
    transitionAmount = (double)a / (double) transitionTime;
    red = color1.r + (int)(transitionAmount * (double)redDiff);
    green = color1.g + (int)(transitionAmount * (double)greenDiff);
    blue = color1.b +  (int)(transitionAmount * (double)blueDiff);
    setAllLEDs(CRGB(red,green,blue));
    FastLED.show();
    delay(10);
  }
}
bool iLightIsDefaultColor(hsv lightSensorHue)
{
  return ((lightSensorHue.h >= defILightHueMin) && (lightSensorHue.h <= defILightHueMax));
}
hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}
rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}
CRGB getCRGBFromRgb(rgb in)
{
  CRGB out;
  out.r = (int)(255.0 * in.r);
  out.g = (int)(255.0 * in.g);
  out.b = (int)(255.0 * in.b);
  return out;
}
rgb getRgbFromCRGB(CRGB in)
{
  rgb out;
  out.r = (double)in.r / 255.0;
  out.g = (double)in.g / 255.0;
  out.b = (double)in.b / 255.0;
  return out;
}
void FillLEDsFromPaletteColors( uint8_t index)
{
    uint8_t brightness = (uint8_t)(brightnessKnob * 255.0);
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, index, brightness, currentBlending);
        index += 3;
    }
}
void FillArrayFromPaletteColors(CRGB& cArray, uint8_t index)
{
  FillLEDsFromPaletteColors(index);
  
    uint8_t brightness = (uint8_t)(brightnessKnob * 255.0);
    for( int i = 0; i < NUM_LEDS; i++) {
        cArray[i] = leds[i];//ColorFromPalette( currentPalette, index, brightness, currentBlending);
        index += 3;
    }
}
/*
void fadeFromSingleColorToPallet(int transLength)
{
  Serial.print("Fading to pallet");
  CRGB palletColors[NUM_LEDS];
  CRGB originalColor = leds[0];
  FillArrayFromPaletteColors(&palletColors, startIndex);
   double percentComplete;
  for (int i = 0; i < transLength; i++)
  {
    percentComplete = getPercentComplete(i, transLength);
    for( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = combineColors(originalColor,palletColors[i],1.0);
    }
   Serial.print(percentComplete); Serial.print("\n");
   FastLED.show();
  }
  Serial.print("Start index upon leaving fadeFromSingleColorToPallet: "); Serial.print(startIndex); Serial.print("\n");
}
*/
/*
void fadeFromPaletteToSingleColor(CRGB singleColor, int transLength)
{
  Serial.print("fadeFromPaletteToSingleColor entered\n");
  CRGB originalColors[NUM_LEDS];
  for (int i = 0; i < NUM_LEDS; i++)
  {
  originalColors[i] = translateColorForLED(leds[i]);
  }

  double percentComplete;

  for (int i = 0; i < transLength; i++)
  {
    percentComplete = getPercentComplete(i, transLength);
    for( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = translateColorForLED(combineColors(originalColors[i],singleColor,percentComplete));
    }
    FastLED.show();
  }
  
  
}
*/
double getPercentComplete(int number, int topNumber)
{
  return ((double)number/(double)topNumber);  
}
CRGB combineColors(CRGB color1, CRGB color2, double percentFade)
{
  int redDiff = color2.red - color1.red, greenDiff = color2.green - color1.green, blueDiff = color2.blue - color1.blue;
  CRGB combColor;

    combColor.r = color1.r + (int)(percentFade * (double)redDiff);
    combColor.g = color1.g + (int)(percentFade * (double)greenDiff);
    combColor.b = color1.b +  (int)(percentFade * (double)blueDiff);
     
  return combColor;
}
