void line_p() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    setToColorShift(i);
  }
}

byte line_cs[] = { 0, 33, 155, 188 };
byte line_c;

void line_colors_p() {
  line_c = line_cs[(channel_idx-1) % ARRAY_SIZE(line_cs)];
  for(byte i = pos_shift; i < NUM_LEDS; i++) {
    leds[i].setHue(line_c);
  }
}


Channel line = { line_p, false, true, true };
Channel line_colors = { line_colors_p, false, false, true };
