#include "../source/client_snow.c"
#include <assert.h>
#include <time.h>
#include <stdarg.h>
World W;
static u64 offset;
static unsigned sent,loads,frees,changes;
u64 now_ms(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (u64)ts.tv_sec*1000+ts.tv_nsec/1000000+offset;}
void dbg_log(const char *fmt,...){va_list ap;va_start(ap,fmt);vfprintf(stderr,fmt,ap);fputc('\n',stderr);va_end(ap);}
void net_send(const u8 *p,int n){assert(n>1&&n<=1024&&p[0]==C_SNOW);sent++;}
bool net_online(void){return false;}
Ent *world_ent(u32 id){(void)id;return NULL;}
bool gfx_snow_load(void){loads++;return true;}
void gfx_snow_free(void){frees++;}
void gfx_snow_sprite(unsigned id,float x,float y,float kx,float ky,u32 c,float l,float t,float r,float b){(void)id;(void)x;(void)y;(void)kx;(void)ky;(void)c;(void)l;(void)t;(void)r;(void)b;}
void fill_round(float x,float y,float w,float h,float r,u32 c){(void)x;(void)y;(void)w;(void)h;(void)r;(void)c;}
void fill_rect(float x,float y,float w,float h,u32 c){(void)x;(void)y;(void)w;(void)h;(void)c;}
float text_draw(const char *s,float x,float y,float size,u32 c,int align){(void)s;(void)x;(void)y;(void)size;(void)c;(void)align;return 0;}
float text_width(const char *s,float size){return strlen(s)*size/2;}
void client_ui_changed(void){changes++;}
void client_ui_close(void){}
void client_ui_edit(const char *title,const char *initial,bool (*submit)(const char *)){(void)title;(void)initial;assert(submit("snow test"));}
void client_ui_button(float x,float y,float w,float h,const char *s,int c,int arg){(void)x;(void)y;(void)w;(void)h;(void)s;(void)c;(void)arg;}
void client_ui_label(const char *s,float x,float y,float w,u32 c){(void)s;(void)x;(void)y;(void)w;(void)c;}
size_t client_ui_paragraph(const char *s,float x,float y,float w,int n,u32 c){(void)s;(void)x;(void)y;(void)w;(void)n;(void)c;return 0;}
int main(void){
 W.me=7;
 for(int run=0;run<2;run++){
  client_snow_open();client_snow_update();assert(context&&!error[0]&&is_mode("list")&&loads==(unsigned)run+1);
  client_snow_action(PRACTICE,0);client_snow_update();assert(client_snow_playing());
  unsigned baseline=sent;
  client_snow_key(SHOW_X);client_snow_key(SHOW_Y);client_snow_key(SHOW_L);client_snow_key(SHOW_A);client_snow_move(1,0);
  client_snow_key(SHOW_B);assert(!client_snow_playing());
  client_snow_action(CHAT,0);client_snow_close();assert(confirm_leave&&active);
  client_snow_action(CANCEL,0);assert(client_snow_playing());
  assert(client_snow_scene(now_ms()));
  for(int frame=0;frame<3100&&!is_mode("results");frame++){offset+=60;client_snow_update();assert(!error[0]);}
  assert(is_mode("results"));assert(sent==baseline);assert(num(json_object_get(snapshot,"state"),"left")==0);
  client_snow_draw(8,8,310);client_snow_action(LOBBY,0);client_snow_update();assert(is_mode("list"));
  client_snow_reset();assert(!active&&!context&&!runtime);assert(frees==(unsigned)run);
  client_snow_update();assert(frees==(unsigned)run+1);
 }
 assert(changes);puts("Snow Storm: two complete real-engine practice matches, controls, chat, results and deferred texture cleanup passed.");
}
