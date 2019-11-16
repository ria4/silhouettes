void point_p() {
  setToColorShift(pos_shift);
}

void points_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (i%(pos_shift+1) == 0) {
      setToColorShift(i);
    }
  }
}

void points_gradient_p() {
  CHSV color_start, color_end;

  if (crgb_shift_active) {
    color_start = rgb2hsv_approximate(crgb_shift);
    if (color_bis_active) {
      color_end = rgb2hsv_approximate(crgb_shift_bis);
    } else {
      color_end = CHSV(color_start.hue + (NUM_LEDS-pos_shift)/5, 255, 255);
    }
  } else {
    color_start = CHSV(hue_shift, 255, 255);
    if (color_bis_active) {
      color_end = CHSV(hue_shift_bis, 255, 255);
    } else {
      color_end = CHSV(hue_shift + (NUM_LEDS-pos_shift)/5, 255, 255);
    }
  }
  fill_gradient(leds, NUM_LEDS-pos_shift, color_start, color_end);

  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (i%(fade_size+1) != 0) {
      leds[i] = 0;
    }
  }
}

void points_gradient_balanced_p() {
  CHSV color_start, color_end;
  color_start = CHSV(hue_shift, 255, 115);
  if (color_bis_active) {
    color_end = CHSV(hue_shift_bis, 255, 255);
  } else {
    color_end = CHSV(hue_shift + (NUM_LEDS-pos_shift)/5, 255, 115 + (NUM_LEDS-pos_shift)*7/8);
  }
  fill_gradient(leds, NUM_LEDS-pos_shift, color_start, color_end);

  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    if (i%(fade_size+1) != 0) {
      leds[i] = 0;
    }
  }
}


Channel point = { point_p, false, false, false };
Channel points = { points_p, false, true, false };
Channel points_gradient = { points_gradient_p, false, true, false };
Channel points_gradient_balanced = { points_gradient_balanced_p, false, true, false };
