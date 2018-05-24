
// Preamble

#include "FastLED.h"
#include "IRremote.h"
//#include "SD.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif


// Constants & Variables

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    144

#define BUTTON_PIN    2
#define IRRCV_PIN     9
#define STRIP_PIN     6

#define FPS_MODES_NBR           5
#define CHANGE_SIG_LENGTH       50

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


CRGB leds[NUM_LEDS];
byte * leds_hue;

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { cylon, point, line, pulse, gradient, desaturate, noise, pixelated_hue, pixelated_drift };
const byte channels_nbr = ARRAY_SIZE(gPatterns);
byte pattern_idx = 0;
byte pattern_idx_tmp;
boolean pause = false;

const int fps_arr[FPS_MODES_NBR] = { 3, 12, 50, 200, 1000 };      // the frame rate may cap by itself
const boolean auto_refresh[channels_nbr] = { true, false, false, false, false, false, false, false, false };

IRrecv irrecv(IRRCV_PIN);
decode_results results;
char ir_buffer[3];
byte curr_char_idx = 0;
byte pos_shift = 0;
byte hue_shift = 0;
byte fade_size = 0;

static uint16_t noise_z;

byte brightness = 15;
byte fps_idx = 2;
byte gHue = 0;

/*
File silhouette;
char silhouette_name[7];
byte sd_idx;
byte sd_params;
bool sd_loop;
int sd_width;
int sd_frames_nbr;
int sd_frame_idx;
byte r, g, b;
*/


// Main Functions

void setup() {
  delay(500);
  //Serial.begin(9600); Serial.println(F("Resetting..."));

  FastLED.addLeds<LED_TYPE,STRIP_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);

  irrecv.enableIRIn();

  noise_z = random16();

  /*
  //Serial.print(F("Trying to init SD card... "));
  pinMode(10, OUTPUT);
  if (!SD.begin(10)) {
    //Serial.println(F("init failed!"));
    return;
  }
  //Serial.println(F("OK!"));
  */
}

void loop()
{
  if (!pause) {
    gPatterns[pattern_idx]();
    trapeze_fade();
    EVERY_N_MILLISECONDS(30) { gHue++; }
  }

  while (!irrecv.isIdle());
  checkIRSignal();

  if (!pause) {
    FastLED.show();
    // long delays with FastLED cause long periods when the IRremote cannot catch interrupts
    //FastLED.delay(1000/fps_arr[fps_idx]);
    if (auto_refresh[pattern_idx]) {
      delay(5);
    } else {
      delay(1000/fps_arr[fps_idx]);
    }
  }
}


// Helpers

void fadeall() {
  for(byte i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(192);
  }
}

byte trapeze_val(byte min_val, byte i) {
  return 255 + ((255-min_val)/(2*fade_size)) * (NUM_LEDS-1-2*fade_size-pos_shift - abs(i - fade_size) - abs(i - (NUM_LEDS-1-pos_shift - fade_size)));
}

void trapeze_fade() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(trapeze_val(0, i));
  }
}

void reset_leds_hue()
{
  //Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
  if (pattern_idx == channels_nbr-1) {
    leds_hue = (byte*) malloc(NUM_LEDS);
    for (byte i = 0; i < NUM_LEDS; i++) { leds_hue[i] = hue_shift; }
  } else {
    free(leds_hue);
    leds_hue = NULL;
  }
  //Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
}

/*
int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
*/


// Variables Updating

void addToBuffer(char c) {
  if ( curr_char_idx < 3 ) {
    ir_buffer[curr_char_idx++] = c;
  }
}

void flushChannelBuffer() {
  ir_buffer[curr_char_idx] = 0;
  pattern_idx_tmp = atoi(ir_buffer);       // note that no buffer raises a plain 0
  if (pattern_idx_tmp < channels_nbr) {
    pattern_idx = pattern_idx_tmp;
    signal(255);
    //if (pattern_idx == 0) { signal(0); } else { signal(255); }
  }
  //else { if (set_sd_silhouette(pattern_idx_tmp)) { pattern_idx = 0; signal(0); } }
  curr_char_idx = 0;
}

void flushFadeSizeBuffer() {
  ir_buffer[curr_char_idx] = 0;
  fade_size = atoi(ir_buffer);
  curr_char_idx = 0;
  FastLED.clear();
}

void flushPosShiftBuffer() {
  ir_buffer[curr_char_idx] = 0;
  pos_shift = atoi(ir_buffer);
  curr_char_idx = 0;
  FastLED.clear();
}

void flushHueShiftBuffer() {
  ir_buffer[curr_char_idx] = 0;
  hue_shift = atoi(ir_buffer);
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
        if (fps_idx < FPS_MODES_NBR - 1) { fps_idx += 1; } break;

      case 0xFFC23D:
      case 0x20FE4DBB:
        FastLED.clear(); FastLED.show(); pause = !pause; break;

      default:
        break; //Serial.println(results.value, HEX);

    }
    delay(50);
    irrecv.resume();
  }
}


