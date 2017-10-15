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
#define IRRCV_PIN     9
#define STRIP_PIN     6

#define INIT_BRIGHTNESS             15
#define FRAMES_PER_SECOND_MODES     4
#define INIT_FPS_IDX                1
#define MAX_CHANNELS_IDX            1		// let's try not to compute a log10...

const int fps_arr[FRAMES_PER_SECOND_MODES] = { 50, 120, 1000, 10000 };

uint8_t brightness;
uint8_t fps_idx;
uint8_t pos_shift = 0;
uint8_t hue_shift = 0;

IRrecv irrecv(IRRCV_PIN);
decode_results results;
boolean pause = false;

CRGB leds[NUM_LEDS];


#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, splash, sinelon, bpm, juggle, confetti };
SimplePatternList gPatterns = { one, rainbow, juggle, confetti };
uint8_t channels_nbr = ARRAY_SIZE(gPatterns);

uint8_t gCurrentPatternNumber = 0;
uint8_t gCurrentPatternNumber_tmp;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

char buffer[3];
uint8_t curr_char_idx = 0;

typedef struct {
  uint8_t params;
  int frames_nbr;
  CRGB frames[][NUM_LEDS];
} silhouette;


void fadeall() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(192);
  }
}


void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println("resetting");
  
  FastLED.addLeds<LED_TYPE,STRIP_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  brightness = INIT_BRIGHTNESS;
  fps_idx = INIT_FPS_IDX;
  FastLED.setBrightness(brightness);
  
  irrecv.enableIRIn();
}


void addToBuffer(char c) {
  if ( curr_char_idx < 3 ) {
    buffer[curr_char_idx++] = c;
  }
}

void flushChannelBuffer() {
  buffer[curr_char_idx] = 0;
  gCurrentPatternNumber_tmp = atoi(buffer);
  if (gCurrentPatternNumber_tmp < channels_nbr) {
    gCurrentPatternNumber = gCurrentPatternNumber_tmp;
  }
  curr_char_idx = 0;
  FastLED.clear();
}

void flushPosShiftBuffer() {
  buffer[curr_char_idx] = 0;
  pos_shift = atoi(buffer);
  curr_char_idx = 0;
  FastLED.clear();
}

void flushHueShiftBuffer() {
  buffer[curr_char_idx] = 0;
  hue_shift = atoi(buffer);
  curr_char_idx = 0;
  FastLED.clear();
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
        
      case 0xFF629D:
      case 0x511DBB:
        flushChannelBuffer();
        break;
        
      case 0xFF9867:  
      case 0x97483BFB:
        flushPosShiftBuffer();
        break;

      case 0xFFB04F:  
      case 0xF0C41643:
        flushHueShiftBuffer();
        break;
        
      case 0xFF6897:
      case 0xC101E57B:
        addToBuffer('0');
        break;

      case 0xFF30CF:
      case 0x9716BE3F:
        addToBuffer('1');
        break;

      case 0xFF18E7:
      case 0x3D9AE3F7:
        addToBuffer('2');
        break;

      case 0xFF7A85:
      case 0x6182021B:
        addToBuffer('3');
        break;

      case 0xFF10EF:
      case 0x8C22657B:
        addToBuffer('4');
        break;

      case 0xFF38C7:
      case 0x488F3CBB:
        addToBuffer('5');
        break;

      case 0xFF5AA5:
      case 0x449E79F:
        addToBuffer('6');
        break;

      case 0xFF42BD:
      case 0x32C6FDF7:
        addToBuffer('7');
        break;

      case 0xFF4AB5:
      case 0x1BC0157B:
        addToBuffer('8');
        break;

      case 0xFF52AD:
      case 0x3EC3FC1B:
        addToBuffer('9');
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
    EVERY_N_MILLISECONDS( 30 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }
  
  while (!irrecv.isIdle());
  checkIRSignal();
  
  if (!pause) {
    FastLED.show();
    FastLED.delay(1000/fps_arr[fps_idx]);
  }
}

void prevPattern() {
  FastLED.clear();
  if (gCurrentPatternNumber == 0) { gCurrentPatternNumber = channels_nbr - 1; }
  else { gCurrentPatternNumber -= 1; }
}
  
void nextPattern() {
  FastLED.clear();
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % channels_nbr;
}

void spark() 
{
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

void one()
{
  uint8_t pos = (0 + pos_shift) % NUM_LEDS;
  leds[pos] = CHSV(hue_shift, 255, 255);
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
    leds[ random16(NUM_LEDS) ] += CRGB::White;
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

