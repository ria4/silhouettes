void point_p() {
  leds[pos_shift].setHue(hue_shift);
}

void points_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (i%(pos_shift+1) == 0) {
      leds[i].setHue(hue_shift);
    }
  }
}


Channel point = { point_p, false, false, false };
Channel points = { points_p, false, true, false };
