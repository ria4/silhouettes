// Preamble

#include "FastLED.h"
#include "silhouettes.h"
#include "patterns/pulse.h"
#include "patterns/sd_read.h"
#include "IRremote.h"


FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif


// Constants & Variables

//#define DEBUG

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
//#define NUM_LEDS    143

#define BUTTON_PIN    2
#define IRRCV_PIN     9
#define STRIP_PIN     6

//#define FPS_MODES_NBR           5
#define CHANGE_SIG_LENGTH       50

//#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


//CRGB leds[NUM_LEDS];
byte leds_hue[NUM_LEDS];

Channel channels[] = { pulse };
/*
struct Channel {
  void (*pattern)();
  bool fade_in;
  bool auto_refresh;
} channels[] {
  { point, false, false },
  { strikes, false, true },
  //{ window, false, false },
  { spaces, true, false },
  { dual, false, false },
  { dual, false, false },
  { dual_rand, false, false },
  { glitch_correct, false, false },
  { glitch_correct, false, false },
  //{ line_flashes, false, true },
  { line, true, false },
  { pulse, false, false },
  { gradient, true, false },
  { noise, true, false },
  { holes, false, false },
  { lace, true, false },
  { sand, false, false },
  { heartbeat, false, true },
  //{ border, true, false },
  { border_i1, true, false },
  { border_i2, true, false },
  { border_i3, true, false },
  //{ rgb, false, false },
  //{ cylon, true, true },
  //{ cylon_rainbow, true, true },
  //{ pixelated_hue, false, false },
  //{ pixelated_drift, false, false },
  //{ disintegrate, false, false },
  //{ invade, false, false },
  //{ split, false, false },
};
*/

//byte channel_idx = 0;
//byte channel_idx_tmp;

//bool pause = false;
//byte channel_timer = 0;

//const int fps_arr[FPS_MODES_NBR] = { 3, 12, 50, 200, 1000 };      // the frame rate may cap by itself

IRrecv irrecv(IRRCV_PIN);
decode_results results;
char ir_buffer[3];
byte curr_char_idx = 0;
//byte pos_shift = 0;
//byte hue_shift = 0;
//byte fade_size = 0;

static uint16_t noise_z;

byte brightness = 15;
//byte fps_idx = 2;
byte gHue = 0;

byte split_start;
byte split_size;
byte split_hue_top;

/*
const byte QUARKS_MAX = 64;
const byte QUARK_DEF_SIZE = 3;
typedef struct Quark {
  int pos; byte pos8;
  int a; int b;
  byte size;
  byte h; byte s;
  bool move_away = false;
};
Quark quarks[QUARKS_MAX];
byte quarks_n;

typedef struct QuarkDeco {
  Quark qk;
  byte n = 8;
  bool line[8];         // n
  bool gap[7];          // n-1
  byte gap_size[7];     // n-1
  byte size;
};
QuarkDeco qkd;
*/

bool hole[20];
byte hole_n;
byte hole_size;
byte holes_hue_shift;

bool sand_burnt[NUM_LEDS];

byte heartbeat_r;
byte heartbeat_b;

byte line_flashes_cnt;

unsigned long start_millis;
unsigned long last_change;
unsigned long next_change;

void set_raw_noise(byte i, byte hue_shift2=0);


// Main Functions

void setup() {
  delay(200);
  #ifdef DEBUG
  Serial.begin(9600); Serial.println(F("Resetting..."));
  #endif

  channels_nbr = ARRAY_SIZE(channels);
  FastLED.addLeds<LED_TYPE,STRIP_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  reset_leds_hue();

  irrecv.enableIRIn();

  noise_z = random16();

  #ifdef SD_READ
    #ifdef DEBUG
    Serial.print(F("Trying to init SD card... "));
    #endif
    pinMode(10, OUTPUT);
    if (!SD.begin(10)) {
      #ifdef DEBUG
      Serial.println(F("init failed!"));
      #endif
      return;
    }
    #ifdef DEBUG
    Serial.println(F("OK!"));
    #endif
  #endif
}

