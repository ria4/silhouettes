void point_p() {
  leds[pos_shift].setHue(hue_shift);
}


Channel point = { point_p, false, false, false };
