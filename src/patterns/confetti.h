void confetti_p()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int test = random8();
  if (test > 248) {
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(hue_warp + random8(64), 200, 255);
  }
}


Channel confetti = { confetti_p, false, false, false };
