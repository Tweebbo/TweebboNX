#include <assert.h>
#include <stdarg.h>
#include <SDL.h>
#include <SDL_image.h>
static int loads;
static int texture_size = 4;
static SDL_Surface *fake_load(const char *path) {
  (void)path;
  loads++;
  return SDL_CreateRGBSurfaceWithFormat(0, texture_size, texture_size, 32, SDL_PIXELFORMAT_RGBA8888);
}
#define IMG_Load fake_load
#include "../source/nx/gfx.c"
void dbg_log(const char *fmt, ...) { (void)fmt; }
int main(void) {
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  win = SDL_CreateWindow("test", 0, 0, 64, 64, 0);
  assert(win);
  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  assert(ren);
  fill_rect(0, 0, 64, 64, 0xffffffff);
  fill_round(4, 4, 20, 10, 5, 0xff000000);
  fill_rect(0, 0, 0, 0, 0xff000000);
  Uint32 pixel=0;
  SDL_Rect outside={40,40,1,1};
  assert(SDL_RenderReadPixels(ren,&outside,SDL_PIXELFORMAT_ABGR8888,&pixel,sizeof pixel)==0);
  assert(pixel==0xffffffff);
  for (int i = 0; i < FURNI_CACHE; i++) assert(furni_get((u16)i, 0));
  assert(loads == FURNI_CACHE);
  for (int frame = 0; frame < 20; frame++)
    for (int i = 0; i < FURNI_CACHE; i++) assert(furni_get((u16)i, 0));
  assert(loads == FURNI_CACHE);
  assert(furni_get(0, 0));
  assert(furni_get(FURNI_CACHE, 0));
  assert(furni_get(0, 0));
  assert(loads == FURNI_CACHE + 1);
  assert(furni_get(1, 0));
  assert(loads == FURNI_CACHE + 2);
  for (int i = 0; i < FURNI_CACHE; i++) furni_evict(&furni_cache[i]);
  texture_size = 1024;
  for (int i = 0; i < 40; i++) {
    assert(furni_get((u16)i, 0));
    assert(furni_bytes <= FURNI_BYTES);
  }
  for (int i = 0; i < FURNI_CACHE; i++) furni_evict(&furni_cache[i]);
  assert(furni_bytes == 0);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  puts("OK: furniture textures remain cached across frames; eviction preserves recently used textures.");
}

void audio_exit(void){}
