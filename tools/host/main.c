// Desktop harness of the Switch client: the same shared core (show.c, net.c, world.c, avatar.c)
// and the same SDL2 renderer (source/nx/gfx.c) as the console build, with a keyboard in place
// of the Joy-Con and POSIX sockets in place of bsd:u. It exists to test the Switch code path
// against a hotel on this PC; it is not a product.
//   Keys: arrows walk, Z = A, X = B, A = X, S = Y, Q/W = L/R, O = Minus (offline), Escape quits.
#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "common.h"
#include "gfx.h"
#include "net.h"
#include "show.h"
#include "world.h"

void gfx_screenshot(const char *path);

static const char *data_dir = "host-data";

u64 now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000 + (u64)ts.tv_nsec / 1000000;
}
int rnd_range(int lo, int hi) { return hi <= lo ? lo : lo + (int)((u32)rand() % (u32)(hi - lo)); }
bool store_log_available(void) { return true; }
void log_start(void) {}

void dbg_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

static void store_path(const char *key, char *path, int cap) {
  char safe[96];
  int n = 0;
  for (const char *k = key; *k && n < (int)sizeof safe - 1; k++)
    safe[n++] = (*k >= 'a' && *k <= 'z') || (*k >= 'A' && *k <= 'Z') || (*k >= '0' && *k <= '9') || *k == '-' || *k == '.' ? *k : '_';
  safe[n] = 0;
  snprintf(path, (size_t)cap, "%s/%s.txt", data_dir, safe);
}

bool store_load(const char *key, char *out, int cap) {
  char path[256];
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
  mkdir(data_dir, 0777);
  char path[256];
  store_path(key, path, sizeof path);
  FILE *f = fopen(path, "w");
  if (!f) return;
  fputs(value, f);
  fclose(f);
}

int main(int argc, char **argv) {
  if (argc > 1) data_dir = argv[1];
  mkdir(data_dir, 0777);
  srand((unsigned)now_ms());
  if (!gfx_init()) return 1;
  show_set_platform("Nintendo Switch", "Hello from Nintendo Switch!");
  show_set_offline_key("O");
  char neterr[128];
  conn_global_init(neterr, sizeof neterr);
  net_start(now_ms());
  show_start(now_ms());
  const char *shot = getenv("SHOT"), *shot_ms = getenv("SHOT_MS"), *quit_ms = getenv("QUIT_MS");
  u64 start = now_ms(), next_step = 0;
  bool running = true, shot_done = false;
  while (running) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) running = false;
      if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
        switch (ev.key.keysym.sym) {
          case SDLK_z: show_action(SHOW_A); break;
          case SDLK_x: show_action(SHOW_B); break;
          case SDLK_a: show_action(SHOW_X); break;
          case SDLK_s: show_action(SHOW_Y); break;
          case SDLK_q: show_action(SHOW_L); break;
          case SDLK_w: show_action(SHOW_R); break;
          case SDLK_o: show_action(SHOW_OFFLINE); break;
          case SDLK_ESCAPE: running = false; break;
        }
      }
    }
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    u64 now = now_ms();
    if (now >= next_step) {
      int dx = k[SDL_SCANCODE_RIGHT] - k[SDL_SCANCODE_LEFT], dy = k[SDL_SCANCODE_DOWN] - k[SDL_SCANCODE_UP];
      if (dx || dy) { show_walk(dx, dy); next_step = now + 180; }
    }
    /* Test hook: OFFLINE_MS presses Minus once at that time (the offline toggle). */
    static bool pressed;
    const char *press = getenv("OFFLINE_MS");
    if (press && *press && !pressed && now - start >= (u64)atoll(press)) { pressed = true; dbg_log("host: Minus pressed"); show_action(SHOW_OFFLINE); }
    net_update(now);
    gfx_frame();
    gfx_begin();
    show_frame(now_ms());
    if (shot && !shot_done && shot_ms && now - start >= (u64)atoll(shot_ms)) {
      shot_done = true;
      gfx_screenshot(shot);
    }
    gfx_present();
    if (quit_ms && now - start >= (u64)atoll(quit_ms)) running = false;
    if (show_quit_requested()) running = false;
  }
  net_stop();
  conn_global_exit();
  gfx_exit();
  return 0;
}
