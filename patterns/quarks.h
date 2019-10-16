const byte QUARKS_MAX = 64;
const byte QUARK_DEF_SIZE = 3;

typedef struct Quark {
  int pos; byte pos8;
  int a; int b;
  byte size;
  byte h; byte s;
  bool move_away = false;
};
Quark quarks[QUARKS_MAX];
byte quarks_n;

typedef struct QuarkDeco {
  Quark qk;
  byte n = 8;
  bool line[8];         // n
  bool gap[7];          // n-1
  byte gap_size[7];     // n-1
  byte size;
};
QuarkDeco qkd;


void quarks_collide_p() {
  if (channel_timer == 0) {
    start_millis = millis();
    quarks_n = QUARKS_MAX >> (2*(2-(fade_size % 3)));
    for (byte i=0; i<quarks_n; i++) {
      Quark qk;
      qk.a = fps_arr[fps_idx]/2 - random8(fps_arr[fps_idx]);
      qk.b = random8(qk.size+2, NUM_LEDS-pos_shift-qk.size-2) << 8;
      qk.size = QUARK_DEF_SIZE*2;
      qk.pos = qk.b;
      qk.pos8 = (byte) (qk.pos>>8);
      if (pos_shift % 2 == 0) { qk.h = hue_shift; }
      else { qk.h = hue_shift + 40 - random8(80); }
      qk.s = 255;
      quarks[i] = qk;
    }
  }

  FastLED.clear();

  for (byte i=0; i<quarks_n; i++) {
    if (!quarks[i].move_away) {
      if (quarks[i].pos8 <= fps_arr[fps_idx]/50) {
        quarks[i].a = random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
        quarks[i].b = quarks[i].pos - (millis() - start_millis) * quarks[i].a/4;
        quarks[i].move_away = true;
      } else if (quarks[i].pos8 >= NUM_LEDS-pos_shift-quarks[i].size-fps_arr[fps_idx]/50) {
        quarks[i].a = -random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
        quarks[i].b = quarks[i].pos - (millis() - start_millis) * quarks[i].a/4;
        quarks[i].move_away = true;
      } else if (random8() > 252 - (fps_arr[fps_idx]>>5)) {
        if (quarks[i].a > 0) { quarks[i].a = -random8(4, fps_arr[fps_idx]/2); } else { quarks[i].a = random8(4, fps_arr[fps_idx]/2); }
        quarks[i].b = quarks[i].pos - (millis() - start_millis) * quarks[i].a/4;
      }
    }

    if ((quarks[i].pos8 > 10) && (quarks[i].pos8 < NUM_LEDS-pos_shift-quarks[i].size-10)) { quarks[i].move_away = false; }

    quarks[i].pos = (millis() - start_millis) * quarks[i].a/4 + quarks[i].b;
    quarks[i].pos8 = (byte) (quarks[i].pos>>8);
    for(byte j = quarks[i].pos8; j < quarks[i].pos8+quarks[i].size; j++) {
      if (leds[j]) {
        if (channel_idx % 2 == 1) {
          leds[j] = 0;
          continue;
        }
      }
      int angle = ((j<<8) - quarks[i].pos) / quarks[i].size;
      leds[j] |= CHSV(quarks[i].h, quarks[i].s, quadwave8((byte) angle));
    }
  }
}


void qkd_change() {
  last_change = millis();
  byte change = random8(qkd.n + qkd.n-1);
  if (change < qkd.n ) {
    if (qkd.line[change] && (random8() < 64)) {
      qkd.line[change] = false;
      if (change > 0) { qkd.gap[change-1] = false; }
      if (change < qkd.n - 1) { qkd.gap[change] = false; }
    } else {
      qkd.line[change] = true;
      if ((change > 0) && qkd.line[change-1] && (random8(2) > 0)) { qkd.gap[change-1] = true; }
      if ((change < qkd.n - 1) && qkd.line[change+1] && (random8(2) > 0)) { qkd.gap[change] = true; }
    }
  } else {
    change -= qkd.n;
    if (qkd.gap[change] && (random8() > 16)) {
      qkd.gap[change] = false;
      if (random8(2) > 0) { qkd.line[change] = false; }
      if (random8(2) > 0) { qkd.line[change+1] = false; }
    } else {
      if (random8(2) > 0) { qkd.line[change] = true; qkd.line[change+1] = true; }
      if (qkd.line[change] && qkd.line[change+1]) { qkd.gap[change] = true; }
    }
  }
}

