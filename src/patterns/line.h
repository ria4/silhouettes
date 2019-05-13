void line_p() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i].setHue(hue_shift);
  }
}

void line_colors_p() {
  hue_shift = 255/6 * ((channel_idx-1) % 6);
  for(byte i = pos_shift; i < NUM_LEDS; i++) {
    leds[i].setHue(hue_shift);
  }
}


Channel line = { line_p, false, true, true };
Channel line_colors = { line_colors_p, false, false, true };
