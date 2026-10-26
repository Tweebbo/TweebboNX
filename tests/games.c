#include "../source/client_games.c"
#include <assert.h>
World W;
static bool online=true;
static int sends,op,from,to,promotion;
static int opened,closed;
static char destination[32];
void client_ui_fastfood(void) { opened=FOOD; }
void client_ui_snow(void) { opened=SNOW; }
void client_ui_battle(void) { opened=BATTLE; }
void client_ui_close(void) { closed++; }
void net_goto(const char *slug,const char *arrival) { assert(!*arrival);snprintf(destination,sizeof destination,"%s",slug); }
u64 now_ms(void) {return 100;}
bool net_online(void) {return online;}
Ent *world_ent(u32 eid) {for(int i=0;i<MAX_ENTS;i++)if(W.ents[i].used && W.ents[i].id==eid)return &W.ents[i];return NULL;}
bool client_social_blocked(u32 guest) {return guest==99;}
void client_ui_changed(void) {}
void show_toast(const char *s,u64 now) {(void)s;(void)now;}
void send_game(int operation,u32 id,int a,int b,int c) {assert(id);sends++;op=operation;from=a;to=b;promotion=c;}
void fill_rect(float x,float y,float w,float h,u32 color) {(void)x;(void)y;(void)w;(void)h;(void)color;}
void client_ui_button(float x,float y,float w,float h,const char *s,int command,int arg) {
  (void)s;(void)command;(void)arg;assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=480 && y+h<=272);
}
void client_ui_label(const char *s,float x,float y,float w,u32 c) {(void)s;(void)x;(void)y;(void)w;(void)c;}
size_t client_ui_paragraph(const char *s,float x,float y,float w,int lines,u32 c) {(void)x;(void)y;(void)w;(void)lines;(void)c;return strlen(s);}
int main(void) {
  client_games_action(FOOD,0);assert(opened==FOOD);
  client_games_action(SNOW,0);assert(opened==SNOW);
  client_games_action(BATTLE,0);assert(opened==BATTLE);
  client_games_action(BATTLE_LOBBY,0);assert(!strcmp(destination,"battleball")&&closed==1);
  client_games_action(SNOW_LOBBY,0);assert(!strcmp(destination,"snowstorm")&&closed==2);
  client_games_action(CENTER,1);
  client_games_reset();W.me=1;W.ents[0]=(Ent){.used=true,.id=2,.guest=2};
  client_games_action(CHALLENGE,2);assert(op==1 && from==1);
  W.invites[0]=(GameInvite){2,3};W.game_rev++;client_games_update(false);client_games_draw(8,8,310);
  client_games_action(ACCEPT,2);assert(op==2 && from==1 && D.pending);int count=sends;
  client_games_action(ACCEPT,2);assert(sends==count);
  W.invites[0].from=0;W.duel=(Duel){.id=42,.kind=1,.players={1,2},.turn=0,.move_count=1,.moves={{4,0}}};
  W.game_rev++;client_games_update(true);client_games_draw(8,8,310);
  client_games_action(CELL,3);assert(sends==count);client_games_action(CELL,4);assert(op==3 && from==4 && to==0 && D.pending);
  client_games_action(CELL,4);assert(sends==count+1);
  W.duel.kind=3;W.duel.move_count=1;W.duel.moves[0][0]=8;W.duel.moves[0][1]=16;
  W.game_rev++;client_games_update(true);client_games_draw(8,8,310);
  client_games_action(CELL,8);assert(D.picked==8);client_games_action(PROMOTION,0);client_games_action(CELL,16);assert(from==8 && to==16 && promotion==4);
  W.game_rev++;client_games_update(true);client_games_action(RESIGN,0);assert(D.confirm);count=sends;
  client_games_draw(8,8,310);client_games_action(CANCEL,0);assert(!D.confirm && sends==count);
  client_games_action(RESIGN,0);client_games_action(CONFIRM,0);assert(op==4 && sends==count+1);
  W.duel.kind=2;W.duel.result=2;W.game_rev++;client_games_update(true);client_games_draw(8,8,310);
  client_games_action(DISMISS,0);assert(D.dismissed);client_games_draw(8,8,310);
  online=false;count=sends;client_games_action(CHALLENGE,2);assert(sends==count);
  puts("OK: game invitation, legal moves, duplicate guard, chess source/target/promotion, resignation confirmation and board bounds.");
}
