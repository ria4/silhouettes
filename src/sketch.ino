// Preamble

#include "FastLED.h"
#include "IRremote.h"

#include "silhouettes.h"
#include "patterns/point.h"
#include "patterns/line.h"
#include "patterns/pulse.h"
#include "patterns/confetti.h"


FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif


// Constants & Variables

//#define DEBUG

byte leds_hue[NUM_LEDS];

Channel channels[] = {
  point,
  line,
  pulse,
  confetti,
};
/*
} channels[] {
  { strikes, false, true },
  //{ window, false, false },
  { spaces, true, false },
  { dual, false, false },
  { dual, false, false },
  { dual_rand, false, false },
  { glitch_correct, false, false },
  { glitch_correct, false, false },
  //{ line_flashes, false, true },
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
  //{ pixelated_hue, false, false },
  //{ pixelated_drift, false, false },
  //{ disintegrate, false, false },
  //{ invade, false, false },
  //{ split, false, false },
};
*/

IRrecv irrecv(IRRCV_PIN);
decode_results results;
char ir_buffer[3];
byte curr_char_idx = 0;

byte split_start;
byte split_size;
byte split_hue_top;

bool hole[20];
byte hole_n;
byte hole_size;
byte holes_hue_shift;

bool sand_burnt[NUM_LEDS];

byte heartbeat_r;
byte heartbeat_b;

byte line_flashes_cnt;

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

    //if ((channels[channel_idx].pattern != holes) &&
    //    (channels[channel_idx].pattern != heartbeat) &&
    //    (channels[channel_idx].pattern != border) &&
    //    (channels[channel_idx].pattern != window) &&
    //    (channels[channel_idx].pattern != rgb)) {
    if (channels[channel_idx].fade_edges) {
      fade_edges();
    }

    if (channel_timer < 255) {
      EVERY_N_MILLISECONDS(20) { channel_timer += 1; }
    }
    if (channels[channel_idx].fade_init) {
      if (channel_timer < 50) {
        timer_fade_in();
      }
    }

    EVERY_N_MILLISECONDS(10) { hue_warp += random8(10); }
  }

  while (!irrecv.isIdle());
  checkIRSignal();

  if (!pause) {
    FastLED.show();
    // long delays with FastLED cause long periods when the IRremote cannot catch interrupts
    //FastLED.delay(1000/fps_arr[fps_idx]);
    if (channels[channel_idx].self_refresh) { delay(5); }
    else { delay(1000/fps_arr[fps_idx]); }
    //TODO init this static array
  }
}


// Helpers

byte neighbours_up(byte i) {
  if (i == 0) { return (byte) leds[1]; }
  if (i == NUM_LEDS-1) { return (byte) leds[NUM_LEDS-2]; }
  return ((byte) leds[i-1]) + ((byte) leds[i+1]);
}

byte fade_edges_px(byte min_val, byte i) {
  return 255 + ((255-min_val)/(2*fade_size)) * (NUM_LEDS-1-2*fade_size-pos_shift - abs(i - fade_size) - abs(i - (NUM_LEDS-1-pos_shift - fade_size)));
}

void fade_edges() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(fade_edges_px(0, i));
  }
}

void timer_fade_in() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(channel_timer*5);
  }
}

void reset_leds_hue() {
  for (byte i = 0; i < NUM_LEDS; i++) { leds_hue[i] = hue_shift; }
  /*
  // use this to free memory (leds_hue must be used by the last channel only)
  #ifdef DEBUG
  Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
  #endif
  if (channel_idx == channels_nbr-1) {
    leds_hue = (byte*) malloc(NUM_LEDS);
    for (byte i = 0; i < NUM_LEDS; i++) { leds_hue[i] = hue_shift; }
  } else {
    free(leds_hue);
    leds_hue = NULL;
  }
  #ifdef DEBUG
  Serial.print(F("Remaining SRAM: ")); Serial.println(freeRam());
  #endif
  */
}


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
        if (fps_idx > 0) { fps_idx -= 1; } break;
        // no loop here, there might be a long FastLED.delay

      case 0xFF02FD:
      case 0xD7E84B1B:
        if (fps_idx < FPS_MODES_NBR - 1) { fps_idx += 1; } break;

      case 0xFFC23D:
      case 0x20FE4DBB:
        FastLED.clear(); FastLED.show(); pause = !pause;
        if (!pause) { channel_timer = 0; } break;

      default:
        break;
        #ifdef DEBUG
        Serial.println(results.value, HEX);
        #endif

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


void gradient() {
  byte beat = beatsin8(fps_arr[fps_idx], 0, 100);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat*2, 255, 255), CHSV(hue_shift+beat+50, 255, 255));
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
