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
#define CHANGE_SIG_LENGTH           50

// the frame rate for 144 leds will cap by itself
const int fps_arr[FRAMES_PER_SECOND_MODES] = { 2, 12, 50, 200, 1000, 5000 };

uint8_t brightness;
uint8_t fps_idx;
uint8_t pos_shift = 0;
uint8_t hue_shift = 0;
uint8_t fade_size = 0;

IRrecv irrecv(IRRCV_PIN);
decode_results results;
boolean pause = false;

CRGB leds[NUM_LEDS];

uint8_t * leds_hue;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { read_sd, point, line, pulse, gradient, desaturate, pixelated, pixelated_hue, pixelated_drift };
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
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(192);
  }
}

uint8_t trapeze_val(uint8_t min_val, uint8_t i) {
  return 255 + ((255-min_val)/(2*fade_size)) * (NUM_LEDS-1-2*fade_size-pos_shift - abs(i - fade_size) - abs(i - (NUM_LEDS-1-pos_shift - fade_size)));
}

void trapeze_fade() {
  for(uint8_t i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(trapeze_val(0, i));
  }
}

void reset_leds_hue()
{
  //Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
  if (pattern_idx == channels_nbr-1) {
    leds_hue = (uint8_t*) malloc(NUM_LEDS);
    for (uint8_t i = 0; i < NUM_LEDS; i++) { leds_hue[i] = hue_shift; }
  } else {
    free(leds_hue);
    leds_hue = NULL;
  }
  //Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
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

void flushFadeSizeBuffer() {
  buffer[curr_char_idx] = 0;
  fade_size = atoi(buffer);
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
        if (brightness >= 5) { brightness -= 5; }
        FastLED.setBrightness(brightness); break;

      case 0xFFA857:
      case 0xA3C8EDDB:
        if (brightness <= 250) { brightness += 5; }
        FastLED.setBrightness(brightness); break;

      case 0xFFA25D:
      case 0xE318261B:
        prevPattern(); reset_leds_hue(); break;

      case 0xFFE21D:
      case 0xEE886D7F:
        nextPattern(); reset_leds_hue(); break;

      case 0xFF629D:
      case 0x511DBB:
        flushChannelBuffer(); reset_leds_hue(); break;

      case 0xFF906F:
      case 0xE5CFBD7F:
        flushFadeSizeBuffer(); break;

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
    trapeze_fade();
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

/*
int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
*/

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
    for (int j=0; j<sd_width; j++) {
      r = silhouette.read();
      g = silhouette.read();
      b = silhouette.read();
      if (fade_size == 0) {
        leds[j] = CRGB(r, g, b);		// note that CRGB class would first retrieve b, then g, then r
      } else {
        leds[NUM_LEDS-1-pos_shift - j] = CRGB(r, g, b);
      }
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

/*
void black()
{
  FastLED.clear();
}
*/

void point()
{
  leds[pos_shift] = CHSV(hue_shift, 255, 255);
}

void line()
{
  for(uint8_t i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift, 255, 255);
  }
}

void pulse()
{
  uint8_t beat = beatsin8(fps_arr[fps_idx], 0, 255);
  fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, beat));
  //fill_solid(&(leds[pos_shift]), NUM_LEDS-pos_shift, CHSV(hue_shift, 255, beat));
}

void gradient()
{
  uint8_t beat = beatsin8(fps_arr[fps_idx], 0, 255);
  uint8_t beat2 = beatsin8(2*fps_arr[fps_idx], 0, 100);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat2, 255, 255), CHSV(hue_shift+beat, 255, 255));
}

/*
void strikes()
{
  if ( random8() < 20 ) {
    uint8_t size;
    switch((pos_shift / 10) % 10) {
      case 1:
        size = 1; break;
      case 2:
        size = 25; break;
      case 3:
        size = NUM_LEDS; break;
      default:
        size = 25 + (7 - random8(14));
    }

    uint8_t hue;
    switch(pos_shift / 100) {
      case 1:
        hue = hue_shift + (30 - random8(60)); break;
      case 2:
        hue = random8(); break;
      default:
        hue = hue_shift;
    }

    uint8_t start = random8(NUM_LEDS - size);
    fill_solid(&(leds[start]), size, CHSV(hue, 255, 255));
    FastLED.show();

    uint8_t del;
    switch(pos_shift % 10) {
      case 1:
        del = 2; break;
      case 2:
        del = 15; break;
      case 3:
        del = 500; break;
      default:
        del = 50;
    }
    delay(del);

    FastLED.clear();
    FastLED.show();
  }
}

void rainbow()
{
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

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

void desaturate()
{
  uint8_t beat = beatsin8(fps_arr[fps_idx], 0, 75);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat, 140, 255), CHSV(hue_shift+beat+10, 10, 255));
}

void pixelated()
{
  for(uint8_t i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(random8(255), 255, 255);
  }
}

void pixelated_hue()
{
  leds[0] = CHSV(hue_shift, 255, 255);
  uint8_t curr_hue = hue_shift;
  for(uint8_t i = 1; i < NUM_LEDS-pos_shift; i++) {
    curr_hue += 9 - random8(19);
    leds[i] = CHSV(curr_hue, 255, 255);
  }
}

void pixelated_drift()
{
  for(uint8_t i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(leds_hue[i], 255, 255);
    leds_hue[i] += 12 - random8(25);
  }
}

/*
void splash()
{
  // random colored splashes that fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 1);
  if (random8() < 4) {
    uint8_t size;
    if (pos_shift == 0) { size = 20; }
    else { size = min(pos_shift, 140); }
    uint8_t pos = random8(NUM_LEDS-1 - size);

    bool overlap = false;
    for(uint8_t i = pos; i < pos+size; i++) {
      overlap |= leds[i];
    }

    if (!overlap) {
      for(uint8_t i = pos; i < pos+size; i++) {
        uint8_t theta = (pos+(size/2)-i) * 255 / (2*size);
        uint8_t val = cos8(theta) - 1;          // library bug? val cannot be 255 below
        leds[i] = CHSV(hue_shift, 230, val);
      }
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
    int jpulse = (i+13)/3;
    leds[beatsin16( jpulse, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
*/

