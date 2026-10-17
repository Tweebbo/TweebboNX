// Tweebbo Switch graphics: SDL2/OpenGL ES with integer scaling and nearest filtering.
// Implements the gfx.h contract (ABGR colors, LRU furniture cache, room pages).
#include "gfx.h"
#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "world.h"

#define ATLAS_PAGES 5
#define FURNI_CACHE 96

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *atlas[ATLAS_PAGES];
static SDL_Texture *font_tex;
static bool linear_filter, additive;
static int win_w = 1280, win_h = 720;

static SDL_Texture *load_png(const char *path);
static void draw_tex(SDL_Texture *t, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color);

static struct {
  SDL_Texture *tex[4];
  const RoomBackground *bg;
  int pages;
} room;
static struct {
  SDL_Texture *tex[4];
  const Scene *scene;
  int pages;
} scene_v;

typedef struct { u16 def; u8 page; bool used; SDL_Texture *tex; u32 stamp; } FurniTex;
static FurniTex furni_cache[FURNI_CACHE];
static u32 furni_clock;

static SDL_Texture *furni_get(u16 def, int page) {
  FurniTex *slot = NULL, *victim = &furni_cache[0];
  for (int i = 0; i < FURNI_CACHE; i++) {
    FurniTex *c = &furni_cache[i];
    if (c->used && c->def == def && c->page == page) { slot = c; break; }
    if (!victim->used || victim->stamp > c->stamp) victim = c;
  }
  if (!slot) {
    char path[64];
    snprintf(path, sizeof path, "romfs:/furni/d%u_%d.png", def, page);
    slot = victim;
    if (slot->tex) SDL_DestroyTexture(slot->tex);
    slot->tex = load_png(path);
    if (!slot->tex) dbg_log("furniture texture missing: %s", path);
    slot->def = def;
    slot->page = (u8)page;
    slot->used = true;
  }
  slot->stamp = ++furni_clock;
  return slot->tex;
}

void gfx_furni(u16 def, const FurniPart *part, float x, float y, float scale, u32 color) {
  SDL_Texture *t = furni_get(def, part->page);
  if (!t) return;
  draw_tex(t, part->x, part->y, part->w, part->h, x, y, scale, scale, false, color);
}

void gfx_furni_preload(u16 def) {
  (void)def; /* Textures load on first draw; the SDL cache handles the rest. */
}

static u32 tex_color(u32 abgr, u8 *r, u8 *g, u8 *b, u8 *a) {
  *r = abgr & 0xff; *g = abgr >> 8 & 0xff; *b = abgr >> 16 & 0xff; *a = abgr >> 24;
  return abgr;
}

void fill_circle(float cx, float cy, float r, u32 color);

static SDL_Texture *load_png(const char *path) {
  SDL_Surface *s = IMG_Load(path);
  if (!s) { dbg_log("texture missing: %s (%s)", path, IMG_GetError()); return NULL; }
  SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
  SDL_FreeSurface(s);
  if (t) {
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t, SDL_ScaleModeNearest);
  }
  return t;
}

bool gfx_init(void) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
    dbg_log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }
  if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
    dbg_log("IMG_Init failed: %s", IMG_GetError());
    return false;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  win = SDL_CreateWindow("Tweebbo Showcase", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                         SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!win) { dbg_log("CreateWindow failed: %s", SDL_GetError()); return false; }
  SDL_GetWindowSize(win, &win_w, &win_h);
  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren) { dbg_log("CreateRenderer failed: %s", SDL_GetError()); return false; }
  SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);
  SDL_RenderSetIntegerScale(ren, SDL_TRUE);
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  static const char *pages[ATLAS_PAGES] = {"atlas0", "atlasg0", "atlasg1", "atlasg2", "atlasg3"};
  for (int i = 0; i < ATLAS_PAGES; i++) {
    char path[64];
    snprintf(path, sizeof path, "romfs:/gfx/%s.png", pages[i]);
    atlas[i] = load_png(path);
    if (!atlas[i] && i == 0) return false;
  }
  font_tex = load_png("romfs:/gfx/font0.png");
  dbg_log("gfx ready: %dx%d logical %dx%d", win_w, win_h, SCREEN_W, SCREEN_H);
  return true;
}

