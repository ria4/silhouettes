void sinelon_p() {
  // a colored dot sweeping back and forth, with fading trail
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(2, 0, 1.2*NUM_LEDS-1 ) - (0.1*NUM_LEDS);
  if ((pos >= 0) & (pos < NUM_LEDS)) {
    leds[pos] += CHSV(hue_warp, 255, 192);
  }
}

void cylon_p() {
  FastLED.clear();
  int beat = beatsin16(fps_arr[fps_idx], 0, (NUM_LEDS-1 - pos_shift - 2*fade_size)<<8);
  byte beat8 = (byte) (beat>>8);
  for(byte i = beat8+1; i < beat8+2*fade_size; i++) {
    int angle = ((i<<8) - beat) / (2*fade_size);
    leds[i] = CHSV(hue_shift, 255, quadwave8((byte) angle));
  }
}

void cylon_rainbow_p() {
  FastLED.clear();
  int beat = beatsin16(fps_arr[fps_idx], 0, (NUM_LEDS-1 - pos_shift - 2*fade_size)<<8);
  byte beat8 = (byte) (beat>>8);
  for(byte i = beat8+1; i < beat8+2*fade_size; i++) {
    int angle = ((i<<8) - beat) / (2*fade_size);
    leds[i] = CHSV(hue_warp+(i-beat8-fade_size)*3, 255, quadwave8((byte) angle));
  }
}


Channel sinelon = { sinelon_p, false, true, false };
Channel cylon = { cylon_p, true, true, false };
Channel cylon_rainbow = { cylon_rainbow_p, true, true, false };
