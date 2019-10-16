void rgb_p() {
  for (byte i=0; i<fade_size+1; i++) {
    leds[4+i]                                     = CRGB(200, 0, 0);
    leds[(NUM_LEDS+4-pos_shift-fade_size)/2 + i]  = CRGB(0, 255, 0);
    leds[NUM_LEDS-1-pos_shift - i]                = CRGB(0, 0, 190);
  }
}


Channel rgb = { rgb_p, false, false, false };
