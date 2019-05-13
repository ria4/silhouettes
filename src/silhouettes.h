#define NUM_LEDS    143

#define FPS_MODES_NBR       5

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


CRGB leds[NUM_LEDS];

struct Channel {
  bool fade_in;
  bool auto_refresh;
  void (*pattern)();
};

byte channels_nbr;
byte channel_idx = 0;
byte channel_idx_tmp;

bool pause = false;
byte channel_timer = 0;

const int fps_arr[FPS_MODES_NBR] = { 3, 12, 50, 200, 1000 };      // the frame rate may cap by itself

byte pos_shift = 0;
byte hue_shift = 0;
byte fade_size = 0;

byte fps_idx = 2;
