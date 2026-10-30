/* Runs through the public input API, with the real renderer and local hotel. */
#include <jansson.h>
#include "catalog.h"
static json_t *host_casino;
static bool ui_exercise_done;
static unsigned host_casino_revision;
static u64 host_casino_received;
void __real_on_casino(const char *text);
void __wrap_on_casino(const char *text) {
  json_decref(host_casino);host_casino=json_loads(text,0,NULL);host_casino_revision++;host_casino_received=now_ms();
  if(json_is_string(json_object_get(host_casino,"game")))dbg_log("Casino cabinet %llu opened.",(unsigned long long)json_integer_value(json_object_get(host_casino,"item")));
  __real_on_casino(text);
}
static const char *host_string(json_t *j,const char *key) {const char *s=json_string_value(json_object_get(j,key));return s?s:"";}
static const char *casino_ui_exercise(u64 elapsed) {
  static int game,step,slot;
  static u64 at;
  static unsigned revision;
  static char shot[64];
  static const char *keys[]={"slot","roulette","blackjack","crash","poker","videopoker","dash"};
  float panel_w=SCREEN_W-16,table_w=floorf(panel_w*.52f),controls=panel_w-table_w;
  float first=8+table_w+8+(controls-16)/(game==2?3:2)/2;
  float second=first+(controls-16)/(game==2?3:2);
  if(elapsed<2000 || game>=7 || elapsed<at+800)return NULL;
  const char *type=host_string(host_casino,"t");
  assert(strcmp(type,"refused") && strcmp(type,"note"));
  switch(step) {
    case 0: {
      assert(W.in_room && net_online() && !strcmp(W.slug,"casino"));
      u32 item=0;for(int i=0;i<MAX_ITEMS;i++)if(W.items[i].used){const FurniDef *d=def_get(W.items[i].def);if(d && d->casino==game+1){item=W.items[i].id;break;}}
      assert(item);json_decref(host_casino);host_casino=NULL;
      client_ui_inspect(item);break;
    }
    case 1:show_tap(180,80);break;
    case 2:
      if(strcmp(type,"open")){assert(elapsed<at+35000);return NULL;}
      assert(!strcmp(host_string(host_casino,"game"),keys[game]));
      assert(!strcmp(host_string(host_casino,"mode"),"free"));
      snprintf(shot,sizeof shot,"casino-%s-controls",keys[game]);step++;at=elapsed;return shot;
    case 3:revision=host_casino_revision;show_tap(first,180);break;
    case 4:
      if(host_casino_revision==revision){assert(elapsed<at+15000);return NULL;}
      assert(strcmp(type,"open"));break;
    case 5:snprintf(shot,sizeof shot,"casino-%s-table-%d",keys[game],slot);step++;at=elapsed;return shot;
    case 6:
      if(game==5)show_tap(40,134);
      show_tap(50,48);break;
    case 7: {
      json_t *hand=json_object_get(host_casino,"hand");
      revision=host_casino_revision;
      if(game==2 || game==4){if(!*host_string(hand,"result"))show_tap(second,180);else step++;}
      else if(game==5 || game==6)show_tap(first,180);
      else if(game==3) {
        const char *p=host_string(json_object_get(host_casino,"round"),"phase");
        // Input runs before drawing: wait for an enabled Cash out button to have been painted.
        if(!strcmp(p,"betting") && now_ms()-host_casino_received>=(u64)json_integer_value(json_object_get(json_object_get(host_casino,"round"),"startsIn"))+200)p="running";
        if(!strcmp(p,"running"))show_tap(second,180);
        else if(!strcmp(p,"crashed") || !strcmp(p,"idle"))step++;
        else {assert(elapsed<at+12000);return NULL;}
      } else step++;
      break;
    }
    case 8:
      if(host_casino_revision==revision){assert(elapsed<at+15000);return NULL;}
      if(game==6 && !json_is_true(json_object_get(host_casino,"done"))) {
        assert(json_integer_value(json_object_get(host_casino,"step"))>0);revision=host_casino_revision;show_tap(second,180);step=12;at=elapsed;return NULL;
      }
      break;
    case 9:
      if(game==0 && slot<4 && !json_integer_value(json_object_get(host_casino,"freeLeft"))) {
        slot++;show_tap(SCREEN_W-40,106);step=3;at=elapsed;return NULL;
      }
      show_tap(260,48);break;
    case 10:snprintf(shot,sizeof shot,"casino-%s-rules",keys[game]);step++;at=elapsed;return shot;
    case 11:
      show_tap(50,48);step=14;at=elapsed;return NULL;
    case 14:
      show_tap(230,212);step=13;at=elapsed;return NULL;
    case 12:
      if(host_casino_revision==revision){assert(elapsed<at+15000);return NULL;}
      assert(json_is_true(json_object_get(host_casino,"done")));step=9;at=elapsed;return NULL;
    case 13:
      show_tap(230,204);dbg_log("OK: casino %s played through native UI, server result received and leave confirmed.",keys[game]);
      game++;step=0;slot=0;at=elapsed;if(game==7){ui_exercise_done=true;dbg_log("OK: all seven native casino UI flows completed.");}return NULL;
  }
  step++;at=elapsed;return NULL;
}
static const char *interaction_ui_exercise(u64 elapsed) {
  static unsigned step;
  static int job;
  static u16 defs[4];
  static u32 items[4];
  if(elapsed<1500+step*1000 || step>45)return NULL;
  const char *shot=NULL;
  Ent *me=world_ent(W.me);
  switch(step++) {
    case 0: {
      assert(net_online());
      json_t *j=json_pack("{s:s,s:s,s:s}","title","Native interactions check","model","model_a","token",net_token());
      char *body=json_dumps(j,JSON_COMPACT);assert(body);
      job=net_http("POST","/api/rooms",body,true);free(body);json_decref(j);
      assert(job);break;
    }
    case 1: {
      HttpResult r;assert(net_http_result(job,&r) && r.status==200);
      json_t *j=json_loads(r.body,0,NULL);const char *slug=json_string_value(json_object_get(j,"slug"));
      assert(slug);net_goto(slug,"");json_decref(j);free(r.body);break;
    }
    case 2:
      assert(W.in_room && W.room_owner);
      for(int i=0;i<cat_offer_count;i++) {
        const CatOffer *o=&cat_offers[i];const FurniDef *d=def_get(o->def);
        if(!d || !o->available || o->amount!=1 || o->price>20 || d->w!=1 || d->h!=1)continue;
        int slot=d->drinks && strchr(d->drinks,',')?1:d->kind==KIND_SIT?0:d->kind==KIND_DICE?2:d->kind==KIND_TOGGLE?3:-1;
        if(slot>=0 && !defs[slot]){defs[slot]=d->id;send_buy(o->id);}
      }
      for(int i=0;i<4;i++)assert(defs[i]);
      break;
    case 3:
      for(int i=0;i<4;i++) {
        for(int j=0;j<W.inv_n;j++)if(W.inv[j].def==defs[i]){items[i]=W.inv[j].id;break;}
        assert(items[i]);send_place(items[i],4+i,6,0,0);
      }break;
    case 4:
      for(int i=0;i<4;i++)assert(world_item(items[i]));
      client_ui_inspect(items[0]);break;
    case 5:shot="interact-seat";break;
    case 6:show_tap(180,80);break;
    case 10:assert(me && (me->flags&FLAG_SIT) && me->x==4 && me->y==6);client_ui_inspect(items[1]);break;
    case 11:shot="interact-drinks";break;
    case 12:show_tap(250,130);break;
    case 13:shot="interact-drink-choice";break;
    case 14:show_tap(180,163);break;
    case 18:assert(me && me->drink);show_tap(30,SCREEN_H-16);break;
    case 19:show_tap(120,205);break;
    case 20:shot="character-actions";break;
    case 21:show_tap(100,172);break;
    case 22:assert(me && !me->drink);client_ui_inspect(items[2]);break;
    case 23:shot="interact-dice";break;
    case 24:show_tap(180,80);break;
    case 29:assert(world_item(items[2])->state>0 && world_item(items[2])->state<7);client_ui_inspect(items[2]);break;
    case 30:shot="interact-dice-close";break;
    case 31:show_tap(180,112);break;
    case 32:assert(world_item(items[2])->state==0);client_ui_inspect(items[3]);break;
    case 33:shot="interact-toggle";break;
    case 34:show_tap(180,80);break;
    case 35:assert(world_item(items[3])->state==1);show_tap(30,SCREEN_H-16);break;
    case 36:show_tap(120,205);break;
    case 37:show_tap(120,108);break;
    case 38:assert(me && (me->flags&FLAG_DANCE));show_tap(315,SCREEN_H-16);break;
    case 39:show_tap(220,50);break;
    case 40:show_tap(280,48);break;
    case 41:shot="catalogue-search-keyboard";break;
    case 42:show_tap(28,98);break;
    case 43:show_action(SHOW_Y);break;
    case 44:shot="catalogue-search-results";break;
    case 45:dbg_log("OK: native seat, selected drink, put-away, dice roll/close, toggle, dance and catalogue search through real UI input.");break;
  }
  return shot;
}
static const char *trade_ui_exercise(u64 elapsed) {
  static unsigned step;
  static u32 original_item;
  if(elapsed<1500+step*800 || step>23)return NULL;
  const char *shot=NULL;
  json_t *trade=W.trade_json?json_loads(W.trade_json,0,NULL):NULL;
  const char *stage=json_string_value(json_object_get(trade,"stage"));
  switch(step++) {
    case 0:assert(net_online() && W.in_room);send_buy(1);break;
    case 1:assert(W.inv_n>0);original_item=W.inv[0].id;show_tap(315,SCREEN_H-16);break;
    case 2:show_tap(90,50);break;
    case 3:show_tap(160,208);break;
    case 4:shot="trade-players";break;
    case 5:show_tap(100,75);break;
    case 6:assert(stage && !strcmp(stage,"open"));shot="trade-peer-offer";break;
    case 7:shot="trade-inventory";show_tap(60,197);break;
    case 8:show_tap(80,70);break;
    case 9:shot="trade-item-detail";break;
    case 10:show_tap(230,195);break;
    case 11:assert(json_array_size(json_object_get(trade,"mine"))==1);shot="trade-offers";break;
    case 12:show_tap(220,70);break;
    case 13:shot="trade-peer-detail";break;
    case 14:show_tap(80,195);break;
    case 15:show_tap(170,197);break;
    case 16:assert(json_is_true(json_object_get(trade,"confirming")));shot="trade-final-review";break;
    case 17:show_tap(170,197);break;
    case 18:
      assert(stage && !strcmp(stage,"done"));
      for(int i=0;i<W.inv_n;i++)assert(W.inv[i].id!=original_item);
      assert(W.inv_n>0);shot="trade-complete";break;
    case 19:show_tap(100,202);break;
    case 20:show_tap(100,75);break;
    case 21:assert(stage && !strcmp(stage,"open"));show_action(SHOW_B);break;
    case 22:assert(stage && !strcmp(stage,"cancelled"));break;
    case 23:dbg_log("OK: native trade invitation, inventory/detail, offer review, two confirmations, inventory swap and close-to-cancel through real UI input.");break;
  }
  json_decref(trade);return shot;
}
#include "client_snow.h"
static unsigned snow_packets;
void __real_on_snow(const u8 *data,int len);
void __wrap_on_snow(const u8 *data,int len){snow_packets++;__real_on_snow(data,len);}
static const char *snow_peer_exercise(u64 elapsed){
  static int step;static u64 at,started;
  assert(elapsed<160000);
  if(elapsed<at+1000)return NULL;
  const char *shot=NULL;
  switch(step){
    case 0:if(!W.in_room)return NULL;client_ui_snow();break;
    case 1:client_snow_action(7004,atoi(getenv("SNOW_GAME")));break;
    case 2:if(!client_snow_playing())return NULL;started=elapsed;shot="snow-peer-start";break;
    case 3:
      if(client_snow_playing()){client_snow_move(1,0);client_ui_action(SHOW_X);client_ui_action(SHOW_L);client_ui_action(SHOW_A);}
      if(elapsed<started+130000){at=elapsed;return NULL;}
      assert(!client_snow_playing()&&snow_packets>600);shot="snow-peer-results";break;
    case 4:client_snow_action(7009,0);ui_exercise_done=true;dbg_log("PASS: native Snow Storm joins browser, executes commands, completes two-minute match and returns (%u packets)",snow_packets);break;
    default:return NULL;
  }
  step++;at=elapsed;return shot;
}
static const char *snow_online_exercise(u64 elapsed){
  static int step;static u64 at,started;
  if(elapsed<3000 || elapsed<at+1000)return NULL;
  const char *shot=NULL;
  switch(step){
    case 0:assert(net_online()&&W.in_room);show_tap(315,SCREEN_H-16);break;
    case 1:show_tap(70,210);break;
    case 2:assert(snow_packets);show_tap(260,168);break;
    case 3:shot="snow-create";show_tap(250,115);break;
    case 4:show_tap(160,205);break;
    case 5:shot="snow-waiting";break;
    case 6:show_tap(220,205);started=elapsed;break;
    case 7:if(!client_snow_playing()){assert(elapsed<at+15000);return NULL;}shot="snow-online";break;
    case 8:
      if(client_snow_playing()){show_walk(1,0);show_action(SHOW_X);show_action(SHOW_L);show_action(SHOW_A);}
      if(elapsed<started+130000){at=elapsed;return NULL;}
      assert(!client_snow_playing()&&snow_packets>600);shot="snow-results";break;
    case 9:show_tap(160,205);break;
    case 10:shot="snow-return-lobby";break;
    case 11:show_action(SHOW_B);ui_exercise_done=true;dbg_log("OK: online Snow Storm creation, host start, commands, 2-minute match, results and return to lobby (%u server packets).",snow_packets);break;
  }
  step++;at=elapsed;return shot;
}
static const char *snow_ui_exercise(u64 elapsed) {
  static unsigned step;static u64 at;
  if(elapsed<2500 || elapsed<at+800)return NULL;
  const char *shot=NULL;
  switch(step) {
    case 0:show_tap(315,SCREEN_H-16);break;
    case 1:show_tap(70,210);break;
    case 2:shot="snow-lobby";break;
    case 3:show_tap(100,205);break;
    case 4:assert(client_snow_playing());shot="snow-practice";break;
    case 5:show_walk(1,0);show_action(SHOW_X);break;
    case 6:show_action(SHOW_Y);show_action(SHOW_L);break;
    case 7:show_action(SHOW_A);show_action(SHOW_R);break;
    case 8:show_action(SHOW_B);break;
    case 9:assert(!client_snow_playing());shot="snow-targets";break;
    case 10:show_tap(230,205);break;
    case 11:show_action(SHOW_A);show_action(SHOW_A);shot="snow-chat";break;
    case 12:show_action(SHOW_Y);break;
    case 13:show_tap(70,205);break;
    case 14:assert(client_snow_playing());shot="snow-zoom";show_action(SHOW_B);break;
    case 15:show_action(SHOW_B);break;
    case 16:shot="snow-leave";break;
    case 17:show_tap(70,205);break;
    case 18:assert(client_snow_playing());show_action(SHOW_B);break;
    case 19:show_action(SHOW_B);break;
    case 20:show_tap(230,205);break;
    case 21:assert(!client_snow_playing());ui_exercise_done=true;dbg_log("OK: Snow Storm practice, controls, targets, live chat, zoom and leave confirmation.");break;
  }
  step++;at=elapsed;return shot;
}
#include "final-exercise.h"
#include "bed-exercise.h"
#include "camera-exercise.h"
#include "reception-exercise.h"
#include "fastfood-exercise.h"
#include "battle-exercise.h"
#include "language-exercise.h"
#include "feedback-exercise.h"
#include "update-exercise.h"
#include "room-controls-exercise.h"

