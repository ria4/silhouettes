byte heartbeat_r;
byte heartbeat_b;

void heartbeat_p() {
  FastLED.clear();

  if (channel_timer == 0) {
    last_change = millis();
    next_change = 872 + random8();
  }

  if (millis() - last_change < next_change) {

    leds[44] = CRGB(255-(millis()-last_change)/4, 0, (millis()-last_change)/5);
    heartbeat_r = leds[44].r;
    heartbeat_b = leds[44].b;

  } else {

    uint32_t d = millis();
    while (millis() - d < 290) {
      FastLED.clear();

      byte i = abs(beat8(200, d+100) - 128);
      for (byte k=i; k<i+15; k++) {
        int angle = ((k - i) << 8)/ 15;
        leds[k] = CRGB(heartbeat_r, 0, heartbeat_b);
        leds[k].nscale8_video(128 + quadwave8((byte) angle)/2);
      }
      if (heartbeat_r != 255) { heartbeat_r += min(20, 255-heartbeat_r); }
      if (heartbeat_b !=   0) { heartbeat_b -= min(20, heartbeat_b); }
      FastLED.show();
      delay(5);
    }

    last_change = millis();
    next_change = 936 + random8()/2;

  }
}


Channel heartbeat = { heartbeat_p, true, false, false };
