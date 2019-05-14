// Preamble

#include "FastLED.h"
#include "IRremote.h"

#include "sketch.h"
#include "patterns/point.h"
#include "patterns/line.h"
#include "patterns/pulse.h"
#include "patterns/confetti.h"

FASTLED_USING_NAMESPACE
#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

//#define DEBUG

Channel channels[] = {
  point,
  line,
  pulse,
  confetti,
};


// Main Functions

void setup() {
  delay(200);
  #ifdef DEBUG
  Serial.begin(9600); Serial.println(F("Resetting..."));
  #endif

  channels_nbr = ARRAY_SIZE(channels);
  FastLED.addLeds<LED_TYPE,STRIP_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  resetLedsHue();

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

void loop() {
  if (!pause) {
    channels[channel_idx].pattern();

    if (channels[channel_idx].fade_edges) { fadeEdges(); }
    if ((channels[channel_idx].fade_init) && (channel_timer < 50)) { fadeInit(); }

    if (channel_timer < 255) { EVERY_N_MILLISECONDS(20) { channel_timer += 1; } }
    EVERY_N_MILLISECONDS(10) { hue_warp += random8(10); }
  }

  while (!irrecv.isIdle());
  checkIRSignal();

  if (!pause) {
    FastLED.show();
    if (channels[channel_idx].self_refresh) { delay(5); }
    else { delay(1000/fps_arr[fps_idx]); }
    // FastLED.delay may prevent IRremote from catching interrupts
    //TODO init this static array
  }
}


// Helpers

void fadeInit() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(channel_timer*5);
  }
}

byte fadeEdgesPx(byte min_val, byte i) {
  return 255 + ((255-min_val)/(2*fade_size)) * (NUM_LEDS-1-2*fade_size-pos_shift - abs(i - fade_size) - abs(i - (NUM_LEDS-1-pos_shift - fade_size)));
}

void fadeEdges() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].nscale8(fadeEdgesPx(0, i));
  }
}

byte neighboursUp(byte i) {
  if (i == 0) { return (byte) leds[1]; }
  if (i == NUM_LEDS-1) { return (byte) leds[NUM_LEDS-2]; }
  return ((byte) leds[i-1]) + ((byte) leds[i+1]);
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void resetLedsHue() {
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

void noisePx(byte i, byte hue_shift2) {
  leds[i] = CHSV(hue_shift + hue_shift2 + inoise8((1+fade_size)*i, noise_z)/2, 192 + (inoise8((1+fade_size)*i, noise_z) >> 2), 255);
}


// Buffer Logic

void addToBuffer(char c) {
  if ( curr_char_idx < 3 ) {
    ir_buffer[curr_char_idx++] = c;
  }
}

void flushChannelBuffer() {
  ir_buffer[curr_char_idx] = 0;
  channel_idx_tmp = atoi(ir_buffer);
  // note that no buffer raises a plain 0
  if (channel_idx_tmp < channels_nbr) {
    channel_idx = channel_idx_tmp;
    resetLedsHue();
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


// IR Remote Instructions

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
  resetLedsHue();
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
  resetLedsHue();
  signal(255);
}
