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

void points_gradient_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (i%(fade_size+1) == 0) {
      leds[i] = CHSV(hue_shift+i/5, 255, 115+i*7/8);
    }
  }
}


Channel point = { point_p, false, false, false };
Channel points = { points_p, false, true, false };
Channel points_gradient = { points_gradient_p, false, true, false };
