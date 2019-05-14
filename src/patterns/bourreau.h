void bourreau_p() {
  if (channel_timer == 0) {
    uint16_t r = random16();
    for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
      leds[i].setHue(hue_shift - 32 + inoise8(10*i, r)/4);
    }
  }

  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    if (leds[i]) {
      byte neighbours = neighboursUp(i);
      if (((neighbours == 0) && (random8() < 16)) ||
          ((neighbours == 1) && (random8() < 8)) ||
          ((neighbours == 2) && (random16() < 32))) {
        leds[i] = 0;
      }
    }
  }
}


Channel bourreau = { bourreau_p, false, false, false };