void qkd_set_color(byte j, byte c, byte v) {
  byte hue; byte coeff;
  if (fade_size % 2 == 0) { coeff = j; } else { coeff = c; }
  hue = inoise8((1+fade_size)*coeff, noise_z);
  if (fade_size % 4 < 2) { hue = hue_shift + hue/2; }
  leds[c] |= CHSV(hue, 192 + (inoise8((1+fade_size)*j, noise_z) >> 2), 255-v);
}

void quarks_deco_p() {
  if (channel_timer == 0) {
    start_millis = millis();
    qkd.qk.size = QUARK_DEF_SIZE;
    qkd.qk.a = fps_arr[fps_idx]/2 - random8(fps_arr[fps_idx]);
    qkd.qk.b = 20<<8;
    qkd.qk.pos = qkd.qk.b;
    qkd.qk.pos8 = (byte) (qkd.qk.pos>>8);
    qkd.qk.h = hue_shift;
    qkd.qk.s = 255;
    for(byte i = 0; i < qkd.n; i++) {
      if (random8(4) < 3) { qkd.line[i] = true; } else { qkd.line[i] = false; } }
    for(byte i = 0; i < qkd.n - 1; i++) {
      if (qkd.line[i] && qkd.line[i+1] && (random8(2) < 1)) { qkd.gap[i] = true; } else { qkd.gap[i] = false; } }
    for(byte i = 0; i < qkd.n - 1; i++) {
      qkd.gap_size[i] = QUARK_DEF_SIZE*4; }
    qkd.size = qkd.qk.size; for(byte i = 0; i < qkd.n - 1; i++) { qkd.size += qkd.gap_size[i]; }
  }

  FastLED.clear();

  if (!qkd.qk.move_away) {
    if (qkd.qk.pos8 <= fps_arr[fps_idx]/50) {
      qkd.qk.a = random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
      qkd.qk.b = qkd.qk.pos - (millis() - start_millis) * qkd.qk.a/4;
      qkd.qk.move_away = true;
    } else if (qkd.qk.pos8 >= NUM_LEDS-pos_shift-qkd.size-fps_arr[fps_idx]/50) {
      qkd.qk.a = -random8(fps_arr[fps_idx]/5, fps_arr[fps_idx]);
      qkd.qk.b = qkd.qk.pos - (millis() - start_millis) * qkd.qk.a/4;
      qkd.qk.move_away = true;
    }
  }

  if ((millis() - last_change > (20000/fps_arr[fps_idx])) && (random8() > 253 - (fps_arr[fps_idx]>>5))) {
    qkd_change();
  }

  if ((qkd.qk.pos8 > 20) && (qkd.qk.pos8 < NUM_LEDS-pos_shift-qkd.size-20)) { qkd.qk.move_away = false; }

  qkd.qk.pos = (millis() - start_millis) * qkd.qk.a/4 + qkd.qk.b;
  qkd.qk.pos8 = (byte) (qkd.qk.pos>>8);
  byte pos8 = qkd.qk.pos8;
  for(byte i = 0; i < qkd.n; i++) {
    if (qkd.line[i]) {
      for(byte j = 0; j < 3; j++) {
        qkd_set_color(j, pos8+j, abs(1-j)*120);
      }
    }
    if (i != qkd.n - 1) {
      if (qkd.line[i] && qkd.line[i+1] && qkd.gap[i]) {
        for(byte j = 0; j < qkd.gap_size[i]; j++) {
          qkd_set_color(j, pos8+1+j, 0);
        }
      }
      pos8 += qkd.gap_size[i];
    }
  }
  noise_z += 5;
}


Channel quarks_collide = { quarks_collide_p, false, false, false };
Channel quarks_deco = { quarks_deco_p, false, false, false };