// Channel Switching

// brief signal when a new pattern is loaded: blue for SD, red for programmatic
void signal(byte rb) {
  FastLED.clear();
  leds[0] = CRGB(rb, 0, 255-rb);
  FastLED.show();
  delay(CHANGE_SIG_LENGTH);
  leds[0] = CRGB(0, 0, 0);
  FastLED.show();
  pause = true;
}

void prevPattern() {
/*
  if (pattern_idx == 0) {
    if (set_sd_silhouette(sd_idx - 1)) { signal(0); return; }
    else { pattern_idx = channels_nbr - 1; } }
  else {
    if (pattern_idx == 1) {
      if (set_sd_silhouette(sd_idx)) { pattern_idx = 0; signal(0); return; }
      else { pattern_idx = channels_nbr - 1; } }
    else { pattern_idx--; } }
*/
  if (pattern_idx == 0) { pattern_idx = channels_nbr-1; } else { pattern_idx--; }
  signal(255);
}

void nextPattern() {
/*
  if (pattern_idx == 0) {
    if (set_sd_silhouette(sd_idx + 1)) { signal(0); return; }
    else { pattern_idx = 1; } }
  else {
    if (pattern_idx == channels_nbr - 1) {
      if (set_sd_silhouette(sd_idx)) { pattern_idx = 0; signal(0); return; }
      else { pattern_idx = 0; } }
    else { pattern_idx++; } }
*/
  if (pattern_idx == channels_nbr-1) { pattern_idx = 0; } else { pattern_idx++; }
  signal(255);
}


// Channels

void cylon() {
  FastLED.clear();
  int beat = beatsin16(fps_arr[fps_idx], 0, (NUM_LEDS-1 - pos_shift - 2*fade_size)<<8);
  byte beat8 = (byte) (beat>>8);
  for(byte i = beat8+1; i < beat8+2*fade_size; i++) {
    int angle = ((i<<8) - beat) / (2*fade_size);
    leds[i] = CHSV(hue_shift, 255, quadwave8((byte) angle));
  }
}


void point()
{
  leds[pos_shift] = CHSV(hue_shift, 255, 255);
}

void line()
{
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift, 255, 255);
  }
}

void pulse()
{
  byte beat = beatsin8(fps_arr[fps_idx], 0, 255);
  fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, beat));
  //fill_solid(&(leds[pos_shift]), NUM_LEDS-pos_shift, CHSV(hue_shift, 255, beat));
}

void gradient()
{
  byte beat = beatsin8(fps_arr[fps_idx], 0, 100);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat*2, 255, 255), CHSV(hue_shift+beat+50, 255, 255));
}

void desaturate()
{
  byte beat = beatsin8(fps_arr[fps_idx], 0, 75);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat, 140, 255), CHSV(hue_shift+beat+10, 10, 255));
}

void noise()
{
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift + inoise8((1+fade_size)*i, noise_z)/2, 192 + (inoise8((1+fade_size)*i, noise_z) >> 2), 255);
  }
  noise_z += 15;
}

void pixelated_hue()
{
  leds[0] = CHSV(hue_shift, 255, 255);
  byte curr_hue = hue_shift;
  for(byte i = 1; i < NUM_LEDS-pos_shift; i++) {
    curr_hue += 7 - random8(15);
    leds[i] = CHSV(curr_hue, 255, 255);
  }
}

void pixelated_drift()
{
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(leds_hue[i], 255, 255);
    leds_hue[i] += 12 - random8(25);
  }
}

/*
void strikes()
{
  if ( random8() < 20 ) {
    byte size;
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

    byte hue;
    switch(pos_shift / 100) {
      case 1:
        hue = hue_shift + (30 - random8(60)); break;
      case 2:
        hue = random8(); break;
      default:
        hue = hue_shift;
    }

    byte start = random8(NUM_LEDS - size);
    fill_solid(&(leds[start]), size, CHSV(hue, 255, 255));
    FastLED.show();

    byte del;
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

void splash()
{
  // random colored splashes that fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 1);
  if (random8() < 4) {
    byte size;
    if (pos_shift == 0) { size = 20; }
    else { size = min(pos_shift, 140); }
    byte pos = random8(NUM_LEDS-1 - size);

    bool overlap = false;
    for(byte i = pos; i < pos+size; i++) {
      overlap |= leds[i];
    }

    if (!overlap) {
      for(byte i = pos; i < pos+size; i++) {
        byte theta = (pos+(size/2)-i) * 255 / (2*size);
        byte val = cos8(theta) - 1;          // library bug? val cannot be 255 below
        leds[i] = CHSV(hue_shift, 230, val);
      }
    }
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
  byte BeatsPerMinute = 8;
  CRGBPalette16 palette = PartyColors_p;
  byte beat = beatsin8( BeatsPerMinute, 64, 255);
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


// SC Card Image Import

/*
bool set_sd_silhouette(byte n) {
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
*/

