void dephasee_p() {
  if (random8() < 220) {
    for (byte i = 0; i < NUM_LEDS-pos_shift; i++) {
      leds[i].fadeToBlackBy(1);
    }
  }

  for (byte i = 1; i < NUM_LEDS-pos_shift-1; i++) {
    int r = random16();
    if ((r < 128) && (r > 3)) {
      for (int j = i; j <= min(NUM_LEDS-pos_shift-1, i + r/8); j++) {
        leds[j].setHue(hue_shift - 96 + inoise8(5*i, noise_z)/4*3);
      }
    }
  }
  noise_z += 10;
}


Channel dephasee = { dephasee_p, false, false, false };