void loop()
{
  if (!pause) {
    channels[channel_idx].pattern();

    if ((channels[channel_idx].pattern != holes) &&
        (channels[channel_idx].pattern != heartbeat) &&
        (channels[channel_idx].pattern != border) &&
        (channels[channel_idx].pattern != window) &&
        (channels[channel_idx].pattern != rgb)) {
      trapeze_fade();
    }

    if (channel_timer < 255) {
      EVERY_N_MILLISECONDS(20) { channel_timer += 1; }
    }
    if (channels[channel_idx].fade_in) {
      if (channel_timer < 50) {
        timer_fade_in();
      }
    }

    EVERY_N_MILLISECONDS(10) { gHue += random8(10); }
  }

  while (!irrecv.isIdle());
  checkIRSignal();

  if (!pause) {
    FastLED.show();
    // long delays with FastLED cause long periods when the IRremote cannot catch interrupts
    //FastLED.delay(1000/fps_arr[fps_idx]);
    if (channels[channel_idx].auto_refresh) { delay(5); }
    else { delay(1000/fps_arr[fps_idx]); }
  }
}


// Helpers

byte neighbours_up(byte i) {
  if (i == 0) { return (byte) leds[1]; }
  if (i == NUM_LEDS-1) { return (byte) leds[NUM_LEDS-2]; }
  return ((byte) leds[i-1]) + ((byte) leds[i+1]);
}

byte trapeze_val(byte min_val, byte i) {
  return 255 + ((255-min_val)/(2*fade_size)) * (NUM_LEDS-1-2*fade_size-pos_shift - abs(i - fade_size) - abs(i - (NUM_LEDS-1-pos_shift - fade_size)));
}

void trapeze_fade() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(trapeze_val(0, i));
  }
}

void timer_fade_in() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(channel_timer*5);
  }
}

void reset_leds_hue() {
  for (byte i = 0; i < NUM_LEDS; i++) { leds_hue[i] = hue_shift; }
}

