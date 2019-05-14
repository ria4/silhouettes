void dual_p() {
  if (channel_timer == 0) {
    if (channel_idx % 2 == 0) {
      fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, 255));
    } else {
      fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift+70, 255, 255));
      for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
        leds_hue[i] = hue_shift+70;
      }
    }
  }

  if (channel_idx % 2 == 0) {
    if (leds_hue[3] != hue_shift+70) {
      for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
        if (i % 6 >= 3) {
          leds_hue[i] += 1;
          leds[i].setHue(leds_hue[i]);
        }
      }
    }
  } else {
    if (leds_hue[0] != hue_shift) {
      for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
        if (i % 6 < 3) {
          leds_hue[i] -= 1;
          leds[i].setHue(leds_hue[i]);
        }
      }
    }
  }
}


void dual_rand_p() {
  if (channel_timer == 0) {
    fill_solid(leds, NUM_LEDS-pos_shift, CHSV(hue_shift, 255, 255));
  }

  for (byte i=0; i<NUM_LEDS-pos_shift-3; i=i+3) {
    if (random8() < 4) {
      if (leds_hue[i] == hue_shift) {
        for (byte j=0; j<3; j++) {
          leds_hue[i+j] = hue_shift+70;
          leds[i+j].setHue(hue_shift+70);
        }
      } else {
        for (byte j=0; j<3; j++) {
          leds_hue[i+j] = hue_shift;
          leds[i+j].setHue(hue_shift);
        }
      }
    }
  }
}


Channel dual = { dual_p, false, false, true };
Channel dual_rand = { dual_rand_p, false, false, true };
