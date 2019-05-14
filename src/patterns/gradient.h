void gradient_p() {
  byte beat = beatsin8(fps_arr[fps_idx], 0, 100);
  fill_gradient(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+beat*2, 255, 255), CHSV(hue_shift+beat+50, 255, 255));
}


Channel gradient = { gradient_p, false, true, true };