/*
void reset_leds_hue()
{
  //Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
  if (channel_idx == channels_nbr-1) {
    leds_hue = (byte*) malloc(NUM_LEDS);
    for (byte i = 0; i < NUM_LEDS; i++) { leds_hue[i] = hue_shift; }
  } else {
    free(leds_hue);
    leds_hue = NULL;
  }
  //Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
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


// Variables Updating

void addToBuffer(char c) {
  if ( curr_char_idx < 3 ) {
    ir_buffer[curr_char_idx++] = c;
  }
}

void flushChannelBuffer() {
  ir_buffer[curr_char_idx] = 0;
  channel_idx_tmp = atoi(ir_buffer);       // note that no buffer raises a plain 0
  if (channel_idx_tmp < channels_nbr) {
    channel_idx = channel_idx_tmp;
    reset_leds_hue();
    signal(255);
    #ifdef SD_READ
    if (channel_idx == 0) { signal(0); } else { signal(255); }
    #endif
  }
  #ifdef SD_READ
  else { if (set_sd_silhouette(channel_idx_tmp)) { channel_idx = 0; signal(0); } }
  #endif
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
        prevChannel(); break;

      case 0xFFE21D:
      case 0xEE886D7F:
        nextChannel(); break;

      case 0xFF629D:
      case 0x511DBB:
        flushChannelBuffer(); break;

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
        FastLED.clear(); FastLED.show(); pause = !pause;
        if (!pause) { channel_timer = 0; } break;

      default:
        break; //Serial.println(results.value, HEX);

    }
    delay(50);
    irrecv.resume();
  }
}


// Channel Switching

// brief signal when a new channel is loaded: blue for SD, red for programmatic
void signal(byte rb) {
  FastLED.clear();
  leds[0] = CRGB(rb, 0, 255-rb);
  FastLED.show();
  delay(CHANGE_SIG_LENGTH);
  leds[0] = CRGB(0, 0, 0);
  FastLED.show();
  pause = true;
}

void prevChannel() {
  #ifdef SD_READ
  if (channel_idx == 0) {
    if (set_sd_silhouette(sd_idx - 1)) { signal(0); return; }
    else { channel_idx = channels_nbr - 1; } }
  else {
    if (channel_idx == 1) {
      if (set_sd_silhouette(sd_idx)) { channel_idx = 0; signal(0); return; }
      else { channel_idx = channels_nbr - 1; } }
    else { channel_idx--; } }
  #else
  if (channel_idx == 0) { channel_idx = channels_nbr-1; } else { channel_idx--; }
  #endif
  reset_leds_hue();
  signal(255);
}

void nextChannel() {
  #ifdef SD_READ
  if (channel_idx == 0) {
    if (set_sd_silhouette(sd_idx + 1)) { signal(0); return; }
    else { channel_idx = 1; } }
  else {
    if (channel_idx == channels_nbr - 1) {
      if (set_sd_silhouette(sd_idx)) { channel_idx = 0; signal(0); return; }
      else { channel_idx = 0; } }
    else { channel_idx++; } }
  #else
  if (channel_idx == channels_nbr-1) { channel_idx = 0; } else { channel_idx++; }
  #endif
  reset_leds_hue();
  signal(255);
}


// Channels

void point() {
  leds[pos_shift].setHue(hue_shift);
}


void heartbeat() {
  FastLED.clear();

  if (channel_timer == 0) {
    last_change = millis();
    next_change = 872 + random8();
  }

  if (millis() - last_change < next_change) {

    leds[44] = CRGB(255-(millis()-last_change)/4, 0, (millis()-last_change)/5);
    heartbeat_r = leds[44].r;
    heartbeat_b = leds[44].b;

  } else {

    uint32_t d = millis();
    while (millis() - d < 290) {
      FastLED.clear();

      byte i = abs(beat8(200, d+100) - 128);
      for (byte k=i; k<i+15; k++) {
        int angle = ((k - i) << 8)/ 15;
        leds[k] = CRGB(heartbeat_r, 0, heartbeat_b);
        leds[k].nscale8_video(128 + quadwave8((byte) angle)/2);
      }
      if (heartbeat_r != 255) { heartbeat_r += min(20, 255-heartbeat_r); }
      if (heartbeat_b !=   0) { heartbeat_b -= min(20, heartbeat_b); }
      FastLED.show();
      delay(5);
    }

    last_change = millis();
    next_change = 936 + random8()/2;

  }
}


void glitch_correct() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (leds_hue[i] == hue_shift) {
      if (random8() < 10) {
        leds_hue[i] = random8();
        leds_hue[i] -= (leds_hue[i] - hue_shift) % 8;
        leds[i].setHue(leds_hue[i]);
      }
    } else {
      leds_hue[i] -= 8;
      leds[i].setHue(leds_hue[i]);
    }
  }
}


void spaces() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (i%(pos_shift+1) == 0) {
      leds[i].setHue(hue_shift);
    }
  }
}


void window() {
  for (byte i = 0; i < NUM_LEDS; i++) {
    if ((channel_timer < 64) || (channel_timer > 128) ||
        (i < NUM_LEDS/2-pos_shift) ||
        (i > NUM_LEDS/2+pos_shift)) {
      leds[i] = CHSV(hue_shift - 64 + (inoise8((1+fade_size)*i, noise_z*2) >> 2), 255, 128 + (inoise8((1+fade_size)*i, noise_z) >> 1));
    } else {
      leds[i] = 0;
    }
  }
  noise_z += 15;
}


void dual() {
  if (channel_timer == 0) {
    if (channel_idx % 2 == 0) {
      fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, 255));
    } else {
      fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+70, 255, 255));
      for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
        leds_hue[i] = hue_shift+70;
      }
    }
  }

  if (channel_idx % 2 == 0) {
    if (leds_hue[3] != hue_shift+70) {
      for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
        if (i % 6 >= 3) {
          leds_hue[i] += 1;
          leds[i].setHue(leds_hue[i]);
        }
      }
    }
  } else {
    if (leds_hue[0] != hue_shift) {
      for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
        if (i % 6 < 3) {
          leds_hue[i] -= 1;
          leds[i].setHue(leds_hue[i]);
        }
      }
    }
  }
}


void dual_rand() {
  if (channel_timer == 0) {
    fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, 255));
  }

  for (byte i=0; i<NUM_LEDS-pos_shift-3; i=i+3) {
    if (random8() < 4) {
      if (leds_hue[i] == hue_shift) {
        for (byte j=0; j<3; j++) {
          leds_hue[i+j] = hue_shift+70;
          leds[i+j].setHue(hue_shift+70);
        }
      } else {
        for (byte j=0; j<3; j++) {
          leds_hue[i+j] = hue_shift;
          leds[i+j].setHue(hue_shift);
        }
      }
    }
  }
}


void line_flashes() {
  delay(4000);
  byte h = 255/6 * line_flashes_cnt;
  fill_solid(leds, NUM_LEDS-pos_shift, CHSV(h, 255, 255));
  line_flashes_cnt = (line_flashes_cnt + 1) % 6;
  FastLED.show();
  delay(500);
  FastLED.clear();
}


void rgb() {
  for (byte i=0; i<fade_size+1; i++) {
    leds[4+i]                                     = CRGB(200, 0, 0);
    leds[(NUM_LEDS+4-pos_shift-fade_size)/2 + i]  = CRGB(0, 255, 0);
    leds[NUM_LEDS-1-pos_shift - i]                = CRGB(0, 0, 190);
  }
}


void lace() {
  FastLED.clear();
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    byte r = inoise8((1+fade_size)*2*i, noise_z);
    if (r < 128) {
      leds[i] = CHSV(hue_shift, 255, 255);
    } else {
      leds[i] = CHSV(hue_shift + (175-r)*2, 255, 255);
    }
  }
  noise_z += 15;
}


void sand() {
  if (channel_timer == 0) {
    for (byte i=0; i<NUM_LEDS-pos_shift; i++) { sand_burnt[i] = false; }
    fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, 255));
  }

  byte i = 0;
  while ((i < NUM_LEDS-pos_shift) && sand_burnt[i] && (leds_hue[i] == hue_shift)) { i+=1; }

  if (i == NUM_LEDS-pos_shift) {
    for (byte i=0; i<NUM_LEDS-pos_shift; i++) { sand_burnt[i] = false; }
    return;
  }

  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (!sand_burnt[i]) {
      if (random8() > 240) {
        leds_hue[i] = hue_shift + 128;
        leds[i].setHue(leds_hue[i]);
        sand_burnt[i] = true;
      }
    } else {
      if (leds_hue[i] != hue_shift) {
        leds_hue[i] += max(1, fps_arr[fps_idx]/50);
        leds[i].setHue(leds_hue[i]);
      }
    }
  }
}


void holes_change() {
  last_change = millis();
  for (byte i = 0; i < hole_n; i++) { if (random8(2) < 1) { hole[i] = true; } else { hole[i] = false; } }
}

void holes() {
  if (channel_timer == 0) {
    if (fade_size % 3 == 0) { hole_n = 20; hole_size = 6; }
    else if (fade_size % 3 == 1) { hole_n = 14; hole_size = 9; }
    else { hole_n = 10; hole_size = 13; }
    holes_hue_shift = 0;
    /*if (channel_idx % 2 == 0) { holes_hue_shift = 0; }
    else { holes_hue_shift = 128; }*/
    holes_change();
  }

  FastLED.clear();

  if (millis() - last_change > 50000/fps_arr[fps_idx]) {
    holes_change();

    /*if (channel_idx % 2 == 1) {
      for(byte i = 0; i < hole_n*(hole_size+1)+1; i++) {
        set_raw_noise(i, holes_hue_shift);
      }
      FastLED.show();
      delay(5000/fps_arr[fps_idx]);
      return;
    }*/
  }

  byte start = 0;
  for(byte i = 0; i < hole_n; i++) {
    set_raw_noise(start, holes_hue_shift);
    if (!hole[i]) {
      for(byte j = start+1; j < start+1+hole_size; j++) {
        set_raw_noise(j);
      }
    }
    start += hole_size + 1;
  }
  set_raw_noise(hole_n*(hole_size+1), holes_hue_shift);
  noise_z += 15;
}


