#include "gfx.h"
#include "language.h"
#include <assert.h>
#include <stdio.h>
static int drawn;
int tw_language(void) { return TW_EN; }
const FontGlyph *tw_cjk_glyph(unsigned code) { (void)code; return NULL; }
void gfx_cjk_sprite(int x,int y,int w,int h,float dx,float dy,unsigned color) {
  (void)x;(void)y;(void)w;(void)h;(void)dx;(void)dy;(void)color;
  assert(!"English fallback fixture must use the Latin atlas");
}
void gfx_font_sprite(int x,int y,int w,int h,float dx,float dy,bool flip,u32 color) {
  (void)x;(void)y;(void)w;(void)h;(void)dx;(void)dy;(void)flip;(void)color;drawn++;
}
int main(void) {
  float expected=text_width("Room ?",12);
  assert(expected>0 && text_width("Room 🎲",12)==expected);
  assert(text_draw("Room 🎲",0,0,12,0xffffffff,0)==expected && drawn==6);
  assert(text_width("España",12)>0);
  puts("OK: unsupported chat/title glyphs render as question marks without hiding the line.");
}
