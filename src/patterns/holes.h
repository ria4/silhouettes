bool hole[20];
byte hole_n;
byte hole_size;
byte holes_hue_shift;

void holes_change() {
  last_change = millis();
  for (byte i = 0; i < hole_n; i++) {
    if (random8(2) < 1) {
      hole[i] = true;
    } else {
      hole[i] = false;
    }
  }
}

void holes_p() {
  if (channel_timer == 0) {
    if (fade_size % 3 == 0) { hole_n = 20; hole_size = 6; }
    else if (fade_size % 3 == 1) { hole_n = 14; hole_size = 9; }
    else { hole_n = 10; hole_size = 13; }
    holes_hue_shift = 0;
    /*if (channel_idx % 2 == 0) { holes_hue_shift = 0; }
    else { holes_hue_shift = 128; }*/
    holes_change();
  }

  FastLED.clear();

  if (millis() - last_change > 50000/fps_arr[fps_idx]) {
    holes_change();

    /*if (channel_idx % 2 == 1) {
      for(byte i = 0; i < hole_n*(hole_size+1)+1; i++) {
        noisePx(i, holes_hue_shift);
      }
      FastLED.show();
      delay(5000/fps_arr[fps_idx]);
      return;
    }*/
  }

  byte start = 0;
  for(byte i = 0; i < hole_n; i++) {
    noisePx(start, holes_hue_shift);
    if (!hole[i]) {
      for(byte j = start+1; j < start+1+hole_size; j++) {
        noisePx(j);
      }
    }
    start += hole_size + 1;
  }
  noisePx(hole_n*(hole_size+1), holes_hue_shift);
  noise_z += 15;
}


Channel holes = { holes_p, false, false, false };
