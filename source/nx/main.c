// Tweebbo for Nintendo Switch: local development port.
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
#include "client_pool.h"
#include "client_ui.h"
#include "world.h"
#include "update.h"

void store_start(void);
void store_stop(void);

u64 now_ms(void) { return armGetSystemTick() / (armGetSystemTickFreq() / 1000); }
int rnd_range(int lo, int hi) { return hi <= lo ? lo : lo + (int)((u32)rand() % (u32)(hi - lo)); }

bool store_log_available(void) { return true; }
void log_start(void);
void log_stop(void);

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
    else if (l.y > 12000) dy = -1;
    else if (l.y < -12000) dy = 1;
  }
  if (!dx && !dy) return;
  show_walk(dx, dy);
  next = now + (pool_mode()==2 ? 80 : 180);
}

int main(int argc, char **argv) {
  update_set_launch_path(argc > 0 ? argv[0] : NULL);
  log_start();
  store_start();
  if (R_FAILED(romfsInit())) {
    dbg_log("romfsInit failed");
    store_stop();
    log_stop();
    return 1;
  }
  dbg_log("Tweebbo Switch development client starting (v%s)", APP_VERSION);
  srand((unsigned)armGetSystemTick());
  if (!gfx_init()) {
    dbg_log("gfx_init failed");
    gfx_exit();
    store_stop();
    log_stop();
    romfsExit();
    return 1;
  }
  show_set_platform("Nintendo Switch", "Hello from Nintendo Switch!");
  show_set_offline_key("Minus");
  char neterr[128];
  conn_global_init(neterr, sizeof neterr);
  net_start(now_ms());
  show_start(now_ms());

  PadState pad;
  padConfigureInput(1, HidNpadStyleSet_NpadFullCtrl);
  padInitializeDefault(&pad);
  hidInitializeTouchScreen();
  HidTouchScreenState touch_state = {0};
  bool touching = false;

  bool running = true;
  while (running && appletMainLoop()) {
    bool tv = appletGetOperationMode() == AppletOperationMode_Console;
    client_ui_set_tv(tv);
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) running = false;
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
    if ((down & HidNpadButton_Plus) && show_confirm_quit("+", now_ms())) running = false;
    walk_input(&pad, now_ms());

    bool touched = !tv && hidGetTouchScreenStates(&touch_state, 1) && touch_state.count;
    if (touched && !touching)
      show_tap(touch_state.touches[0].x * (float)SCREEN_W / 1280, touch_state.touches[0].y * (float)SCREEN_H / 720);
    touching = touched;

    u64 now = now_ms();
    net_update(now);
    client_ui_update();
    if (update_exit_requested()) running = false;
    gfx_begin();
    show_frame(now_ms());
    gfx_present();
    if (show_quit_requested()) running = false;
  }

  net_stop();
  conn_global_exit();
  gfx_exit();
  store_stop();
  log_stop();
  romfsExit();
  return 0;
}