void line() {
/*
  hue_shift = 255/6 * ((channel_idx-1) % 6);
  for(byte i = pos_shift; i < NUM_LEDS; i++) {
    leds[i].setHue(hue_shift);
  }
*/
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].setHue(hue_shift);
  }
}

void gradient() {
  byte beat = beatsin8(fps_arr[fps_idx], 0, 100);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat*2, 255, 255), CHSV(hue_shift+beat+50, 255, 255));
}

void cylon() {
  FastLED.clear();
  int beat = beatsin16(fps_arr[fps_idx], 0, (NUM_LEDS-1 - pos_shift - 2*fade_size)<<8);
  byte beat8 = (byte) (beat>>8);
  for(byte i = beat8+1; i < beat8+2*fade_size; i++) {
    int angle = ((i<<8) - beat) / (2*fade_size);
    leds[i] = CHSV(hue_shift, 255, quadwave8((byte) angle));
  }
}

void cylon_rainbow() {
  FastLED.clear();
  int beat = beatsin16(fps_arr[fps_idx], 0, (NUM_LEDS-1 - pos_shift - 2*fade_size)<<8);
  byte beat8 = (byte) (beat>>8);
  for(byte i = beat8+1; i < beat8+2*fade_size; i++) {
    int angle = ((i<<8) - beat) / (2*fade_size);
    leds[i] = CHSV(gHue+(i-beat8-fade_size)*3, 255, quadwave8((byte) angle));
  }
}