static const char *ui_exercise(u64 elapsed) {
  static unsigned step;
  static bool initialized;
  static u64 original;
  static char original_room[48];
  static u32 finish_balance;
  if (!getenv("UI_EXERCISE") || !*getenv("UI_EXERCISE")) return NULL;
  if(!strcmp(getenv("UI_EXERCISE"),"power"))return NULL;
  if(!strcmp(getenv("UI_EXERCISE"),"room-controls"))return room_controls_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"update")||!strcmp(getenv("UI_EXERCISE"),"update-fallback"))return update_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"languages"))return language_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"feedback"))return feedback_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"fastfood"))return fastfood_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"fastfood-online"))return fastfood_online_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"battle"))return battle_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"battle-online"))return battle_online_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"beds"))return bed_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"camera"))return camera_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"reception"))return reception_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"final")||!strcmp(getenv("UI_EXERCISE"),"final-music"))return final_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"trade"))return trade_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"interactions"))return interaction_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"snow-online"))return snow_online_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"snow-peer"))return snow_peer_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"snow"))return snow_ui_exercise(elapsed);
  if(!strcmp(getenv("UI_EXERCISE"),"casino"))return casino_ui_exercise(elapsed);
  unsigned first=!strcmp(getenv("UI_EXERCISE"),"settings")?93:0;
  if(!initialized){step=first;initialized=true;}
  if (elapsed < 1500 + (step-first) * 800 || step > 107) return NULL;
  float kx=8;
  const char *shot=NULL;
  Ent *me=world_ent(W.me);
  switch(step++) {
    case 0: assert(W.in_room && me);original=me->look;snprintf(original_room,sizeof original_room,"%s",W.slug);show_action(SHOW_B);break;
    case 1: shot="rooms";break;
    case 2: show_tap(70,126);break;
    case 3: assert(W.in_room && me && strcmp(original_room,W.slug));show_tap(30,SCREEN_H-16);break;
    case 4: show_tap(130,108);break;
    case 5: show_tap(90,48);break;
    case 6: show_tap(290,114);break;
    case 7: shot="wardrobe";break;
    case 8: show_action(SHOW_B);assert(me->look==original);break;
    case 9: show_tap(130,108);break;
    case 10: show_tap(290,114);break;
    case 11: show_tap(230,204);break;
    case 12: assert(me->look!=original);show_action(SHOW_Y);break;
    case 13: show_tap(kx+20,98);show_tap(kx+20,122);break;
    case 14: shot="keyboard";break;
    case 15: show_tap(kx+250,200);break;
    case 16: assert(me->talk_until>0);shot="chat";break;
    case 17: show_tap(30,SCREEN_H-16);break;
    case 18: shot="character";show_tap(130,140);break;
    case 19:
      for(int i=0;i<24;i++)show_tap(kx+170,200);
      show_tap(kx+30,200); /* Uppercase. */
      show_tap(kx+8+9*34.4f+10,98); /* P */
      show_tap(kx+30,200);
      show_tap(kx+8+8*34.4f+10,98); /* o */
      show_tap(kx+8+3*34.4f+10,98); /* r */
      show_tap(kx+8+4*34.4f+10,98); /* t */
      show_tap(kx+8+(TWEEBBO_DEVICE-1)*34.4f+10,74);
      break;
    case 20: show_tap(kx+250,200);break;
    case 21: {
      char expected[16];snprintf(expected,sizeof expected,"Port%d",TWEEBBO_DEVICE);
      assert(!strcmp(W.name,expected));shot="saved-character";break;
    }
    case 22: show_tap(245,SCREEN_H-16);break;
    case 23: shot="help";break;
    case 24: dbg_log("OK: rooms, chat, name, wardrobe save/cancel through real UI input.");break;
    case 25: show_tap(100,SCREEN_H-16);break;
    case 26: show_tap(260,74);break;
    case 27: shot="create-room";show_tap(100,74);break;
    case 28:
      show_tap(kx+20,98);show_tap(kx+55,98);show_tap(kx+90,98);break;
    case 29: show_action(SHOW_Y);break;
    case 30: show_tap(240,208);break;
    case 31: assert(W.in_room && W.room_owner);shot="created-room";show_tap(100,SCREEN_H-16);break;
    case 32: show_tap(170,48);break;
    case 33: shot="my-rooms";break;
    case 34: dbg_log("OK: room creation preserves character identity and Mine lists the new room.");break;
    case 35:show_tap(315,SCREEN_H-16);break;
    case 36:show_tap(220,50);break;
    case 37:shot="catalogue";show_tap(100,176);break;
    case 38:show_tap(100,75);break;
    case 39:shot="offer";break;
    case 40:show_tap(245,208);break;
    case 41:assert(W.inv_n>=2);show_tap(315,SCREEN_H-16);break;
    case 42:show_tap(100,95);break;
    case 43:shot="inventory";show_tap(100,75);break;
    case 44:show_tap(180,120);break;
    case 45: {
      assert(client_shop_building());const BuildGhost *g=client_shop_ghost();
      while(g->x<4)show_walk(1,0);
      while(g->x>4)show_walk(-1,0);
      while(g->y<4)show_walk(0,1);
      while(g->y>4)show_walk(0,-1);
      assert(g->ok);break;
    }
    case 46:shot="build";break;
    case 47:show_action(SHOW_A);break;
    case 48:assert(!client_shop_building() && W.inv_n>=1);shot="placed";break;
    case 49:show_tap(180,132);break;
    case 50:show_tap(245,208);break;
    case 51:assert(W.inv_n>=2);show_tap(315,SCREEN_H-16);break;
    case 52:show_tap(100,138);break;
    case 53:shot="friends";break;
    case 54:show_tap(100,74);break;
    case 55:shot="friend-search";dbg_log("OK: catalogue bundle purchase, inventory, pad placement, pickup and Friends through the real hotel.");break;
    case 56:show_action(SHOW_B);break;
    case 57:show_tap(315,SCREEN_H-16);break;
    case 58:show_tap(100,178);break;
    case 59:show_tap(100,50);break;
    case 60:shot="pet-preview";break;
    case 61:show_tap(100,170);break;
    case 62:show_tap(kx+20,98);show_tap(kx+55,98);show_tap(kx+90,98);break;
    case 63:show_action(SHOW_Y);break;
    case 64:show_tap(100,208);break;
    case 65:show_tap(100,208);break;
    case 66:assert(W.pet_inv_json && strstr(W.pet_inv_json,"qwe"));break;
    case 67:show_tap(100,83);break;
    case 68:show_tap(180,196);break;
    case 69:assert(pets_count()>0);shot="pet-room";break;
    case 70:show_tap(128,202);break;
    case 71:shot="pet-care";break;
    case 72:show_tap(270,202);break;
    case 73:assert(pets_count()==0);break;
    case 74:dbg_log("OK: pet appearance, name, adoption, room rendering, feeding and pickup through real UI input.");break;
    case 75:finish_balance=W.credits;assert(!*W.wall && !*W.floor);show_tap(315,SCREEN_H-16);break;
    case 76:show_tap(220,50);break;
    case 77:show_tap(100,125);break;
    case 78:show_tap(230,148);break;
    case 79:assert(*client_shop_wall() && !*W.wall);shot="wall-preview";break;
    case 80:show_tap(235,208);break;
    case 81:shot="finish-confirm";break;
    case 82:show_tap(235,208);break;
    case 83:assert(*W.wall && W.credits==finish_balance-(u32)finish_price);show_tap(235,70);break;
    case 84:shot="floor-preview";assert(*client_shop_floor() && !*W.floor);break;
    case 85:show_action(SHOW_B);assert(!*client_shop_floor());break;
    case 86:shot="finish-preview-cancelled";show_tap(315,SCREEN_H-16);break;
    case 87:show_tap(220,50);break;
    case 88:show_tap(100,125);break;
    case 89:show_tap(235,208);break;
    case 90:show_tap(235,208);break;
    case 91:assert(*W.floor && W.credits==finish_balance-2*(u32)finish_price);show_action(SHOW_B);break;
    case 92:shot="room-finishes";dbg_log("OK: wall/floor preview, cancellation, confirmed purchases and exact credit deduction through real UI input.");break;
    case 93:assert(W.in_room && W.room_owner && net_online());show_tap(315,SCREEN_H-16);break;
    case 94:show_tap(220,178);break;
    case 95:show_tap(100,108);break;
    case 96:show_tap(kx+20,98);show_tap(kx+55,98);show_tap(kx+90,98);show_tap(kx+125,98);shot="masked-room-password";break;
    case 97:shot="room-password-confirm";show_action(SHOW_Y);break;
    case 98:show_tap(230,200);break;
    case 99:shot="room-unlock-confirm";show_tap(100,140);break;
    case 100:show_action(SHOW_B);break;
    case 101:show_tap(100,140);break;
    case 102:show_tap(230,200);break;
    case 103:shot="room-rights";show_tap(100,175);break;
    case 104:show_action(SHOW_B);break;
    case 105:show_tap(315,SCREEN_H-16);break;
    case 106:show_tap(220,178);break;
    case 107:shot="room-settings-reloaded";dbg_log("OK: room settings, masked password, confirmation, removal cancellation, removal and rights through real UI input.");break;
  }
  return shot;
}

