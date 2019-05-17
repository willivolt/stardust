#include "Bounce2.h"
//#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

#define PATTERN_CHANGE_TIME_MS 60000
#define STAR_TIME_MIN 3 // Minutes
#define STAR_TIME_MAX 10 // Minutes
#define STAR_SPEED_MS 4

#define LED_PIN 13
#define MAX_PATTERN 3
#define PIR_PIN 23
#define BUTTON_PIN 22

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

#define NUM_TUBE_1 43
#define NUM_TUBE_2 45
#define NUM_TUBE_3 47
#define NUM_TUBE_4 43
#define NUM_TUBE_5 30
#define NUM_TUBE_6 30
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

CLEDController* upperBenchAndTubeController = NULL;
CLEDController* lowerBenchController = NULL;
CLEDController* portalController = NULL;
CLEDController* starController = NULL;
CLEDController* fiberController = NULL;
Bounce pushbutton = Bounce(BUTTON_PIN, 10);

byte colorIndex = 0;
byte pattern = 0;
byte switchIndex = 0;
byte activePortal = 0;
boolean portalClockwise = true;
int starIndex = -1;
short portalIndex = 0;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);
  randomSeed(analogRead(12));
  // To save power, set pins to output and stop serial.
  //  for (byte pin = 0; pin < 27; pin++) {
  //    pinMode(LED_PIN, OUTPUT);
  //  }
  Serial.end();
  //Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  // Create LED strips
  upperBenchAndTubeController = &FastLED.addLeds<BENCH_LED_TYPE, UPPER_DATA_PIN, UPPER_CLOCK_PIN, BENCH_COLOR_ORDER, DATA_RATE_MHZ(5)>(upperLeds, 0, NUM_UPPER_BENCH_LEDS + NUM_TUBE_LEDS).setCorrection(TypicalLEDStrip);
  lowerBenchController = &FastLED.addLeds<BENCH_LED_TYPE, LOWER_DATA_PIN, LOWER_CLOCK_PIN, BENCH_COLOR_ORDER, DATA_RATE_MHZ(5)>(lowerLeds, 0, NUM_LOWER_BENCH_LEDS).setCorrection(TypicalLEDStrip);
  portalController = &FastLED.addLeds<PORTAL_LED_TYPE, PORTAL_DATA_PIN, PORTAL_COLOR_ORDER>(portalLeds, 0, NUM_PORTAL_LEDS).setCorrection(TypicalLEDStrip);
  starController = &FastLED.addLeds<STAR_LED_TYPE, STAR_DATA_PIN, STAR_CLOCK_PIN, STAR_COLOR_ORDER, DATA_RATE_MHZ(5)>(starLeds, 0, NUM_STAR_LEDS);
  fiberController = &FastLED.addLeds<FIBER_LED_TYPE, FIBER_DATA_PIN, FIBER_COLOR_ORDER>(fiberLeds, 0, NUM_FIBER_LEDS).setCorrection(TypicalLEDStrip);

  FastLED.clear(true);
  FastLED.show();
  FastLED.delay(5000);

  // Set different outputs to different colors, so we can verify that
  // things are plug in correctly
  FastLED.setBrightness(20);
  fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, CRGB::Red);
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, CRGB::Green);
  fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, CRGB::Blue);
  fill_solid(&(portalLeds[0]), NUM_PORTAL_LEDS, CRGB::Yellow);
  fill_solid(&(starLeds[0]), NUM_STAR_LEDS, CRGB::Grey);
  fill_solid(&(fiberLeds[0]), NUM_FIBER_LEDS, CRGB::Purple);
  FastLED.show();
  FastLED.delay(5000);

  FastLED.clear(true);
  FastLED.show();
  FastLED.setBrightness(255);

}

/*
   Updates the bench, galactic tube and fiber optic illuminators
*/
void showBase() {
  upperBenchAndTubeController->showLeds(FastLED.getBrightness());
  lowerBenchController->showLeds(FastLED.getBrightness());
  fiberController->showLeds(255);
}

/*
   Updates the portals
*/
void showPortal() {
  portalController->showLeds();
}

/*
   Updates the shooting star
*/
void showStar() {
  starController->showLeds();
}

/*
   Get a rainbow color the same way that fill_rainbow does.
   Paige says these colors look different the the rainbow
   gradient
*/
CHSV rainbowColor(uint8_t hue) {
  return CHSV(hue, 255, 240);
}

/*
   Sets the color of a "column" within the portals.
   The portals are viewd as a collection of 234
   columns.  This function will wrap numbers outside
   the 0-233 range.
*/
void setPortalColumn(short column, CRGB color) {
  // normalize the column number
  short realColumn = column;
  if (realColumn < 0) {
    realColumn += (NUM_PORTALS * COLUMNS_PER_PORTAL);
  } else if (realColumn >= NUM_PORTALS * COLUMNS_PER_PORTAL) {
    realColumn -= (NUM_PORTALS * COLUMNS_PER_PORTAL);
  }
  // Figure out which puck and column we are in
  short puckNum = realColumn / COLUMNS_PER_PORTAL; // 0 - 18
  short puckColumn = realColumn % COLUMNS_PER_PORTAL; // 0 - 13
  // Set the top LED color
  portalLeds[(puckNum * LEDS_PER_PORTAL) + 18 - puckColumn] = color;
  // Set the bottom LED color
  if (puckColumn >= 1 && puckColumn <= 5) {
    portalLeds[(puckNum * LEDS_PER_PORTAL) + 18 + puckColumn] = color;
  } else if (puckColumn >= 6 && puckColumn <= 11) {
    portalLeds[(puckNum * LEDS_PER_PORTAL) + puckColumn - 6] = color;
  }
}

