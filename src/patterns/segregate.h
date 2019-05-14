void segregate_p() {
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


Channel segregate = { segregate_p, false, true, false };
