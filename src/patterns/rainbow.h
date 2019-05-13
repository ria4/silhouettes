void rainbow_p() {
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter) {
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbow_glitter_p() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow_p();
  addGlitter(80);
}


Channel rainbow = { rainbow_p, true, false, true };
Channel rainbow_glitter = { rainbow_glitter_p, true, false, true };
