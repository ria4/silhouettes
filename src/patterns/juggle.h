void juggle_p() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);

  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    int jpulse = (i+13)/3;
    leds[beatsin16( jpulse, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}


Channel juggle = { juggle_p, false, true, false };