static const char *pad_exercise(u64 elapsed) {
  static unsigned step;
  if (!getenv("PAD_EXERCISE") || !*getenv("PAD_EXERCISE")) return NULL;
  if (elapsed < 1500 + step * 400 || step > 13) return NULL;
  assert(W.in_room && world_ent(W.me));
  switch(step++) {
    case 0: client_ui_set_tv(true);show_action(SHOW_Y);break;
    case 1: show_action(SHOW_A);break;
    case 2: show_action(SHOW_X);break;
    case 3: show_action(SHOW_L);show_action(SHOW_A);break;
    case 4: show_action(SHOW_R);break;
    case 5: show_walk(1,0);break;
    case 6: show_action(SHOW_A);break;
    case 7: return "dock-keyboard";
    case 8: client_ui_set_tv(false);break;
    case 9: return "handheld-keyboard";
    case 10: client_ui_set_tv(true);break;
    case 11: return "redock-keyboard";
    case 12: show_action(SHOW_Y);break;
    case 13:
      assert(world_ent(W.me)->talk_until>0);
      dbg_log("OK: controller-only typing, deletion, case, space, send and live dock/undock layouts.");
      return "dock-chat";
  }
  return NULL;
}

static void soak_exercise(u64 elapsed) {
  static int step;
  static const char *rooms[]={"parque","pool","diving","rooftop","afterhours","casino","recibidor"};
  if(!getenv("SOAK_EXERCISE") || !*getenv("SOAK_EXERCISE"))return;
  if(step<7 && elapsed>=5000+(u64)step*8000 && net_online() && W.in_room) {
    net_goto(rooms[step++],"");
  }
  if(step==7 && elapsed>=65000 && net_online() && W.in_room) {
    show_action(SHOW_Y);step++;
  }
  if(step==8 && elapsed>=320000) {
    assert(net_online() && W.in_room && world_ent(W.me));
    dbg_log("OK: seven room changes and a live keyboard across session renewal.");step++;
  }
}
