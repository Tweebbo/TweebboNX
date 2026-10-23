// Tweebbo Switch graphics: SDL2/OpenGL ES with integer scaling and nearest filtering.
// Implements the gfx.h contract (ABGR colors, LRU furniture cache, room pages).
#include "gfx.h"
#include "audio.h"
#include "gen/snow_assets.h"
#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "world.h"
#include "finishes.h"
#include "client_shop.h"
#include "photo_pixels.h"

#define ATLAS_MAX 16
#define FURNI_CACHE 512
#define FURNI_BYTES (64u * 1024 * 1024)

static SDL_Window *win;
static SDL_Renderer *ren;
static u8 *photo_pending;
static int photo_status;
static SDL_Texture *photo_texture;
void gfx_photo_request(u8 *gray){photo_pending=gray;photo_status=0;}
int gfx_photo_status(void){return photo_status;}
bool gfx_photo_preview(const u8 *gray) {
  if(!photo_texture)photo_texture=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,322,234);
  if(!photo_texture)return false;
  void *pixels;int pitch;if(SDL_LockTexture(photo_texture,NULL,&pixels,&pitch))return false;
  for(int y=0;y<234;y++)for(int x=0;x<322;x++)((u32*)((u8*)pixels+y*pitch))[x]=photo_duotone(gray[y*322+x]);
  SDL_UnlockTexture(photo_texture);return true;
}
void gfx_photo_draw(float x,float y,float w,float h){SDL_FRect rect={x,y,w,h};if(photo_texture)SDL_RenderCopyF(ren,photo_texture,NULL,&rect);}
static SDL_Texture *atlas[ATLAS_MAX];
static SDL_Texture *font_tex;
static SDL_Texture *pet_tex[64];
static float view_x,view_y,view_kx=1,view_ky=1;
void gfx_sprite_transform(float x,float y,float kx,float ky){view_x=x;view_y=y;view_kx=kx;view_ky=ky;}
static bool linear_filter, additive;
static int win_w = 1280, win_h = 720;

static SDL_Texture *load_png(const char *path);
static void draw_tex(SDL_Texture *t, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color);

static struct {
  SDL_Texture *tex[4];
  const RoomBackground *bg;
  int pages;
  char wall[9], floor[9];
  bool finish_attempted;
} room;
static void room_finish(void);
static struct {
  SDL_Texture *tex[4];
  const Scene *scene;
  int pages;
} scene_v;

typedef struct { u16 def; u8 page; bool used; SDL_Texture *tex; u32 stamp; size_t bytes; } FurniTex;
static FurniTex furni_cache[FURNI_CACHE];
static u32 furni_clock;
static size_t furni_bytes;

static void furni_evict(FurniTex *slot) {
  if (slot->tex) SDL_DestroyTexture(slot->tex);
  furni_bytes -= slot->bytes;
  memset(slot, 0, sizeof *slot);
}

static SDL_Texture *furni_get(u16 def, int page) {
  FurniTex *slot = NULL, *victim = &furni_cache[0];
  for (int i = 0; i < FURNI_CACHE; i++) {
    FurniTex *c = &furni_cache[i];
    if (c->used && c->def == def && c->page == page) { slot = c; break; }
    if (victim->used && (!c->used || c->stamp < victim->stamp)) victim = c;
  }
  if (!slot) {
    char path[64];
    snprintf(path, sizeof path, "romfs:/furni/d%u_%d.png", def, page);
    slot = victim;
    furni_evict(slot);
    slot->tex = load_png(path);
    if (slot->tex) {
      int w, h;
      SDL_QueryTexture(slot->tex, NULL, NULL, &w, &h);
      slot->bytes = (size_t)w * h * 4;
      while (furni_bytes + slot->bytes > FURNI_BYTES && furni_bytes) {
        FurniTex *oldest = NULL;
        for (int i = 0; i < FURNI_CACHE; i++) {
          FurniTex *c = &furni_cache[i];
          if (c != slot && c->used && c->bytes && (!oldest || c->stamp < oldest->stamp)) oldest = c;
        }
        if (!oldest) break;
        furni_evict(oldest);
      }
      furni_bytes += slot->bytes;
    }
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
  win = SDL_CreateWindow("Tweebbo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                         SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!win) { dbg_log("CreateWindow failed: %s", SDL_GetError()); return false; }
  SDL_GetWindowSize(win, &win_w, &win_h);
  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren) { dbg_log("CreateRenderer failed: %s", SDL_GetError()); return false; }
  SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);
  SDL_RenderSetIntegerScale(ren, SDL_TRUE);
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  // The export decides the atlas pages (atlas0..N in RGBA8, atlasg0..N in LA8).
  for (int i = 0; i < atlas_page_count && i < ATLAS_MAX; i++) {
    char path[64];
    snprintf(path, sizeof path, "romfs:/gfx/%s.png", atlas_page_files[i]);
    atlas[i] = load_png(path);
    if (!atlas[i]) return false;
  }
  font_tex = load_png("romfs:/gfx/font0.png");
  if (!font_tex) return false;
  dbg_log("gfx ready: %dx%d logical %dx%d", win_w, win_h, SCREEN_W, SCREEN_H);
  return true;
}

