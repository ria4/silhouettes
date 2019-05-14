#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      143

#define BUTTON_PIN    2
#define IRRCV_PIN     9
#define STRIP_PIN     6

#define FPS_MODES_NBR         5
#define CHANGE_SIG_LENGTH     50

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


CRGB leds[NUM_LEDS];
byte leds_hue[NUM_LEDS];

struct Channel {
  void (*pattern)();
  bool self_refresh;
  bool fade_init;
  bool fade_edges;
};

byte channels_nbr;
byte channel_idx = 0;
byte channel_idx_tmp;

IRrecv irrecv(IRRCV_PIN);
decode_results results;
char ir_buffer[3];
byte curr_char_idx = 0;

bool pause = false;
byte channel_timer = 0;

const int fps_arr[FPS_MODES_NBR] = { 3, 12, 50, 200, 1000 };
// the refresh rate may cap by itself

byte pos_shift = 0;
byte hue_shift = 0;
byte hue_warp = 0;
byte fade_size = 0;

byte brightness = 15;
byte fps_idx = 2;

static uint16_t noise_z;

unsigned long start_millis;
unsigned long last_change;
unsigned long next_change;


void faneInit();
byte fadeEdgesPx(byte min_val, byte i);
void fadeEdges();
byte neighboursUp(byte i);
int freeRam();
void resetLedsHue();
void noisePx(byte i, byte hue_shift2=0);
