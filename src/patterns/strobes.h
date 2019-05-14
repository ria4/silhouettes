byte strobes_cnt;;

void strobes_p() {

  if ( random8() < 20 ) {
    byte size;
    switch((pos_shift / 10) % 10) {
      case 1:
        size = 1; break;
      case 2:
        size = 25; break;
      case 3:
        size = NUM_LEDS; break;
      default:
        size = 25 + (7 - random8(14));
    }

    byte hue;
    switch(pos_shift / 100) {
      case 1:
        hue = hue_shift + (30 - random8(60)); break;
      case 2:
        hue = random8(); break;
      default:
        hue = hue_shift;
    }

    byte start = random8(NUM_LEDS - size);
    fill_solid(&(leds[start]), size, CHSV(hue, 255, 255));
    FastLED.show();

    byte del;
    switch(pos_shift % 10) {
      case 1:
        del = 2; break;
      case 2:
        del = 15; break;
      case 3:
        del = 500; break;
      default:
        del = 50;
    }
    delay(del);

    FastLED.clear();
  }
}

void strobes_rotate_colors_p() {
  delay(4000);
  byte h = 255/6 * strobes_cnt;
  fill_solid(leds, NUM_LEDS-pos_shift, CHSV(h, 255, 255));
  strobes_cnt = (strobes_cnt + 1) % 6;
  FastLED.show();
  delay(500);
  FastLED.clear();
}


Channel strobes = { strobes_p, false, false, false };
Channel strobes_rotate_colors = { strobes_rotate_colors_p, true, false, true };
