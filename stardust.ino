//#define FASTLED_FORCE_SOFTWARE_SPI
#include "FastLED.h"

#define PATTERN_CHANGE_TIME_MS 60000
#define STAR_TIME_MIN 10000
#define STAR_TIME_MAX 60000
#define STAR_SPEED_MS 4

#define LED_PIN 13
#define MAX_PATTERN 3

// DotStar strips
#define BENCH_LED_TYPE APA102
#define BENCH_COLOR_ORDER BGR
#define LOWER_DATA_PIN  21
#define LOWER_CLOCK_PIN 20
#define NUM_LOWER_BENCH_LEDS 180

#define UPPER_DATA_PIN  7
#define UPPER_CLOCK_PIN 14
#define NUM_UPPER_BENCH_LEDS 144
//#define NUM_LEDS (NUM_UPPER_BENCH_LEDS + NUM_LOWER_BENCH_LEDS)

#define NUM_TUBE_LEDS 238

// Neopixel Portals
#define PORTAL_LED_TYPE WS2812
#define PORTAL_COLOR_ORDER GRB
#define PORTAL_DATA_PIN 2
#define NUM_PORTALS   18
#define LEDS_PER_PORTAL 24
#define COLUMNS_PER_PORTAL 13
#define NUM_PORTAL_LEDS (NUM_PORTALS * LEDS_PER_PORTAL)
#define PORTAL_STEP 5
#define PORTAL_TIMING 140

// Dotstar White Shooting Stars
#define STAR_LED_TYPE APA102
#define STAR_DATA_PIN  5
#define STAR_CLOCK_PIN 6
#define STAR_COLOR_ORDER BGR
#define NUM_STAR_LEDS 360 // 60 * 6 METERS

// "Neopixel" illuminators
#define FIBER_LED_TYPE WS2812
#define FIBER_DATA_PIN 8
#define FIBER_COLOR_ORDER GBR
#define NUM_FIBER_LEDS 9

CRGB upperLeds[NUM_UPPER_BENCH_LEDS + NUM_TUBE_LEDS];
CRGB lowerLeds[NUM_LOWER_BENCH_LEDS];
CRGB portalLeds[NUM_PORTAL_LEDS];
CRGB starLeds[NUM_STAR_LEDS];
CRGB fiberLeds[NUM_FIBER_LEDS];

byte colorIndex = 0;
byte pattern = 0;
byte switchIndex = 0;
byte activePortal = 0;
boolean portalClockwise = true;
int starIndex = -1;
short portalIndex = 0;

void setup() {
  randomSeed(analogRead(12));
  for (byte pin = 0; pin < 27; pin++) {
    pinMode(LED_PIN, OUTPUT);
  }
  Serial.end();
  //Serial.begin(9600);
  // Setting unused pins to OUPUT to lower power usage.
  pinMode(LED_PIN, OUTPUT);
  // Create LED strips
  FastLED.addLeds<BENCH_LED_TYPE, UPPER_DATA_PIN, UPPER_CLOCK_PIN, BENCH_COLOR_ORDER, DATA_RATE_MHZ(5)>(upperLeds, 0, NUM_UPPER_BENCH_LEDS + NUM_TUBE_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<BENCH_LED_TYPE, LOWER_DATA_PIN, LOWER_CLOCK_PIN, BENCH_COLOR_ORDER, DATA_RATE_MHZ(5)>(lowerLeds, 0, NUM_LOWER_BENCH_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<PORTAL_LED_TYPE, PORTAL_DATA_PIN, PORTAL_COLOR_ORDER>(portalLeds, 0, NUM_PORTAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<STAR_LED_TYPE, STAR_DATA_PIN, STAR_CLOCK_PIN, STAR_COLOR_ORDER, DATA_RATE_MHZ(5)>(starLeds, 0, NUM_STAR_LEDS);
  FastLED.addLeds<FIBER_LED_TYPE, FIBER_DATA_PIN, FIBER_COLOR_ORDER>(fiberLeds, 0, NUM_FIBER_LEDS).setCorrection(TypicalLEDStrip);

  fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, CRGB::Black);
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, CRGB::Black);
  fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, CRGB::Black);
  fill_solid(&(portalLeds[0]), NUM_PORTAL_LEDS, CRGB::Black);
  fill_solid(&(starLeds[0]), NUM_STAR_LEDS, CRGB::Black);
  fill_solid(&(fiberLeds[0]), NUM_FIBER_LEDS, CRGB::Black);
  FastLED.show();
  FastLED.delay(5000);

//  FastLED.setBrightness(2);
//  FastLED.showColor(CRGB::Red);
//  delay(500);
//  FastLED.showColor(CRGB::Green);
//  delay(500);
//  FastLED.showColor(CRGB::Blue);
//  delay(500);
//  FastLED.showColor(CRGB::Grey);
//  delay(500);
//  FastLED.setBrightness(255);

  // Set different outputs to different colors, so we can verify that
  // things are plug in correctly
  FastLED.setBrightness(15);
  fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, CRGB::Red);
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, CRGB::Green);
  fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, CRGB::Blue);
  fill_solid(&(portalLeds[0]), NUM_PORTAL_LEDS, CRGB::Yellow);
  fill_solid(&(starLeds[0]), NUM_STAR_LEDS, CRGB::Grey);
  fill_solid(&(fiberLeds[0]), NUM_FIBER_LEDS, CRGB::Purple);
  FastLED.show();

  FastLED.showColor(CRGB::Black);
  FastLED.setBrightness(255);

}

