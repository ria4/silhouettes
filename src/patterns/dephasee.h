void dephasee_p()
{
  // random colored splashes that fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 1);
  if (random8() < 4) {
    byte size;
    if (pos_shift == 0) { size = 20; }
    else { size = min(pos_shift, 140); }
    byte pos = random8(NUM_LEDS-1 - size);

    bool overlap = false;
    for(byte i = pos; i < pos+size; i++) {
      overlap |= leds[i];
    }

    if (!overlap) {
      for(byte i = pos; i < pos+size; i++) {
        byte theta = (pos+(size/2)-i) * 255 / (2*size);
        byte val = cos8(theta) - 1;
        // library bug? val cannot be 255 below
        leds[i] = CHSV(hue_shift, 230, val);
      }
    }
  }
}


Channel dephasee = { dephasee_p, false, false, false };
