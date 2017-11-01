#include "FastLED.h"
#include "IRremote.h"
#include "SD.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    144

#define BUTTON_PIN    2
#define IRRCV_PIN     9
#define STRIP_PIN     6

#define INIT_BRIGHTNESS             15
#define FRAMES_PER_SECOND_MODES     6
#define INIT_FPS_IDX                2
#define MAX_CHANNELS_IDX            1		// let's try not to compute a log10...
#define CHANGE_SIG_LENGTH           200

// the frame rate for 144 leds will cap by itself
const int fps_arr[FRAMES_PER_SECOND_MODES] = { 2, 12, 50, 200, 1000, 5000 };

uint8_t brightness;
uint8_t fps_idx;
uint8_t pos_shift = 0;
uint8_t pos_shifted;
uint8_t hue_shift = 0;

IRrecv irrecv(IRRCV_PIN);
decode_results results;
boolean pause = false;
boolean mirror = false;

CRGB leds[NUM_LEDS];


#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, splash, sinelon, bpm, juggle, confetti };
SimplePatternList gPatterns = { read_sd, point, line, wave, gradient, rainbow };
uint8_t channels_nbr = ARRAY_SIZE(gPatterns);

uint8_t pattern_idx = 0;
uint8_t pattern_idx_tmp;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

char buffer[3];
uint8_t curr_char_idx = 0;

File silhouette;
char silhouette_name[7];
int sd_idx;
uint8_t sd_params;
bool sd_loop;
int sd_width;
int sd_frames_nbr;
int sd_frame_idx;
uint8_t r, g, b;


void fadeall() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(192);
  }
}


void setup() {
  delay(1000);
  //Serial.begin(9600);
  //Serial.println(F("Resetting..."));

  FastLED.addLeds<LED_TYPE,STRIP_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  brightness = INIT_BRIGHTNESS;
  fps_idx = INIT_FPS_IDX;
  FastLED.setBrightness(brightness);

  irrecv.enableIRIn();

  //Serial.print(F("Trying to init SD card... "));
  pinMode(10, OUTPUT);
  if (!SD.begin(10)) {
    //Serial.println(F("init failed!"));
    return;
  }
  //Serial.println(F("OK!"));
}


void addToBuffer(char c) {
  if ( curr_char_idx < 3 ) {
    buffer[curr_char_idx++] = c;
  }
}

void flushChannelBuffer() {
  buffer[curr_char_idx] = 0;
  pattern_idx_tmp = atoi(buffer);       // note that no buffer raises a plain 0
  if (pattern_idx_tmp < channels_nbr) {
    pattern_idx = pattern_idx_tmp;
    if (pattern_idx == 0) { signal(0); } else { signal(255); }
  }
  else { if (set_sd_silhouette(pattern_idx_tmp)) { pattern_idx = 0; signal(0); } }
  curr_char_idx = 0;
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
        if (brightness >= 5) { brightness -= 5; }
        FastLED.setBrightness(brightness); break;

      case 0xFFA857:
      case 0xA3C8EDDB:
        if (brightness <= 250) { brightness += 5; }
        FastLED.setBrightness(brightness); break;

      case 0xFFA25D:
      case 0xE318261B:
        prevPattern(); break;

      case 0xFFE21D:
      case 0xEE886D7F:
        nextPattern(); break;

      case 0xFF629D:
      case 0x511DBB:
        flushChannelBuffer(); break;

      case 0xFF9867:
      case 0x97483BFB:
        flushPosShiftBuffer(); break;

      case 0xFFB04F:
      case 0xF0C41643:
        flushHueShiftBuffer(); break;

      case 0xFF6897:
      case 0xC101E57B:
        addToBuffer('0'); break;

      case 0xFF30CF:
      case 0x9716BE3F:
        addToBuffer('1'); break;

      case 0xFF18E7:
      case 0x3D9AE3F7:
        addToBuffer('2'); break;

      case 0xFF7A85:
      case 0x6182021B:
        addToBuffer('3'); break;

      case 0xFF10EF:
      case 0x8C22657B:
        addToBuffer('4'); break;

      case 0xFF38C7:
      case 0x488F3CBB:
        addToBuffer('5'); break;

      case 0xFF5AA5:
      case 0x449E79F:
        addToBuffer('6'); break;

      case 0xFF42BD:
      case 0x32C6FDF7:
        addToBuffer('7'); break;

      case 0xFF4AB5:
      case 0x1BC0157B:
        addToBuffer('8'); break;

      case 0xFF52AD:
      case 0x3EC3FC1B:
        addToBuffer('9'); break;

      case 0xFF22DD:
      case 0x52A3D41F:
        if (fps_idx > 0) { fps_idx -= 1; } break;     // no loop here, there might be a long FastLED.delay

      case 0xFF02FD:
      case 0xD7E84B1B:
        if (fps_idx < FRAMES_PER_SECOND_MODES - 1) { fps_idx += 1; }; break;

      case 0xFF906F:
      case 0xE5CFBD7F:
        mirror = !mirror; break;

      case 0xFFC23D:
      case 0x20FE4DBB:
        FastLED.clear(); FastLED.show(); pause = !pause; break;

      default:
        break; //Serial.println(results.value, HEX);

    }
    delay(200);
    irrecv.resume();
  }
}

