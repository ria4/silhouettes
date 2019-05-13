#define SD_READ

#include "SD.h"

File silhouette;
char silhouette_name[7];
byte sd_idx;
byte sd_params;
bool sd_loop;
int sd_width;
int sd_frames_nbr;
int sd_frame_idx;
byte r, g, b;


bool set_sd_silhouette(byte n) {
  if (silhouette) { silhouette.close(); }
  sprintf(silhouette_name, "%03d", n);
  strcpy(silhouette_name + 3, ".SIL");
  silhouette = SD.open(silhouette_name, FILE_READ);
  if (silhouette) {
    #ifdef DEBUG
    Serial.print(F("Opened "));
    Serial.println(silhouette_name);
    #endif
    sd_frame_idx = 0;
    sd_idx = n;
    sd_params = silhouette.read();
    sd_loop = (sd_params >> 7);
    sd_width = (silhouette.read() << 8) + silhouette.read();
    sd_frames_nbr = (silhouette.read() << 8) + silhouette.read();
    return true;
  } else {
    #ifdef DEBUG
    Serial.print(F("Failed to open "));
    Serial.println(silhouette_name);
    #endif
    return false;
  }
}


void read_sd()
{
  if ((sd_idx == 0) || (!silhouette && !set_sd_silhouette(sd_idx))) {
    channel_idx = 1;
    return;
  }

  //if (silhouette) { return; }

  #ifdef DEBUG
  Serial.print(F("Available from SD: "));
  Serial.println(silhouette.available());
  #endif
  if (silhouette && silhouette.available()) {
    #ifdef DEBUG
    Serial.print(F("Remaining SRAM: "));
    Serial.println(freeRam());
    #endif
    for (int j=0; j<sd_width; j++) {
      r = silhouette.read();
      g = silhouette.read();
      b = silhouette.read();
      if (fade_size == 0) {
        leds[j] = CRGB(r, g, b);
        // note that CRGB class would first retrieve b, then g, then r
      } else {
        leds[NUM_LEDS-1-pos_shift - j] = CRGB(r, g, b);
      }
    }
    sd_frame_idx++;
  }

  if (sd_frame_idx == sd_frames_nbr) {
    sd_frame_idx = 0;
    if (sd_loop) {
      silhouette.seek(5);
    } else {
      silhouette.close();
      FastLED.clear();
      FastLED.show();
      pause = true;
    }
  }
}
