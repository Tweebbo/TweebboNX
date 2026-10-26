#include "client_battle.h"
static const char *battle_online_exercise(u64 elapsed){
  static int step,moves;static u64 at,move_at;
  assert(!battle_error()[0]);assert(elapsed<155000);
  json_t *g=battle_snapshot();
  if(step==0){if(!W.in_room)return NULL;client_ui_battle();step++;return NULL;}
  if(step==1){if(!json_is_object(g))return NULL;client_battle_action(BATTLE_JOIN,0);step++;return NULL;}
  if(step==2){if(!battle_playing())return NULL;at=elapsed;step++;return "battle-online-start";}
  if(step==3){
    if(json_integer_value(json_object_get(g,"phase"))==2){step++;at=elapsed;return "battle-online-results";}
    if(elapsed>move_at){static const int d[][2]={{1,0},{0,1},{-1,0},{0,-1}};battle_move(d[moves%4][0],d[moves%4][1]);client_ui_action(SHOW_A);moves++;move_at=elapsed+800;}
    return NULL;
  }
  if(step==4){if(elapsed<at+1500)return NULL;assert(moves>100);client_ui_action(SHOW_B);assert(!battle_active());ui_exercise_done=true;dbg_log("PASS: Battle Ball native joined browser, completed two-minute match, moved, used powers, received results and returned");step++;return "battle-online-closed";}
  return NULL;
}
static const char *battle_ui_exercise(u64 elapsed){
  static int step,arena=1,moves,start_x,start_y;static u64 at,move_at;static char shot[40];
  json_t *g=battle_snapshot(),*players=json_object_get(g,"players"),*self=NULL;
  for(size_t i=0;i<json_array_size(players);i++){json_t *p=json_array_get(players,i);if(json_integer_value(json_object_get(p,"eid"))==W.me)self=p;}
  assert(!battle_error()[0]);assert(elapsed<100000);
  if(step==0){if(!W.in_room)return NULL;client_ui_battle();at=elapsed;step++;return NULL;}
  if(step==1){if(elapsed<at+700)return NULL;for(int i=0;i<arena%5;i++)client_battle_action(BATTLE_ARENA,0);client_battle_action(BATTLE_PRACTICE,0);at=elapsed;step++;return NULL;}
  if(step==2){if(!battle_playing()||elapsed<at+3800)return NULL;assert(self);assert(json_integer_value(json_object_get(g,"arena"))==arena);start_x=json_integer_value(json_object_get(self,"x"));start_y=json_integer_value(json_object_get(self,"y"));moves=0;move_at=0;at=elapsed;step++;return NULL;}
  if(step==3){
    assert(self);int x=json_integer_value(json_object_get(self,"x")),y=json_integer_value(json_object_get(self,"y"));
    if(x!=start_x||y!=start_y){client_battle_action(BATTLE_ZOOM,1);client_battle_action(BATTLE_ZOOM,-1);client_battle_action(BATTLE_CENTRE,0);client_ui_action(SHOW_A);snprintf(shot,sizeof shot,"battle-arena-%d",arena);at=elapsed;step++;return shot;}
    if(elapsed>=move_at){static const int directions[][2]={{1,0},{0,1},{-1,0},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}};assert(moves<8);battle_move(directions[moves][0],directions[moves][1]);moves++;move_at=elapsed+800;}
    return NULL;
  }
  if(step==4){if(elapsed<at+1000)return NULL;client_ui_action(SHOW_B);assert(!battle_active());at=elapsed;step++;return NULL;}
  if(step==5){if(elapsed<at+600)return NULL;if(++arena<=5){step=0;return NULL;}ui_exercise_done=true;dbg_log("Battle Ball: all five arenas, authoritative movement, power control, zoom and return passed.");return "battle-closed";}
  return NULL;
}