void gfx_exit(void) {
  if(photo_texture){SDL_DestroyTexture(photo_texture);photo_texture=NULL;}
  audio_exit();
  gfx_snow_free();
  for(int i=0;i<COUNT(pet_tex);i++)if(pet_tex[i]){SDL_DestroyTexture(pet_tex[i]);pet_tex[i]=NULL;}
  for (int i = 0; i < ATLAS_MAX; i++) if (atlas[i]) SDL_DestroyTexture(atlas[i]);
  if (font_tex) SDL_DestroyTexture(font_tex);
  gfx_room_release();
  for (int i = 0; i < 4; i++) if (scene_v.tex[i]) { SDL_DestroyTexture(scene_v.tex[i]); scene_v.tex[i] = NULL; }
  for (int i = 0; i < FURNI_CACHE; i++) {
    furni_evict(&furni_cache[i]);
  }
  if (ren) SDL_DestroyRenderer(ren);
  if (win) SDL_DestroyWindow(win);
  IMG_Quit();
  SDL_Quit();
}

void gfx_frame(void) {
  room_finish();
}

void gfx_begin(void) {
  SDL_SetRenderDrawColor(ren, 0x0b, 0x0d, 0x0e, 255);
  SDL_RenderClear(ren);
}

void gfx_present(void) {
  if(photo_pending) {
    int w,h;SDL_GetRendererOutputSize(ren,&w,&h);
    int scale=w/SCREEN_W;if(h/SCREEN_H<scale)scale=h/SCREEN_H;
    SDL_Surface *frame=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_ABGR8888);
    photo_status=-1;
    if(frame && scale>0 && SDL_RenderReadPixels(ren,NULL,SDL_PIXELFORMAT_ABGR8888,frame->pixels,frame->pitch)==0) {
      photo_sample(photo_pending,frame->pixels,frame->pitch/4,(w-322*scale)/2,(h-234*scale)/2,scale);photo_status=1;
    }
    SDL_FreeSurface(frame);photo_pending=NULL;
  }
  SDL_RenderPresent(ren);
}

static void draw_tex(SDL_Texture *t, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color) {
  if (!t) return;
  x=view_x+x*view_kx;y=view_y+y*view_ky;kx*=view_kx;ky*=view_ky;
  bool flip_y=ky<0;
  if(kx<0){x+=sw*kx;kx=-kx;flip=!flip;}
  if(ky<0){y+=sh*ky;ky=-ky;}
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
  SDL_RenderCopyExF(ren, t, &src, &dst, 0, NULL, (SDL_RendererFlip)((flip?SDL_FLIP_HORIZONTAL:0)|(flip_y?SDL_FLIP_VERTICAL:0)));
}

void gfx_sprite(int page, int sx, int sy, int sw, int sh, float x, float y, float scale, bool flip, u32 color) {
  if (page < 0 || page >= atlas_page_count || !atlas[page]) return;
  draw_tex(atlas[page], sx, sy, sw, sh, x, y, scale, scale, flip, color);
}

void gfx_sprite_xy(int page, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color) {
  if (page < 0 || page >= atlas_page_count || !atlas[page]) return;
  draw_tex(atlas[page], sx, sy, sw, sh, x, y, kx, ky, flip, color);
}

void gfx_sprite_solid(int page, int sx, int sy, int sw, int sh, float x, float y, float kx, float ky, bool flip, u32 color) {
  /* Approximate silhouette: the same sprite multiplied by the veil color. */
  if (page < 0 || page >= atlas_page_count || !atlas[page]) return;
  draw_tex(atlas[page], sx, sy, sw, sh, x, y, kx, ky, flip, color);
}

void gfx_font_sprite(int sx, int sy, int sw, int sh, float x, float y, bool flip, u32 color) {
  draw_tex(font_tex, sx, sy, sw, sh, x, y, 1, 1, flip, color);
}
void gfx_pet_preload(int palette) {
  if(palette<0 || palette>=pet_palette_count || palette>=COUNT(pet_tex) || pet_tex[palette])return;
  char path[64];snprintf(path,sizeof path,"romfs:/gfx/pets_%d.png",palette);pet_tex[palette]=load_png(path);
}
void gfx_pet(int palette,const PetPart *part,float x,float y,float scale,bool flip,u32 color,bool solid) {
  (void)solid;
  if(palette<0 || palette>=COUNT(pet_tex))return;
  draw_tex(pet_tex[palette],part->x,part->y,part->w,part->h,x,y,scale,scale,flip,color);
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
  room.finish_attempted=false;
}

