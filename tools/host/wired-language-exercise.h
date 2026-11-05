#include "language.h"
#include "wired_services.h"
#include "wired_chests.h"

static const char *wired_language_exercise(u64 elapsed){
  static unsigned step,index;
  static u64 at;
  static char shot[64];
  static const int languages[]={TW_FR,TW_PT_BR,TW_ZH,TW_JA,TW_ES,TW_EN};
  if(!W.in_room||elapsed<at+650||ui_exercise_done)return NULL;
  at=elapsed;
  if(step==0){client_ui_close();tw_language_request(languages[index]);step++;return NULL;}
  if(step==1){
    assert(tw_language()==languages[index]);assert(!tw_language_error()[0]);
    char saved[16];assert(store_load("language",saved,sizeof saved)&&!strcmp(saved,tw_language_code()));
    snprintf(shot,sizeof shot,"language-%s-room",tw_language_code());step++;return shot;
  }
  if(step==2){
    client_ui_close();client_ui_services(WS_BOTS);
    on_furniture("{\"op\":\"bots\",\"offers\":[],\"bots\":[{\"id\":900001,\"name\":\"Player text stays English\",\"room\":null,\"kind\":0}]}");
    assert(!strcmp(wired_service_panel.title,"Room bots"));
    snprintf(shot,sizeof shot,"language-%s-wired-bots",tw_language_code());step++;return shot;
  }
  if(step==3){
    client_ui_close();client_ui_services(WS_GROUPS);
    on_furniture("{\"op\":\"groups\",\"canCreate\":false,\"clubActive\":true,\"price\":150,\"groups\":[{\"id\":900002,\"name\":\"Player text stays English\",\"description\":\"Written by a player\",\"ownerName\":\"Player name\",\"permissions\":{}}]}");
    assert(!strcmp(wired_service_panel.title,"Groups"));
    snprintf(shot,sizeof shot,"language-%s-wired-groups",tw_language_code());step++;return shot;
  }
  if(step==4){
    client_ui_close();client_ui_services(WS_QUESTS);
    on_furniture("{\"op\":\"quests\",\"quests\":[{\"id\":900003,\"name\":\"Place furniture\",\"progress\":2,\"target\":5,\"reward\":20,\"active\":false,\"available\":true,\"completedSteps\":1,\"totalSteps\":4}]}");
    wired_services_select(0);
    snprintf(shot,sizeof shot,"language-%s-wired-quest",tw_language_code());step++;return shot;
  }
  if(step==5){
    client_ui_close();on_furniture("{\"op\":\"open\",\"kind\":\"wired_label\",\"item\":900004,\"definition\":40519,\"editable\":true,\"data\":{\"text\":\"Player text stays English\",\"color\":4278912,\"visibility\":0,\"alwaysOpen\":true}}");
    assert(!strcmp(wired_chest_panel.body,"Player text stays English"));
    wired_chests_action(WC_CONFIGURE);
    snprintf(shot,sizeof shot,"language-%s-wired-label",tw_language_code());step++;return shot;
  }
  if(step==6){
    client_ui_close();on_furniture("{\"op\":\"open\",\"kind\":\"wired_chest\",\"item\":900005,\"definition\":40514,\"owned\":true,\"editable\":true,\"data\":{\"kind\":\"coins\",\"coins\":100,\"level\":1,\"maximumCapacity\":1000,\"settings\":{\"name\":\"Player text stays English\",\"capacity\":100,\"locked\":false},\"permissions\":{\"settings\":true}}}");
    wired_chests_action(WC_CONFIGURE);
    snprintf(shot,sizeof shot,"language-%s-wired-chest",tw_language_code());step++;return shot;
  }
  if(step==7){
    client_ui_close();on_furniture("{\"op\":\"open\",\"kind\":\"wired_contract\",\"item\":900006,\"definition\":40521,\"owned\":true,\"editable\":true,\"data\":{\"contract\":{\"kind\":1,\"paymentMode\":1,\"payment\":[[]],\"reward\":[],\"receiveText\":\"Player text stays English\",\"rewardText\":\"Written by a player\",\"layout\":\"generic\",\"rewardCategory\":11,\"showDialog\":true}}}");
    snprintf(shot,sizeof shot,"language-%s-wired-contract",tw_language_code());step++;return shot;
  }
  if(step==8){
    client_ui_close();on_furniture("{\"type\":\"wired_link\",\"item\":900007,\"result\":0,\"network\":\"0\",\"receiveLocal\":false,\"networks\":[]}");
    snprintf(shot,sizeof shot,"language-%s-wired-link",tw_language_code());step++;return shot;
  }
  client_ui_close();step=0;
  if(++index==COUNT(languages)){
    ui_exercise_done=true;
    dbg_log("PASS: six languages selected through the public language API, persisted, and rendered in seven Wired service views; player text retained. Desktop SDL only.");
  }
  return NULL;
}