/*
   Set the tube to six differnet colors
*/
void rainbowTube(byte baseIndex) {
  unsigned short startIndex = 0;
  byte m_colorIndex = baseIndex;
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS + startIndex]), NUM_TUBE_1, rainbowColor(m_colorIndex));

  startIndex += NUM_TUBE_1;
  m_colorIndex += 42;
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS + startIndex]), NUM_TUBE_2, rainbowColor(m_colorIndex));

  startIndex += NUM_TUBE_2;
  m_colorIndex += 42;
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS + startIndex]), NUM_TUBE_3, rainbowColor(m_colorIndex));

  startIndex += NUM_TUBE_3;
  m_colorIndex += 42;
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS + startIndex]), NUM_TUBE_4, rainbowColor(m_colorIndex));

  startIndex += NUM_TUBE_4;
  m_colorIndex += 42;
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS + startIndex]), NUM_TUBE_5, rainbowColor(m_colorIndex));

  startIndex += NUM_TUBE_5;
  m_colorIndex += 42;
  fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS + startIndex]), NUM_TUBE_6, rainbowColor(m_colorIndex));
}

void loop() {
  // Activate the shooting star randomly in a period from 10 to 60 seconds
  EVERY_N_MINUTES_I(starTimer, STAR_TIME_MIN) {
    if (starIndex < 0) {
      starIndex = 0;
    }
    starTimer.setPeriod(random(STAR_TIME_MIN, STAR_TIME_MAX));
  }

  // Button activates shooting star
  if (pushbutton.update() && (starIndex < 0)) {
    if (pushbutton.fallingEdge()) {
      starIndex = 0;
    }
  }

  // If the shooting star is running, incriment to the next pixel
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

  // Base and tube pattern
  EVERY_N_MILLIS_I(baseTime, 45) {
    // fading between patterns
    if (switchIndex > 63) {
      int bright = map(switchIndex, 64, 127, 0, 255);
      FastLED.setBrightness(bright);
      if (switchIndex == 64) {
        //portalClockwise = !portalClockwise;
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
    if (0 == pattern) {
      baseTime.setPeriod(45);
      fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, baseColor);
      rainbowTube(colorIndex);
      fill_rainbow(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, colorIndex, 4);
      for (byte i = 0; i < NUM_FIBER_LEDS; i++) {
        fill_solid(&(fiberLeds[i]), 1, rainbowColor(colorIndex + (27 * i)));
      }
    } else if (1 == pattern) {
      baseTime.setPeriod(240);
      //fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, baseColor);
      rainbowTube(colorIndex);
      for (byte i = 0; i < 3; i++) {
        fill_solid(&(fiberLeds[i * 3]), NUM_FIBER_LEDS / 3, rainbowColor(colorIndex + (85 * i)));
      }
      for (byte i = 0; i < 18; i++) {
        CRGB nextColor = rainbowColor(colorIndex + i);
        fill_solid(&(upperLeds[8 * i]), 8, nextColor);
        fill_solid(&(lowerLeds[10 * i]), 10, nextColor);
      }
    } else if (2 == pattern) {
      baseTime.setPeriod(45);
      CRGB nextColor = rainbowColor(colorIndex + 128);
      fill_solid(&(fiberLeds[0]), NUM_FIBER_LEDS, baseColor);
      fill_solid(&(upperLeds[NUM_UPPER_BENCH_LEDS]), NUM_TUBE_LEDS, nextColor);
      fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS, baseColor);
      fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, nextColor);
    } else {
      // Everything the same color
      baseTime.setPeriod(45);
      fill_solid(&(fiberLeds[0]), NUM_FIBER_LEDS, baseColor);
      fill_solid(&(upperLeds[0]), NUM_UPPER_BENCH_LEDS + NUM_TUBE_LEDS, baseColor);
      fill_solid(&(lowerLeds[0]), NUM_LOWER_BENCH_LEDS, baseColor);
    }
    showBase();
    colorIndex++;
  }

  // Portal pattern
  EVERY_N_MILLISECONDS(PORTAL_TIMING) {
    CRGB full = rainbowColor(colorIndex);
    for (byte i = 0; i < PORTAL_STEP; i++) {
      setPortalColumn(portalIndex - i - 1, CRGB::Black);
      setPortalColumn(portalIndex + i + (COLUMNS_PER_PORTAL * 2), CRGB::Black);
    }
    if (0 == pattern) {
      for (byte i = 0; i < (COLUMNS_PER_PORTAL * 2); i++) {
        full = rainbowColor(colorIndex+(i*4));      
        setPortalColumn(portalIndex + i, full);
      }
    } else {
      for (byte i = 0; i < (COLUMNS_PER_PORTAL * 2); i++) {
        setPortalColumn(portalIndex + i, full);
      }
    }
    showPortal();
    if (portalClockwise) {
      portalIndex += PORTAL_STEP;
      if (portalIndex >= NUM_PORTALS * COLUMNS_PER_PORTAL) {
        portalIndex -= NUM_PORTALS * COLUMNS_PER_PORTAL;
      }
    } else {
      portalIndex -= PORTAL_STEP;
      if (portalIndex < 0) {
        portalIndex += NUM_PORTALS * COLUMNS_PER_PORTAL;
      }
    }
  }

  //  // Old portal pattern
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

  // Triger a change in the base and tube pattern
  EVERY_N_MILLISECONDS(PATTERN_CHANGE_TIME_MS) {
    // set switchIndex to start pattern change;
    switchIndex = 127;
  }

  portalClockwise = (LOW == digitalRead(PIR_PIN));

  // Blink the Teensy LED so that we know it is alive
  //  EVERY_N_MILLISECONDS(1000) {
  //    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  //  }
  digitalWrite(LED_PIN, digitalRead(PIR_PIN));
}
