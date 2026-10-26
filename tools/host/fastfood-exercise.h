#include "fastfood.h"
#include "client_fastfood.h"
static const char *fastfood_online_exercise(u64 elapsed){
  static int step;static u64 at;json_t *root=fastfood_snapshot(),*state=json_object_get(root,"state");
  assert(!fastfood_error()[0]);assert(elapsed<45000);
  if(step==0){if(!W.in_room)return NULL;client_ui_fastfood();at=elapsed;step++;return NULL;}
  if(step==1){if(!root)return NULL;fastfood_online();step++;return NULL;}
  if(step==2){if(!json_is_true(json_object_get(root,"connected")))return NULL;client_fastfood_action(15008,atoi(getenv("FASTFOOD_GAME")));step++;return "fastfood-online-list";}
  if(step==3){if(strcmp(host_string(state,"phase"),"playing"))return NULL;client_ui_action(SHOW_A);at=elapsed;step++;return NULL;}
  if(step==4){json_t *p=json_array_get(json_object_get(state,"players"),json_integer_value(json_object_get(root,"slot")));if(!json_is_object(json_object_get(p,"plate")))return NULL;client_ui_action(SHOW_X);at=elapsed;step++;return "fastfood-online-play";}
  if(step==5){if(elapsed<at+1500)return NULL;client_ui_action(SHOW_B);at=elapsed;step++;return NULL;}
  if(step==6){if(elapsed<at+1000)return NULL;assert(!json_is_object(state));assert(json_is_object(json_object_get(root,"lobby")));assert(json_is_null(json_object_get(json_object_get(root,"lobby"),"current")));client_ui_close();ui_exercise_done=true;dbg_log("Fast Food: native socket joined, received authority, sent pad actions and left.");return "fastfood-online-closed";}
  return NULL;
}
static const char *fastfood_ui_exercise(u64 elapsed){
  static int step,cycles;static u64 at;json_t *root=fastfood_snapshot(),*state=json_object_get(root,"state");
  assert(!fastfood_error()[0]);
  if(step==0){if(!W.in_room){assert(elapsed<15000);return NULL;}client_ui_fastfood();at=elapsed;step++;return NULL;}
  if(step==1){if(elapsed<at+1000)return NULL;assert(root);step++;return "fastfood-menu";}
  if(step==2){client_fastfood_action(15000,6);at=elapsed;step++;return NULL;}
  if(step==3){if(elapsed<at+4200)return NULL;assert(json_is_object(state));assert(!strcmp(host_string(state,"phase"),"playing"));client_ui_action(SHOW_A);at=elapsed;step++;return NULL;}
  if(step==4){if(elapsed<at+500)return NULL;json_t *p=json_array_get(json_object_get(state,"players"),0);assert(json_is_object(json_object_get(p,"plate")));client_ui_action(SHOW_X);client_ui_action(SHOW_R);client_ui_action(SHOW_Y);at=elapsed;step++;return "fastfood-play";}
  if(step==5){if(elapsed<at+2000)return NULL;client_ui_close();assert(!fastfood_active());at=elapsed;step++;return NULL;}
  if(step==6){if(elapsed<at+1000)return NULL;if(++cycles<3){step=0;return NULL;}ui_exercise_done=true;dbg_log("Fast Food: three practice opens, pad actions and closes passed.");return "fastfood-closed";}
  return NULL;
}
