void pulse_p() {
  byte beat = beatsin8(fps_arr[fps_idx], 0, 255);
  fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, beat));
  //fill_solid(&(leds[pos_shift]), NUM_LEDS-pos_shift, CHSV(hue_shift, 255, beat));
}

Channel pulse = { false, false, pulse_p };