void pixelated_hue() {
  leds[0].setHue(hue_shift);
  byte curr_hue = hue_shift;
  for(byte i = 1; i < NUM_LEDS-pos_shift; i++) {
    curr_hue += 7 - random8(15);
    leds[i].setHue(curr_hue);
  }
}

void pixelated_drift() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].setHue(leds_hue[i]);
    leds_hue[i] += 12 - random8(25);
  }
}


void set_raw_noise(byte i, byte hue_shift2) {
  leds[i] = CHSV(hue_shift + hue_shift2 + inoise8((1+fade_size)*i, noise_z)/2, 192 + (inoise8((1+fade_size)*i, noise_z) >> 2), 255);
}

void noise() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    set_raw_noise(i);
  }
  noise_z += 15;
}

void noise_fade_out() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift - 64 + (inoise8((1+fade_size)*i, noise_z*2) >> 2), 255, 64 + (inoise8((1+fade_size)*i, noise_z) >> 2)*3);
    leds[i].nscale8(255-channel_timer);
  }
  noise_z += 15;
}

void border() {
  leds[0] = CRGB(80, 80, 80);
  leds[1] = CRGB(80, 80, 80);
  leds[2] = CRGB(80, 80, 80);
  leds[(NUM_LEDS-pos_shift)/2-1] = CRGB(80, 80, 80);
  leds[(NUM_LEDS-pos_shift)/2] = CRGB(80, 80, 80);
  leds[(NUM_LEDS-pos_shift)/2+1] = CRGB(80, 80, 80);
  leds[NUM_LEDS-pos_shift-3] = CRGB(80, 80, 80);
  leds[NUM_LEDS-pos_shift-2] = CRGB(80, 80, 80);
  leds[NUM_LEDS-pos_shift-1] = CRGB(80, 80, 80);

  for(byte i = 3; i < (NUM_LEDS-pos_shift)/2-1; i++) {
    leds[i] = CHSV(hue_shift + inoise8((1+fade_size)*i, noise_z)/2, 192 + (inoise8((1+fade_size)*i, noise_z) >> 2), 255);
  }
  for(byte i = (NUM_LEDS-pos_shift)/2+2; i < NUM_LEDS-pos_shift-3; i++) {
    leds[i] = CHSV(hue_shift + inoise8((1+3*fade_size)*i, 3*noise_z)/4*3, 192 + (inoise8((1+3*fade_size)*i, 3*noise_z) >> 2), 255);
  }
  noise_z += 15;
}


double sqi;
void border_i1() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    sqi = sqrt(i);
    leds[i] = CHSV(hue_shift + inoise8((1+fade_size)*i*sqi*sqrt(sqi), noise_z)/3*2, 255, 255);
  }
  noise_z += 15;
}


void border_i2() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(inoise8(1+fade_size, noise_z*i/16), 255, 255);
  }
  noise_z += 2;
}


void border_i3() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift + inoise8(1+fade_size, noise_z*i/16)/2, 255, 255);
  }
  noise_z += 2;
}


void disintegrate() {
  if (channel_timer == 0) {
    uint16_t r = random16();
    for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
      leds[i].setHue(hue_shift - 32 + inoise8(10*i, r)/4);
    }
  }

  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    if (leds[i]) {
      byte neighbours = neighbours_up(i);
      if (((neighbours == 0) && (random8() < 16)) ||
          ((neighbours == 1) && (random8() < 8)) ||
          ((neighbours == 2) && (random16() < 32))) {
        leds[i] = 0;
      }
    }
  }
}

void invade() {
  if (random8() < 220) {
    for (byte i = 0; i < NUM_LEDS-pos_shift; i++) {
      leds[i].fadeToBlackBy(1);
    }
  }

  for (byte i = 1; i < NUM_LEDS-pos_shift-1; i++) {
    int r = random16();
    if ((r < 128) && (r > 3)) {
      for (int j = i; j <= min(NUM_LEDS-pos_shift-1, i + r/8); j++) {
        leds[j].setHue(hue_shift - 96 + inoise8(5*i, noise_z)/4*3);
      }
    }
  }
  noise_z += 10;
}

