double sqi;

void noise_p() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    noise_px(i);
  }
  noise_z += 15;
}

void noise_fade_out_p() {
  for(byte i = 0; i < NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift - 64 + (inoise8((1+fade_size)*i, noise_z*2) >> 2), 255, 64 + (inoise8((1+fade_size)*i, noise_z) >> 2)*3);
    leds[i].nscale8(255-channel_timer);
  }
  noise_z += 15;
}

void noise_conditional_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    byte r = inoise8((1+fade_size)*2*i, noise_z);
    if (r < 128) {
      leds[i] = CHSV(hue_shift, 255, 255);
    } else {
      leds[i] = CHSV(hue_shift + (175-r)*2, 255, 255);
    }
  }
  noise_z += 15;
}

void noise_log_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    sqi = sqrt(i);
    leds[i] = CHSV(hue_shift + inoise8((1+fade_size)*i*sqi*sqrt(sqi), noise_z)/3*2, 255, 255);
  }
  noise_z += 15;
}

void noise_log_full_color_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(inoise8(1+fade_size, noise_z*i/16), 255, 255);
  }
  noise_z += 2;
}


void noise_transfer_p() {
  for (byte i=0; i<NUM_LEDS-pos_shift; i++) {
    leds[i] = CHSV(hue_shift + inoise8(1+fade_size, noise_z*i/16)/2, 255, 255);
  }
  noise_z += 2;
}


Channel noise = { noise_p, false, true, true };
Channel noise_fade_out = { noise_fade_out_p, false, true, true };
Channel noise_conditional = { noise_conditional_p, false, true, true };
Channel noise_log = { noise_log_p, false, true, true };
Channel noise_log_full_color = { noise_log_full_color_p, false, true, true };
Channel noise_transfer = { noise_transfer_p, false, true, true };
