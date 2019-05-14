bool leds_glitch[NUM_LEDS];

void glitch_reset_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (leds_hue[i] == hue_shift) {
      if (random8() < 10) {
        leds_hue[i] = random8();
        leds_hue[i] -= (leds_hue[i] - hue_shift) % 8;
        leds[i].setHue(leds_hue[i]);
      }
    } else {
      leds_hue[i] -= 8;
      leds[i].setHue(leds_hue[i]);
    }
  }
}

void glitch_reset_once_p() {
  if (channel_timer == 0) {
    for (byte i=0; i<NUM_LEDS-pos_shift; i++) { leds_glitch[i] = false; }
    fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, 255));
  }

  byte i = 0;
  while ((i < NUM_LEDS-pos_shift) && leds_glitch[i] && (leds_hue[i] == hue_shift)) { i+=1; }

  if (i == NUM_LEDS-pos_shift) {
    for (byte i=0; i<NUM_LEDS-pos_shift; i++) { leds_glitch[i] = false; }
    return;
  }

  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (!leds_glitch[i]) {
      if (random8() > 240) {
        leds_hue[i] = hue_shift + 128;
        leds[i].setHue(leds_hue[i]);
        leds_glitch[i] = true;
      }
    } else {
      if (leds_hue[i] != hue_shift) {
        leds_hue[i] += max(1, fps_arr[fps_idx]/50);
        leds[i].setHue(leds_hue[i]);
      }
    }
  }
}


Channel glitch_reset = { glitch_reset_p, false, false, true };
Channel glitch_reset_once = { glitch_reset_once_p, false, false, true };
