#include "update.h"
static const char *update_ui_exercise(u64 elapsed) {
  static unsigned step;
  static u64 at;
  static bool progress_shown;
  bool fallback=getenv("UI_EXERCISE")&&!strcmp(getenv("UI_EXERCISE"),"update-fallback");
  if(elapsed<5500)return NULL;
  float x=8,y=8;
  switch(step){
    case 0:
      if(update_state()!=UPD_AVAILABLE)return NULL;
      assert(update_small() && update_download_bytes()<update_full_bytes());
      at=elapsed;step++;return "update-offer";
    case 1:
      if(elapsed-at<500)return NULL;
      show_tap(x+78,y+201);assert(update_dismissed());at=elapsed;step++;break;
    case 2:
      if(elapsed-at<500)return NULL;
      assert(update_dismissed());update_check();step++;break;
    case 3:
      if(update_state()!=UPD_AVAILABLE)return NULL;
      assert(!update_dismissed());at=elapsed;step++;return "update-offer-again";
    case 4:
      if(elapsed-at<500)return NULL;
      show_tap(x+229,y+201);step++;break;
    case 5:
      if(update_state()==UPD_DOWNLOADING&&!progress_shown){progress_shown=true;return "update-progress";}
      if(fallback&&update_state()==UPD_FAILED){assert(update_full_fallback());at=elapsed;step=7;return "update-fallback";}
      if(update_state()!=UPD_INSTALLED){assert(update_state()!=UPD_FAILED);return NULL;}
      assert(update_installed_path()[0]);at=elapsed;step++;return "update-prepared";
    case 6:
      if(elapsed-at<500)return NULL;
      show_tap(x+78,y+201);assert(update_dismissed());ui_exercise_done=true;
      dbg_log("PASS: optional update offer, Later, manual recheck, small download, preparation and close on device %d",TWEEBBO_DEVICE);step++;break;
    case 7:
      if(elapsed-at<500)return NULL;
      show_tap(x+229,y+201);step=8;break;
    case 8:
      if(update_state()!=UPD_INSTALLED){assert(update_state()!=UPD_FAILED);return NULL;}
      assert(update_installed_path()[0]);at=elapsed;step=6;return "update-full-prepared";
  }
  return NULL;
}
