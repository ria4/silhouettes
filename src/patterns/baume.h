byte split_start;
byte split_size;
byte split_hue_top;

void baume_p() {
  byte middle = (NUM_LEDS-pos_shift)/2;

  split_start = middle - 52 + inoise8(20, noise_z)/3;
  split_size = inoise8(20, 2*noise_z)/6;
  if (split_size % 2 == 1) { split_size += 1; }
  split_hue_top = hue_shift - 85 + inoise8(20, 3*noise_z)/3;

  fill_gradient(leds, split_start, CHSV(hue_shift, 255, 255), CHSV(split_hue_top, 255, 255));
  fill_gradient(&(leds[split_start]), split_size/2, CHSV(split_hue_top, 255, 255), CHSV(split_hue_top, 0, 0));
  fill_gradient(&(leds[split_start+split_size/2]), split_size/2, CHSV(split_hue_top, 0, 0), CHSV(split_hue_top, 255, 255));
  fill_gradient(&(leds[split_start+split_size]), NUM_LEDS-pos_shift-split_size-split_start, CHSV(split_hue_top, 255, 255), CHSV(hue_shift, 255, 255));

  noise_z += 10;
}


Channel baume = { baume_p, false, false, true };
