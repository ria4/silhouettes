#include "FastLED.h"
#include "IRremote.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    60

#define BUTTON_PIN    2
#define IRRCV_PIN    11
#define STRIP_PIN     6

#define INIT_BRIGHTNESS               15
#define FRAMES_PER_SECOND_MODES       4
#define INIT_FPS_IDX                  1

const int fps_arr[FRAMES_PER_SECOND_MODES] = { 50, 120, 1000, 10000 };

int brightness;
int fps_idx;
int randHue;
boolean pause = false;

IRrecv irrecv(IRRCV_PIN);
decode_results results;

// Define the array of leds
CRGB leds[NUM_LEDS];


void setup() {
  delay(1000); // 3 second delay for recovery
  Serial.begin(9600);
  Serial.println("resetting");
  
  FastLED.addLeds<LED_TYPE,STRIP_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  brightness = INIT_BRIGHTNESS;
  fps_idx = INIT_FPS_IDX;
  FastLED.setBrightness(brightness);
  
  irrecv.enableIRIn();
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, splash, sinelon, bpm, juggle, confetti };
SimplePatternList gPatterns = { rainbow, juggle, confetti };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns


void fadeall() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(192);
  }
}

void checkIRSignal()
{
  if (irrecv.decode(&results)) {
    switch(results.value) {
      
      case 0xFFE01F:  
      case 0xF076C13B:
        if (brightness >= 5) {
          brightness -= 5;
        }
        FastLED.setBrightness(brightness);
        break;
      
      case 0xFFA857:  
      case 0xA3C8EDDB:
        if (brightness <= 250) {
          brightness += 5;
        }
        FastLED.setBrightness(brightness);
        break;
        
      case 0xFFA25D:  
      case 0xE318261B:
        prevPattern();
        break;
        
      case 0xFFE21D:  
      case 0xEE886D7F:
        nextPattern();
        break;
        
      case 0xFF22DD:  
      case 0x52A3D41F:
        if (fps_idx > 0) { fps_idx -= 1; }     // no loop here, there might be a long FastLED.delay
        break;
  
      case 0xFF02FD:  
      case 0xD7E84B1B:
        if (fps_idx < FRAMES_PER_SECOND_MODES - 1) { fps_idx += 1; };
        break;
  
      case 0xFFC23D:  
      case 0x20FE4DBB:
        pause = !pause;
        break;
        
      default:
        Serial.println(results.value, HEX);
        
    }
    delay(500);
    irrecv.resume();
  }
}  

void loop()
{
  if (!pause) {
    gPatterns[gCurrentPatternNumber]();

    // do some periodic updates
    EVERY_N_MILLISECONDS( 30 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }
  
  while (!irrecv.isIdle());
  checkIRSignal();
  
  if (!pause) {
    FastLED.show();
    FastLED.delay(1000/fps_arr[fps_idx]);
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void prevPattern() {
  if (gCurrentPatternNumber == 0) { gCurrentPatternNumber = ARRAY_SIZE(gPatterns) - 1; }
  else { gCurrentPatternNumber -= 1; }
}
  
void nextPattern() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void spark() 
{
  //randHue = random8();
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(gHue, 255, 255);
    }
  FastLED.show();
  fadeall();
  delay(50);

  for(int i = 0; i < 100; i++) {
    FastLED.show(); 
    fadeall();
    delay(50);
  }  
}

void rainbow() 
{
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

/*
void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    switch (stripsState) {
      case 0: {leds[ random16(NUM_LEDS) ] += CRGB::White; ledsb[ random16(NUM_LEDS) ] += CRGB::White; break;}
      case 1: {leds[ random16(NUM_LEDS) ] += CRGB::White; break;}
      case 2: {ledsb[ random16(NUM_LEDS) ] += CRGB::White; break;}
    }
  }
}
*/

void splash() 
{
  // random colored splashes that fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 1);
  int test = random8();
  if (test > 253) {
    int pos = random16(NUM_LEDS);
    int colorb = random8();
    for( int i = max(0, pos-10); i < min(NUM_LEDS, pos+10); i++) {
      int theta = (pos-i) * 255 / 40;
      int x = cos8(theta);
      leds[i] = CHSV( gHue, 200, x);
    }
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int test = random8();
  if (test > 248) {
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( gHue + random8(64), 200, 255);
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16( 2, 0, 1.2*NUM_LEDS-1 ) - (0.1*NUM_LEDS);
  if ((pos >= 0) & (pos < NUM_LEDS)) {
    leds[pos] += CHSV(gHue, 255, 192);
  }
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 8;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*7));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    int pulse = (i+13)/3;
    leds[beatsin16( pulse, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

