#include "language.h"
static const char *language_exercise(u64 elapsed){
  static unsigned step,index;
  static u64 at,ready;
  static char shot[48];
  static const int languages[]={TW_FR,TW_PT_BR,TW_ZH,TW_JA,TW_ES,TW_EN};
  if(!W.in_room)return NULL;
  if(!ready)ready=elapsed;
  if(elapsed<ready+1000||elapsed<at+650)return NULL;
  at=elapsed;
  if(step==0){float cell=SCREEN_W>=600?66:50;show_tap(SCREEN_W/2.f+3*cell,SCREEN_H-16);step++;return NULL;}
  if(step==1){show_tap(238,152);step++;return NULL;}
  if(step==2){show_tap(238,130);step++;return NULL;}
  int lang=languages[index];
  if(step==3){show_tap(88+(lang%2)*147,56+(lang/2)*38);step++;return NULL;}
  if(step==4){
    if(tw_language()!=lang){dbg_log("Language picker failed: expected %d, got %d at step %u",lang,tw_language(),step);gfx_screenshot("language-failed.png");}
    assert(tw_language()==lang);assert(!tw_language_error()[0]);
    char saved[16];assert(store_load("language",saved,sizeof saved)&&!strcmp(saved,tw_language_code()));
    snprintf(shot,sizeof shot,"language-%s-selector",tw_language_code());step++;return shot;
  }
  if(step==5){show_tap(150,208);step++;return NULL;}
  if(step==6){snprintf(shot,sizeof shot,"language-%s-help",tw_language_code());step++;return shot;}
  float cell=SCREEN_W>=600?66:50;
  if(step==7){show_tap(SCREEN_W/2.f-2*cell,SCREEN_H-16);step++;return NULL;}
  if(step==8){snprintf(shot,sizeof shot,"language-%s-navigator",tw_language_code());step++;return shot;}
  if(step==9){show_tap(SCREEN_W/2.f-cell,SCREEN_H-16);step++;return NULL;}
  if(step==10){snprintf(shot,sizeof shot,"language-%s-catalogue",tw_language_code());step++;return shot;}
  if(step==11){client_ui_snow();step++;return NULL;}
  if(step==12){show_tap(260,168);step++;return NULL;}
  if(step==13){snprintf(shot,sizeof shot,"language-%s-snow-create",tw_language_code());step++;return shot;}
  if(step==14){client_ui_action(SHOW_B);client_ui_close();step++;return NULL;}
  if(step==15){client_ui_battle();step++;return NULL;}
  if(step==16){snprintf(shot,sizeof shot,"language-%s-battle",tw_language_code());step++;return shot;}
  if(step==17){client_ui_action(SHOW_B);step++;return NULL;}
  if(step==18){show_tap(SCREEN_W/2.f+3*cell,SCREEN_H-16);step++;return NULL;}
  if(step==19){show_tap(238,152);step++;return NULL;}
  if(++index==COUNT(languages)){ui_exercise_done=true;dbg_log("PASS: six languages selected through UI input, persisted, and drawn with their fonts.");return NULL;}
  show_tap(238,130);step=3;return NULL;
}