void split() {
  byte middle = (NUM_LEDS-pos_shift)/2;

  split_start = middle - 52 + inoise8(20, noise_z)/3;
  split_size = inoise8(20, 2*noise_z)/6;
  if (split_size % 2 == 1) { split_size += 1; }
  split_hue_top = hue_shift - 85 + inoise8(20, 3*noise_z)/3;

  fill_gradient(leds, split_start, CHSV(hue_shift, 255, 255), CHSV(split_hue_top, 255, 255));
  fill_gradient(&(leds[split_start]), split_size/2, CHSV(split_hue_top, 255, 255), CHSV(split_hue_top, 0, 0));
  fill_gradient(&(leds[split_start+split_size/2]), split_size/2, CHSV(split_hue_top, 0, 0), CHSV(split_hue_top, 255, 255));
  fill_gradient(&(leds[split_start+split_size]), NUM_LEDS-pos_shift-split_size-split_start, CHSV(split_hue_top, 255, 255), CHSV(hue_shift, 255, 255));

  noise_z += 10;
}

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
  }
}

/*
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


void quarks_collide() {
  if (channel_timer == 0) {
    start_millis = millis();
    quarks_n = QUARKS_MAX >> (2*(2-(fade_size % 3)));
    for (byte i=0; i<quarks_n; i++) {
      Quark qk;
      qk.a = fps_arr[fps_idx]/2 - random8(fps_arr[fps_idx]);
      qk.b = random8(qk.size+2, NUM_LEDS-pos_shift-qk.size-2) << 8;
      qk.size = QUARK_DEF_SIZE*2;
      qk.pos = qk.b;
      qk.pos8 = (byte) (qk.pos>>8);
      if (pos_shift % 2 == 0) { qk.h = hue_shift; }
      else { qk.h = hue_shift + 40 - random8(80); }
      qk.s = 255;
      quarks[i] = qk;
    }
  }

  FastLED.clear();

  for (byte i=0; i<quarks_n; i++) {
    if (!quarks[i].move_away) {
      if (quarks[i].pos8 <= fps_arr[fps_idx]/50) {
        quarks[i].a = random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
        quarks[i].b = quarks[i].pos - (millis() - start_millis) * quarks[i].a/4;
        quarks[i].move_away = true;
      } else if (quarks[i].pos8 >= NUM_LEDS-pos_shift-quarks[i].size-fps_arr[fps_idx]/50) {
        quarks[i].a = -random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
        quarks[i].b = quarks[i].pos - (millis() - start_millis) * quarks[i].a/4;
        quarks[i].move_away = true;
      } else if (random8() > 252 - (fps_arr[fps_idx]>>5)) {
        if (quarks[i].a > 0) { quarks[i].a = -random8(4, fps_arr[fps_idx]/2); } else { quarks[i].a = random8(4, fps_arr[fps_idx]/2); }
        quarks[i].b = quarks[i].pos - (millis() - start_millis) * quarks[i].a/4;
      }
    }

    if ((quarks[i].pos8 > 10) && (quarks[i].pos8 < NUM_LEDS-pos_shift-quarks[i].size-10)) { quarks[i].move_away = false; }

    quarks[i].pos = (millis() - start_millis) * quarks[i].a/4 + quarks[i].b;
    quarks[i].pos8 = (byte) (quarks[i].pos>>8);
    for(byte j = quarks[i].pos8; j < quarks[i].pos8+quarks[i].size; j++) {
      if (leds[j]) {
        if (channel_idx % 2 == 1) {
          leds[j] = 0;
          continue;
        }
      }
      int angle = ((j<<8) - quarks[i].pos) / quarks[i].size;
      leds[j] |= CHSV(quarks[i].h, quarks[i].s, quadwave8((byte) angle));
    }
  }
}


void qkd_change() {
  last_change = millis();
  byte change = random8(qkd.n + qkd.n-1);
  if (change < qkd.n ) {
    if (qkd.line[change] && (random8() < 64)) {
      qkd.line[change] = false;
      if (change > 0) { qkd.gap[change-1] = false; }
      if (change < qkd.n - 1) { qkd.gap[change] = false; }
    } else {
      qkd.line[change] = true;
      if ((change > 0) && qkd.line[change-1] && (random8(2) > 0)) { qkd.gap[change-1] = true; }
      if ((change < qkd.n - 1) && qkd.line[change+1] && (random8(2) > 0)) { qkd.gap[change] = true; }
    }
  } else {
    change -= qkd.n;
    if (qkd.gap[change] && (random8() > 16)) {
      qkd.gap[change] = false;
      if (random8(2) > 0) { qkd.line[change] = false; }
      if (random8(2) > 0) { qkd.line[change+1] = false; }
    } else {
      if (random8(2) > 0) { qkd.line[change] = true; qkd.line[change+1] = true; }
      if (qkd.line[change] && qkd.line[change+1]) { qkd.gap[change] = true; }
    }
  }
}

void qkd_set_color(byte j, byte c, byte v) {
  byte hue; byte coeff;
  if (fade_size % 2 == 0) { coeff = j; } else { coeff = c; }
  hue = inoise8((1+fade_size)*coeff, noise_z);
  if (fade_size % 4 < 2) { hue = hue_shift + hue/2; }
  leds[c] |= CHSV(hue, 192 + (inoise8((1+fade_size)*j, noise_z) >> 2), 255-v);
}

void quarks_deco() {
  if (channel_timer == 0) {
    start_millis = millis();
    qkd.qk.size = QUARK_DEF_SIZE;
    qkd.qk.a = fps_arr[fps_idx]/2 - random8(fps_arr[fps_idx]);
    qkd.qk.b = 20<<8;
    qkd.qk.pos = qkd.qk.b;
    qkd.qk.pos8 = (byte) (qkd.qk.pos>>8);
    qkd.qk.h = hue_shift;
    qkd.qk.s = 255;
    for(byte i = 0; i < qkd.n; i++) {
      if (random8(4) < 3) { qkd.line[i] = true; } else { qkd.line[i] = false; } }
    for(byte i = 0; i < qkd.n - 1; i++) {
      if (qkd.line[i] && qkd.line[i+1] && (random8(2) < 1)) { qkd.gap[i] = true; } else { qkd.gap[i] = false; } }
    for(byte i = 0; i < qkd.n - 1; i++) {
      qkd.gap_size[i] = QUARK_DEF_SIZE*4; }
    qkd.size = qkd.qk.size; for(byte i = 0; i < qkd.n - 1; i++) { qkd.size += qkd.gap_size[i]; }
  }

  FastLED.clear();

  if (!qkd.qk.move_away) {
    if (qkd.qk.pos8 <= fps_arr[fps_idx]/50) {
      qkd.qk.a = random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
      qkd.qk.b = qkd.qk.pos - (millis() - start_millis) * qkd.qk.a/4;
      qkd.qk.move_away = true;
    } else if (qkd.qk.pos8 >= NUM_LEDS-pos_shift-qkd.size-fps_arr[fps_idx]/50) {
      qkd.qk.a = -random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
      qkd.qk.b = qkd.qk.pos - (millis() - start_millis) * qkd.qk.a/4;
      qkd.qk.move_away = true;
    }
  }

  if ((millis() - last_change > (20000/fps_arr[fps_idx])) && (random8() > 253 - (fps_arr[fps_idx]>>5))) {
    qkd_change();
  }

  if ((qkd.qk.pos8 > 20) && (qkd.qk.pos8 < NUM_LEDS-pos_shift-qkd.size-20)) { qkd.qk.move_away = false; }

  qkd.qk.pos = (millis() - start_millis) * qkd.qk.a/4 + qkd.qk.b;
  qkd.qk.pos8 = (byte) (qkd.qk.pos>>8);
  byte pos8 = qkd.qk.pos8;
  for(byte i = 0; i < qkd.n; i++) {
    if (qkd.line[i]) {
      for(byte j = 0; j < 3; j++) {
        qkd_set_color(j, pos8+j, abs(1-j)*120);
      }
    }
    if (i != qkd.n - 1) {
      if (qkd.line[i] && qkd.line[i+1] && qkd.gap[i]) {
        for(byte j = 0; j < qkd.gap_size[i]; j++) {
          qkd_set_color(j, pos8+1+j, 0);
        }
      }
      pos8 += qkd.gap_size[i];
    }
  }
  noise_z += 5;
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