static void room_finish(void) {
  const char *wall=client_shop_wall(),*floor=client_shop_floor();
  if(!room.bg || (room.finish_attempted && !strcmp(room.wall,wall) && !strcmp(room.floor,floor)))return;
  SDL_Texture *next[4]={0};bool ok=true;
  for(int i=0;i<room.pages && ok;i++) {
    char path[96];snprintf(path,sizeof path,"romfs:/gfx/%s.png",room.bg->tiles[i].file);
    SDL_Surface *raw=IMG_Load(path),*s=raw?SDL_ConvertSurfaceFormat(raw,SDL_PIXELFORMAT_RGBA32,0):NULL;
    if(raw)SDL_FreeSurface(raw);
    ok=s && finishes_paint("romfs:/spaces",room.bg,i,wall,floor,s->pixels,s->w,s->h,s->pitch);
    if(ok) {next[i]=SDL_CreateTextureFromSurface(ren,s);ok=next[i]!=NULL;}
    if(s)SDL_FreeSurface(s);
    if(next[i]) {SDL_SetTextureBlendMode(next[i],SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(next[i],SDL_ScaleModeNearest);}
  }
  for(int i=0;i<room.pages;i++) {
    if(ok) {SDL_DestroyTexture(room.tex[i]);room.tex[i]=next[i];}
    else if(next[i])SDL_DestroyTexture(next[i]);
  }
  if(!ok)dbg_log("room finishes unavailable: %s",room.bg->model);
  snprintf(room.wall,sizeof room.wall,"%s",wall);snprintf(room.floor,sizeof room.floor,"%s",floor);
  room.finish_attempted=true;
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
  if (!want || want->pages > 4) return NULL;
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
  if (!scene_v.scene || s->page >= scene_v.pages || s->page >= 4) return;
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
  if (w <= 0 || h <= 0) return;
  set_color(color);
  SDL_FRect r = {x, y, w, h};
  SDL_RenderFillRectF(ren, &r);
}

void fill_round(float x, float y, float w, float h, float r, u32 color) {
  x=roundf(x); y=roundf(y); w=roundf(w); h=roundf(h);
  if(w<=0 || h<=0)return;
  r=fmaxf(0,fminf(r,fminf(w,h)/2));
  int band=(int)ceilf(r);
  for(int i=0;i<band && i<h/2;i++) {
    float dy=fmaxf(0,r-(i+.5f));
    float inset=roundf(r-sqrtf(fmaxf(0,r*r-dy*dy)));
    fill_rect(x+inset,y+i,w-2*inset,1,color);
    if(h-1-i!=i)fill_rect(x+inset,y+h-1-i,w-2*inset,1,color);
  }
  if(h>2*band)fill_rect(x,y+band,w,h-2*band,color);
}

void fill_circle(float cx, float cy, float r, u32 color) {
  fill_round(cx-r,cy-r,2*r,2*r,r,color);
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

#ifdef TWEEBBO_HOST
/* Desktop harness only: saves the frame being drawn (before present) as a PNG. */
void gfx_screenshot(const char *path) {
  int w, h;
  SDL_GetRendererOutputSize(ren, &w, &h);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
  if (!s) return;
  if (SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ABGR8888, s->pixels, s->pitch) == 0) IMG_SavePNG(s, path);
  SDL_FreeSurface(s);
}
#endif

static SDL_Texture *snow_pages[12];
static unsigned snow_loaded;
void gfx_snow_free(void) {
  while(snow_loaded) SDL_DestroyTexture(snow_pages[--snow_loaded]);
}
bool gfx_snow_load(void) {
  if(snow_loaded==snow_page_count)return true;
  gfx_snow_free();
  if(snow_page_count>COUNT(snow_pages))return false;
  for(unsigned i=0;i<snow_page_count;i++){
    char path[128];snprintf(path,sizeof path,"romfs:/snow/%s.png",snow_page_files[i]);
    SDL_Texture *p=load_png(path);
    if(!p){gfx_snow_free();return false;}
    snow_pages[snow_loaded++]=p;
  }
  return true;
}
#include "../snow_draw.inc"

const SceneArt *gfx_art(const char *name){
  if(!scene_v.scene)return NULL;
  for(int i=0;i<scene_v.scene->art_count;i++)if(!strcmp(scene_v.scene->art[i].name,name))return &scene_v.scene->art[i];
  return NULL;
}
void gfx_art_draw(const SceneArt *a,float x,float y,float kx,float ky,u32 color){
  if(a && scene_v.scene && a->page<scene_v.pages && a->page<4)
    draw_tex(scene_v.tex[a->page],a->sx,a->sy,a->w,a->h,x,y,kx,ky,false,color);
}
