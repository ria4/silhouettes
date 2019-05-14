void hue_anchor_p() {
  leds[0].setHue(hue_shift);
  byte curr_hue = hue_shift;
  for(byte i = 1; i < NUM_LEDS-pos_shift; i++) {
    curr_hue += 7 - random8(15);
    leds[i].setHue(curr_hue);
  }
}

void hue_drift_p() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].setHue(leds_hue[i]);
    leds_hue[i] += 12 - random8(25);
  }
}


Channel hue_anchor = { hue_anchor_p, false, false, true };
Channel hue_drift = { hue_drift_p, false, false, true };
