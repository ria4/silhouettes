void window_p() {
  for (byte i = 0; i < NUM_LEDS; i++) {
    if ((channel_timer < 64) || (channel_timer > 128) ||
        (i < NUM_LEDS/2-pos_shift) ||
        (i > NUM_LEDS/2+pos_shift)) {
      leds[i] = CHSV(hue_shift - 64 + (inoise8((1+fade_size)*i, noise_z*2) >> 2), 255, 128 + (inoise8((1+fade_size)*i, noise_z) >> 1));
    } else {
      leds[i] = 0;
    }
  }
  noise_z += 15;
}


Channel window = { window_p, false, false, false };