void showBase() {
  FastLED[0].showLeds(FastLED.getBrightness());
  FastLED[1].showLeds(FastLED.getBrightness());
  // FIBERS
  FastLED[4].showLeds(255);
}

void showPortal() {
  FastLED[2].showLeds();
}

void showStar() {
  FastLED[3].showLeds();
}

/**
   Get a rainbow color the same way that fill_rainbow does.
   Paige says these colors look different the the rainbow
   gradient
*/
CHSV rainbowColor(uint8_t hue) {
  return CHSV(hue, 255, 240);
}

void setPortalColumn(short column, CRGB color) {
  short realColumn = column;
  if (realColumn < 0) {
    realColumn += (NUM_PORTALS * COLUMNS_PER_PORTAL);
  } else if (realColumn >= NUM_PORTALS * COLUMNS_PER_PORTAL) {
    realColumn -= (NUM_PORTALS * COLUMNS_PER_PORTAL);
  }
  short puckNum = realColumn / COLUMNS_PER_PORTAL; // 0 - 18
  short puckColumn = realColumn % COLUMNS_PER_PORTAL; // 0 - 13
  portalLeds[(puckNum*LEDS_PER_PORTAL) + 18 - puckColumn] = color;
  if (puckColumn >= 1 && puckColumn <= 5) {
    portalLeds[(puckNum*LEDS_PER_PORTAL) + 18 + puckColumn] = color;
  } else if (puckColumn >= 6 && puckColumn <= 11) {
    portalLeds[(puckNum*LEDS_PER_PORTAL) + puckColumn - 6] = color;
  }
}
void loop() {
  EVERY_N_MILLISECONDS_I(starTimer, STAR_TIME_MIN) {
    if (starIndex < 0) {
      starIndex = 0;
    }
    starTimer.setPeriod(random(STAR_TIME_MIN, STAR_TIME_MAX));
  }

  EVERY_N_MILLISECONDS(STAR_SPEED_MS) {
    if (starIndex > -1) {
      if (starIndex > 0 || starIndex == NUM_STAR_LEDS) {
        starLeds[starIndex - 1] = CRGB::Black;
      }
      if (starIndex >= 0 && starIndex < NUM_STAR_LEDS) {
        starLeds[starIndex] = CRGB::White;
      }
      showStar();
      starIndex++;
      if (starIndex > NUM_STAR_LEDS) {
        starIndex = -1;
      }
    }
  }

  EVERY_N_MILLIS_I(baseTime, 45) {
    // fading between patterns
    if (switchIndex > 63) {
      int bright = map(switchIndex, 64, 127, 0, 255);
      FastLED.setBrightness(bright);
      if (switchIndex == 64) {
        portalClockwise = !portalClockwise;
        if (pattern == MAX_PATTERN) {
          pattern = 0;
        } else {
          pattern++;
        }
      }
      switchIndex--;
    } else if (switchIndex > 0) {
      int bright = map(switchIndex, 1, 63, 255, 0);
      FastLED.setBrightness(bright);
      switchIndex--;
    }

    CRGB baseColor = rainbowColor(colorIndex);
    for (byte i = 0; i < 3; i++) {
      fill_solid(&(fiberLeds[0*i]), NUM_FIBER_LEDS/3, rainbowColor(colorIndex + (85*i)));
    }    
    if (0 == pattern) {
      baseTime.setPeriod(45);
      fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, baseColor);
      fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, baseColor);
      fill_rainbow(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, colorIndex, 4);
    } else if (1 == pattern) {
      baseTime.setPeriod(240);
      fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, baseColor);
      for (byte i = 0; i < 18; i++) {
        CRGB nextColor = rainbowColor(colorIndex+i);
        //fill_rainbow(&(upperLeds[8 * i]), 8, colorIndex, 0);
        fill_solid(&(upperLeds[8 * i]), 8, nextColor);
        //fill_rainbow(&(lowerLeds[10 * i]), 10, colorIndex++, 0);
        fill_solid(&(lowerLeds[10 * i]), 10, nextColor);
      }
    } else if (2 == pattern) {
      baseTime.setPeriod(45);
      fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, baseColor);
      CRGB nextColor = rainbowColor(colorIndex + 128);
      fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, nextColor);
      fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, nextColor);
    } else {
      baseTime.setPeriod(45);
      fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS + NUM_TUBE_LEDS, baseColor);
      fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, baseColor);
    }
    showBase();
    colorIndex++;
  }

  EVERY_N_MILLISECONDS(PORTAL_TIMING) {
    CRGB full = rainbowColor(colorIndex);
    for (byte i = 0; i < PORTAL_STEP; i++) {
      setPortalColumn(portalIndex - i - 1, CRGB::Black);
      setPortalColumn(portalIndex + i + (COLUMNS_PER_PORTAL*2), CRGB::Black);
    }
    for (byte i = 0; i < (COLUMNS_PER_PORTAL*2); i++) {
      setPortalColumn(portalIndex + i, full);       
    }
    showPortal();
    if (portalClockwise) {
      portalIndex += PORTAL_STEP;      
      if (portalIndex >= NUM_PORTALS*COLUMNS_PER_PORTAL) {
        portalIndex -= NUM_PORTALS*COLUMNS_PER_PORTAL;
      }    
    } else {
      portalIndex -= PORTAL_STEP;
      if (portalIndex < 0) {
        portalIndex += NUM_PORTALS*COLUMNS_PER_PORTAL;
      }      
    }
  }

//  EVERY_N_MILLISECONDS(120) {
//    CRGB full = rainbowColor(colorIndex);
//    //CRGB dim = full;
//    //dim = -dim;
//    fill_solid(portalLeds, NUM_PORTAL_LEDS, CRGB::Black);
//    fill_solid(&portalLeds[LEDS_PER_PORTAL * activePortal], LEDS_PER_PORTAL, full);
//    //fill_rainbow(&portalLeds[LEDS_PER_PORTAL * activePortal], LEDS_PER_PORTAL, colorIndex, 0);
//    if (portalClockwise) {
//      activePortal++;
//      if (activePortal >= NUM_PORTALS) {
//        activePortal = 0;
//      }
//    } else {
//      if (activePortal == 0) {
//        activePortal = NUM_PORTALS - 1;
//      } else {
//        activePortal--;
//      }
//    }
//    showPortal();
//  }

  EVERY_N_MILLISECONDS(PATTERN_CHANGE_TIME_MS) {
    // set switchIndex to start pattern change;
    switchIndex = 127;
  }

  EVERY_N_MILLISECONDS(1000) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}
