// Tweebbo for Nintendo Switch: showcase proof of concept.
// One 16:9 screen, SDL2 renderer, libnx sockets with mbedTLS; plays in the same
// room as the web, the 3DS and the Vita (protocol v6 + device byte).
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>

#include "common.h"
#include "gfx.h"
#include "net.h"
#include "show.h"
#include "world.h"

#define DATA_DIR "sdmc:/switch/Tweebbo"

u64 now_ms(void) { return armGetSystemTick() / (armGetSystemTickFreq() / 1000); }
int rnd_range(int lo, int hi) { return hi <= lo ? lo : lo + (int)((u32)rand() % (u32)(hi - lo)); }

bool store_log_available(void) { return true; }
void log_start(void);

// Settings and per-server token on the SD card (never shown on screen).
static void store_path(const char *key, char *path, int cap) {
  char safe[96];
  int n = 0;
  for (const char *k = key; *k && n < (int)sizeof safe - 1; k++)
    safe[n++] = (*k >= 'a' && *k <= 'z') || (*k >= 'A' && *k <= 'Z') || (*k >= '0' && *k <= '9') || *k == '-' || *k == '.' ? *k : '_';
  safe[n] = 0;
  snprintf(path, (size_t)cap, DATA_DIR "/%s.txt", safe);
}

bool store_load(const char *key, char *out, int cap) {
  char path[160];
  store_path(key, path, sizeof path);
  FILE *f = fopen(path, "r");
  if (!f) return false;
  int n = (int)fread(out, 1, (size_t)cap - 1, f);
  fclose(f);
  out[n > 0 ? n : 0] = 0;
  out[strcspn(out, "\r\n")] = 0;
  return true;
}

void store_save(const char *key, const char *value) {
  mkdir("sdmc:/switch", 0777);
  mkdir(DATA_DIR, 0777);
  char path[160];
  store_path(key, path, sizeof path);
  FILE *f = fopen(path, "w");
  if (!f) return;
  fputs(value, f);
  fclose(f);
}

/** D-pad and stick: one step per press with a short repeat. */
static void walk_input(PadState *pad, u64 now) {
  static u64 next;
  if (now < next) return;
  int dx = 0, dy = 0;
  u64 held = padGetButtons(pad);
  if (held & HidNpadButton_Up) dy = -1;
  else if (held & HidNpadButton_Down) dy = 1;
  else if (held & HidNpadButton_Left) dx = -1;
  else if (held & HidNpadButton_Right) dx = 1;
  else {
    HidAnalogStickState l = padGetStickPos(pad, 0);
    if (l.x > 12000) dx = 1;
    else if (l.x < -12000) dx = -1;
    else if (l.y > 12000) dy = 1;
    else if (l.y < -12000) dy = -1;
  }
  if (!dx && !dy) return;
  show_walk(dx, dy);
  next = now + 180;
}

int main(int argc, char **argv) {
  romfsInit();
  log_start();
  dbg_log("Tweebbo Switch showcase starting (v%s)", APP_VERSION);
  srand((unsigned)armGetSystemTick());
  if (!gfx_init()) {
    dbg_log("gfx_init failed");
    romfsExit();
    return 1;
  }
  show_set_platform("Nintendo Switch", "Hello from Nintendo Switch!");
  char neterr[128];
  conn_global_init(neterr, sizeof neterr);
  net_start(now_ms());
  show_start(now_ms());

  PadState pad;
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  HidTouchScreenState touch_state = {0};

  bool running = true;
  while (running && appletMainLoop()) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) running = false;
      if (ev.type == SDL_MOUSEBUTTONDOWN) {
        int ww, wh;
        SDL_GetWindowSize(SDL_GetWindowFromID(ev.button.windowID), &ww, &wh);
        show_tap(ev.button.x * (float)SCREEN_W / ww, ev.button.y * (float)SCREEN_H / wh);
      }
    }
    padUpdate(&pad);
    u64 down = padGetButtonsDown(&pad);
    if (down & HidNpadButton_A) show_action(SHOW_A);
    if (down & HidNpadButton_B) show_action(SHOW_B);
    if (down & HidNpadButton_X) show_action(SHOW_X);
    if (down & HidNpadButton_Y) show_action(SHOW_Y);
    if (down & HidNpadButton_L) show_action(SHOW_L);
    if (down & HidNpadButton_R) show_action(SHOW_R);
    if (down & HidNpadButton_Minus) show_action(SHOW_OFFLINE);
    if (down & HidNpadButton_Plus) running = false;
    walk_input(&pad, now_ms());

    if (hidGetTouchScreenStates(&touch_state, 1) && touch_state.count)
      show_tap(touch_state.touches[0].x * (float)SCREEN_W / 1280, touch_state.touches[0].y * (float)SCREEN_H / 720);

    u64 now = now_ms();
    net_update(now);
    gfx_begin();
    show_frame(now_ms());
    gfx_present();
    if (show_quit_requested()) running = false;
  }

  net_stop();
  conn_global_exit();
  gfx_exit();
  romfsExit();
  return 0;
}