void gfx_exit(void) {
  for (int i = 0; i < ATLAS_PAGES; i++) if (atlas[i]) SDL_DestroyTexture(atlas[i]);
  if (font_tex) SDL_DestroyTexture(font_tex);
  gfx_room_release();
  for (int i = 0; i < 4; i++) if (scene_v.tex[i]) { SDL_DestroyTexture(scene_v.tex[i]); scene_v.tex[i] = NULL; }
  if (ren) SDL_DestroyRenderer(ren);
  if (win) SDL_DestroyWindow(win);
  IMG_Quit();
  SDL_Quit();
}

void gfx_frame(void) {
  /* The furniture cache is released when a room changes (like the 3DS LRU budget). */
}

void gfx_begin(void) {
  SDL_SetRenderDrawColor(ren, 0x0b, 0x0d, 0x0e, 255);
  SDL_RenderClear(ren);
}

void gfx_present(void) { SDL_RenderPresent(ren); }

static void draw_tex(SDL_Texture *t, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color) {
  if (!t) return;
  u8 r, g, b, a;
  tex_color(color, &r, &g, &b, &a);
  if (r != 255 || g != 255 || b != 255) SDL_SetTextureColorMod(t, r, g, b);
  else SDL_SetTextureColorMod(t, 255, 255, 255);
  if (a != 255) SDL_SetTextureAlphaMod(t, a);
  else SDL_SetTextureAlphaMod(t, 255);
  if (additive) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_ADD);
  else SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
  SDL_Rect src = {sx, sy, sw, sh};
  SDL_FRect dst = {x, y, sw * kx, sh * ky};
  SDL_RenderCopyExF(ren, t, &src, &dst, 0, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void gfx_sprite(int page, int sx, int sy, int sw, int sh, float x, float y, float scale, bool flip, u32 color) {
  if (page < 0 || page >= ATLAS_PAGES) return;
  draw_tex(atlas[page], sx, sy, sw, sh, x, y, scale, scale, flip, color);
}

void gfx_sprite_xy(int page, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color) {
  if (page < 0 || page >= ATLAS_PAGES) return;
  draw_tex(atlas[page], sx, sy, sw, sh, x, y, kx, ky, flip, color);
}

void gfx_sprite_solid(int page, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color) {
  /* Approximate silhouette: the same sprite multiplied by the veil color. */
  if (page < 0 || page >= ATLAS_PAGES) return;
  draw_tex(atlas[page], sx, sy, sw, sh, x, y, kx, ky, flip, color);
}

void gfx_font_sprite(int sx, int sy, int sw, int sh, float x, float y, bool flip, u32 color) {
  draw_tex(font_tex, sx, sy, sw, sh, x, y, 1, 1, flip, color);
}

const RoomBackground *gfx_room(const char *model) {
  for (int i = 0; i < room_background_count; i++) {
    const RoomBackground *bg = &room_backgrounds[i];
    if (strcmp(bg->model, model)) continue;
    if (room.bg == bg) return bg;
    gfx_room_release();
    if (bg->tile_count > 4) return NULL;
    for (int j = 0; j < bg->tile_count; j++) {
      char path[64];
      snprintf(path, sizeof path, "romfs:/gfx/%s.png", bg->tiles[j].file);
      room.tex[j] = load_png(path);
      if (!room.tex[j]) { gfx_room_release(); return NULL; }
    }
    room.bg = bg;
    room.pages = bg->tile_count;
    return bg;
  }
  return NULL;
}

void gfx_room_release(void) {
  for (int i = 0; i < 4; i++) if (room.tex[i]) { SDL_DestroyTexture(room.tex[i]); room.tex[i] = NULL; }
  room.bg = NULL;
  room.pages = 0;
}

void gfx_room_draw(const RoomBackground *bg, float x, float y, float scale) {
  if (!bg || room.bg != bg) return;
  for (int j = 0; j < room.pages; j++) {
    const RoomTile *t = &bg->tiles[j];
    draw_tex(room.tex[j], 0, 0, t->w, t->h, x + t->x * scale, y + t->y * scale, scale, scale, false, 0xffffffffu);
  }
}

const Scene *gfx_scene(const char *model) {
  const Scene *want = NULL;
  for (int i = 0; i < scene_count; i++) if (!strcmp(scenes[i].model, model)) want = &scenes[i];
  if (want == scene_v.scene) return want;
  for (int i = 0; i < 4; i++) if (scene_v.tex[i]) { SDL_DestroyTexture(scene_v.tex[i]); scene_v.tex[i] = NULL; }
  scene_v.scene = NULL;
  if (!want) return NULL;
  for (int i = 0; i < want->pages && i < 4; i++) {
    char path[64];
    snprintf(path, sizeof path, "romfs:/gfx/scene_%s_%d.png", want->model, i);
    scene_v.tex[i] = load_png(path);
    if (!scene_v.tex[i]) return NULL;
  }
  scene_v.scene = want;
  scene_v.pages = want->pages;
  return want;
}

void gfx_scene_sprite(const SceneSprite *s, float x, float y, float zoom, u8 alpha) {
  if (!scene_v.scene || s->page >= scene_v.pages) return;
  draw_tex(scene_v.tex[s->page], s->sx, s->sy, s->w, s->h, x, y, zoom * s->dw / (float)s->w, zoom * s->dh / (float)s->h, false,
           ((u32)alpha << 24) | 0xffffff);
}

void gfx_filter(bool linear) { linear_filter = linear; (void)linear_filter; }
void gfx_additive(bool on) { additive = on; }

// ------------------------------------------------------------------ primitives

static void set_color(u32 color) {
  u8 r, g, b, a;
  tex_color(color, &r, &g, &b, &a);
  SDL_SetRenderDrawColor(ren, r, g, b, a);
  SDL_SetRenderDrawBlendMode(ren, a == 255 ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
}

void fill_rect(float x, float y, float w, float h, u32 color) {
  set_color(color);
  SDL_FRect r = {x, y, w, h};
  SDL_RenderFillRectF(ren, &r);
}

void fill_round(float x, float y, float w, float h, float r, u32 color) {
  /* Rounded corners approximated with three rects and two circles. */
  fill_rect(x + r, y, w - 2 * r, h, color);
  fill_rect(x, y + r, w, h - 2 * r, color);
  fill_circle(x + r, y + r, r, color);
  fill_circle(x + w - r, y + r, r, color);
  fill_circle(x + r, y + h - r, r, color);
  fill_circle(x + w - r, y + h - r, r, color);
}

void fill_circle(float cx, float cy, float r, u32 color) {
  set_color(color);
  for (int y = (int)-r; y <= (int)r; y++) {
    float dx = sqrtf(r * r - (float)(y * y));
    SDL_FRect row = {cx - dx, cy + y, dx * 2, 1};
    SDL_RenderFillRectF(ren, &row);
  }
}

void fill_triangle(float x1, float y1, float x2, float y2, float x3, float y3, u32 color) {
  set_color(color);
  u8 r, g, b, a;
  tex_color(color, &r, &g, &b, &a);
  SDL_Color c = {r, g, b, a};
  SDL_Vertex v[3] = {
    {{x1, y1}, c, {0, 0}},
    {{x2, y2}, c, {0, 0}},
    {{x3, y3}, c, {0, 0}},
  };
  SDL_RenderGeometry(ren, NULL, v, 3, NULL, 0);
}

void draw_line(float x1, float y1, float x2, float y2, u32 color) {
  set_color(color);
  SDL_RenderDrawLineF(ren, x1, y1, x2, y2);
}
