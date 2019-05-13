#define NUM_LEDS    143

#define FPS_MODES_NBR       5

CRGB leds[NUM_LEDS];

struct Channel {
  void (*pattern)();
  bool fade_in;
  bool auto_refresh;
};

const int fps_arr[FPS_MODES_NBR] = { 3, 12, 50, 200, 1000 };      // the frame rate may cap by itself

byte pos_shift = 0;
byte hue_shift = 0;

byte fps_idx = 2;