void loop()
{
  if (!pause) {
    gPatterns[pattern_idx]();
    EVERY_N_MILLISECONDS( 30 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

  while (!irrecv.isIdle());
  checkIRSignal();

  if (!pause) {
    FastLED.show();
    // long delays with FastLED cause long periods when the IRremote cannot catch interrupts
    //FastLED.delay(1000/fps_arr[fps_idx]);
    delay(1000/fps_arr[fps_idx]);
  }
}

// brief signal when a new pattern is loaded: blue for SD, red for programmatic
void signal(uint8_t rb) {
  FastLED.clear();
  leds[0] = CRGB(rb, 0, 255-rb);
  FastLED.show();
  delay(CHANGE_SIG_LENGTH);
  leds[0] = CRGB(0, 0, 0);
  FastLED.show();
  pause = true;
}

void prevPattern() {
  if (pattern_idx == 0) {
    if (set_sd_silhouette(sd_idx - 1)) { signal(0); return; }
    else { pattern_idx = channels_nbr - 1; } }
  else {
    if (pattern_idx == 1) {
      if (set_sd_silhouette(sd_idx)) { pattern_idx = 0; signal(0); return; }
      else { pattern_idx = channels_nbr - 1; } }
    else { pattern_idx--; } }
  signal(255);
}

void nextPattern() {
  if (pattern_idx == 0) {
    if (set_sd_silhouette(sd_idx + 1)) { signal(0); return; }
    else { pattern_idx = 1; } }
  else {
    if (pattern_idx == channels_nbr - 1) {
      if (set_sd_silhouette(sd_idx)) { pattern_idx = 0; signal(0); return; }
      else { pattern_idx = 0; } }
    else { pattern_idx++; } }
  signal(255);
}

/*
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
*/

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

bool set_sd_silhouette(int n) {
  if (silhouette) { silhouette.close(); }
  sprintf(silhouette_name, "%03d", n);
  strcpy(silhouette_name + 3, ".SIL");
  silhouette = SD.open(silhouette_name, FILE_READ);
  if (silhouette) {
    //Serial.print(F("Opened "));
    //Serial.println(silhouette_name);
    sd_frame_idx = 0;
    sd_idx = n;
    sd_params = silhouette.read();
    sd_loop = (sd_params >> 7);
    sd_width = (silhouette.read() << 8) + silhouette.read();
    sd_frames_nbr = (silhouette.read() << 8) + silhouette.read();
    return true;
  } else {
    //Serial.print(F("Failed to open "));
    //Serial.println(silhouette_name);
    return false;
  }
}

void read_sd()
{
  if ((sd_idx == 0) || (!silhouette && !set_sd_silhouette(sd_idx))) {
    pattern_idx = 1;
    return;
  }

  //if (silhouette) { return; }

  //Serial.print(F("Available from SD: "));
  //Serial.println(silhouette.available());
  if (silhouette && silhouette.available()) {
    //Serial.print(F("Remaining SRAM: "));
    //Serial.println(freeRam());
    for (int j=pos_shift; j<sd_width+pos_shift; j++) {
      r = silhouette.read();
      g = silhouette.read();
      b = silhouette.read();
      pos_shifted = j % NUM_LEDS;
      if (mirror) { pos_shifted = NUM_LEDS - 1 - pos_shifted; }
      leds[pos_shifted] = CRGB(r, g, b);		// note that CRGB class would first retrieve b, then g, then r
    }
    sd_frame_idx++;
  }

  if (sd_frame_idx == sd_frames_nbr) {
    sd_frame_idx = 0;
    if (sd_loop) {
      silhouette.seek(5);
    } else {
      silhouette.close();
      FastLED.clear();
      FastLED.show();
      pause = true;
    }
  }
}

void black()
{
  FastLED.clear();
}

void point()
{
  pos_shifted = (1 + pos_shift) % NUM_LEDS;
  leds[pos_shifted] = CHSV(hue_shift, 255, 255);
}

void line()
{
  pos_shifted = pos_shift % NUM_LEDS;
  fill_solid(&(leds[pos_shifted]), NUM_LEDS - pos_shifted, CHSV(hue_shift, 255, 255));
}

void wave()
{
  uint8_t BeatsPerMinute = fps_arr[fps_idx];
  uint8_t beat = beatsin8( BeatsPerMinute, 0, 255);
  fill_solid(leds, NUM_LEDS, CHSV(hue_shift, 255, beat));
}

void gradient()
{
  uint8_t BeatsPerMinute = fps_arr[fps_idx];
  uint8_t beat = beatsin8( BeatsPerMinute, 0, 100);
  fill_gradient(leds, NUM_LEDS, CHSV(hue_shift, 255, 255), CHSV(hue_shift+beat, 255, 255));
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
*/

