#include "client_pool.h"
#include "client_bar.h"
#include "client_music.h"
#include "audio.h"
#include <stdatomic.h>
static json_t *host_activity,*host_bar,*host_music;
static atomic_ulong host_audio_frames;
void __real_on_activity(const char *json);
void __wrap_on_activity(const char *json){
  json_t *j=json_loads(json,0,NULL);
  if(json_is_object(json_object_get(j,"state"))){json_decref(host_activity);host_activity=json_incref(json_object_get(j,"state"));}
  if(json_is_string(json_object_get(j,"bar"))){json_decref(host_bar);host_bar=json_incref(j);}
  json_decref(j);__real_on_activity(json);
}
void __real_on_music(const char *json);
void __wrap_on_music(const char *json){json_t *j=json_loads(json,0,NULL);json_decref(host_music);host_music=json_incref(json_object_get(j,"state"));json_decref(j);__real_on_music(json);}
void __real_audio_mix(s16 *out,int frames);
void __wrap_audio_mix(s16 *out,int frames){__real_audio_mix(out,frames);unsigned n=0;for(int i=0;i<frames*2;i++)n+=out[i]!=0;atomic_fetch_add(&host_audio_frames,n);}
static const char *final_ui_exercise(u64 elapsed){
  static int step,run;static u64 at;static u32 balance;static bool initialized;
  if(!initialized){initialized=true;if(!strcmp(getenv("UI_EXERCISE"),"final-music"))step=32;}
  if(elapsed<2500 || elapsed<at+(step==9?80:500))return NULL;
  const char *shot=NULL;Ent *me=world_ent(W.me);json_t *dive=json_object_get(host_activity,"dive");
  if(elapsed>280000){dbg_log("Final exercise timed out at step %d",step);assert(false);}
  switch(step){
    case 0:assert(net_online()&&me&&!strcmp(W.model,"pool_a"));send_walk(17,11);break;
    case 1:if(pool_mode()!=1)return NULL;shot="lido-booth";break;
    case 2:show_tap(140,82);break;
    case 3:if(pool_swimsuit(W.me)!=3)return NULL;show_tap(240,190);break;
    case 4:if(pool_mode())return NULL;send_walk(25,9);break;
    case 5:if(!me||me->x!=25||me->y!=9||ent_moving(me,now_ms()))return NULL;show_action(SHOW_A);break;
    case 6:if(strcmp(W.model,"pool_b"))return NULL;shot="lido-signpost";send_walk(26,5);break;
    case 7:if(!me||me->x!=26||me->y!=5||ent_moving(me,now_ms()))return NULL;show_action(SHOW_A);break;
    case 8:if(!pool_tower_view())return NULL;shot="lido-tower";break;
    case 9:show_walk(run++%2?1:-1,0);if(run<30){at=elapsed;return NULL;}show_action(SHOW_A);break;
    case 10:if(!json_is_true(json_object_get(json_object_get(dive,"motion"),"airborne")))return NULL;show_walk(0,-1);show_action(SHOW_B);shot="lido-jump";break;
    case 11:if(json_integer_value(json_object_get(dive,"phase"))!=2)return NULL;shot="lido-landing";net_goto("rooftop","");break;
    case 12:if(strcmp(W.model,"md_a")||!net_online())return NULL;send_walk(20,19);break;
    case 13:if(!me||me->x!=20||me->y!=19||ent_moving(me,now_ms()))return NULL;show_action(SHOW_A);break;
    case 14:if(pool_mode()!=4)return NULL;shot="lido-wobble";break;
    case 15:show_action(SHOW_L);show_action(SHOW_X);show_action(SHOW_A);show_walk(1,0);break;
    case 16:shot="lido-balance";show_tap(SCREEN_W-40,SCREEN_H-18);break;
    case 17:if(pool_mode())return NULL;net_goto("afterhours","");break;
    case 18:if(strcmp(W.slug,"afterhours")||!net_online())return NULL;client_ui_bar();break;
    case 19:if(strcmp(host_string(host_bar,"bar"),"menu"))return NULL;shot="bar-menu";balance=W.credits;break;
    case 20:show_tap(160,55);shot="bar-confirm";break;
    case 21:show_tap(220,202);break;
    case 22:if(!me||!me->drink)return NULL;assert(W.credits<balance);client_ui_bar();break;
    case 23:shot="bar-drink";show_tap(60,90);break;
    case 24:if(strcmp(host_string(host_bar,"bar"),"act"))return NULL;assert(client_bar_sip(me,now_ms(),false)>=0);show_tap(160,90);shot="bar-toast";break;
    case 25:if(strcmp(host_string(host_bar,"bar"),"toast"))return NULL;show_tap(260,90);break;
    case 26:if(me&&me->drink)return NULL;show_tap(35,SCREEN_H-16);break;
    case 27:show_tap(160,173);break;
    case 28:shot="identity-hidden";break;
    case 29:show_tap(230,159);break;
    case 30:show_action(SHOW_A);show_action(SHOW_A);shot="identity-masked";break;
    case 31:show_action(SHOW_B);break;
    case 32:show_action(SHOW_B);net_goto("ports-music-final","");break;
    case 33:if(strcmp(W.slug,"ports-music-final")||!net_online())return NULL;client_ui_music();break;
    case 34:if(!json_is_object(host_music))return NULL;shot="music-playlist";break;
    case 35:show_tap(50,120);break;
    case 36:if(!json_is_true(json_object_get(host_music,"playing"))||!atomic_load(&host_audio_frames))return NULL;shot="music-playing";break;
    case 37:show_tap(50,90);assert(audio_muted());break;
    case 38:show_tap(50,90);assert(!audio_muted());show_tap(50,120);break;
    case 39:if(json_is_true(json_object_get(host_music,"playing")))return NULL;shot="music-paused";break;
    case 40:show_action(SHOW_B);ui_exercise_done=true;dbg_log("OK: %s; Trax download, audible PCM, mute and pause.",!strcmp(getenv("UI_EXERCISE"),"final-music")?"Music-only exercise":"Lido booth/signpost/dive/Wobble, paid bar order/sip/toast/drop, hidden/masked identity");break;
  }
  dbg_log("Final UI stage %d completed",step);step++;at=elapsed;return shot;
}
