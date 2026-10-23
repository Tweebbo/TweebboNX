#include "feedback.h"
static const char *reception_ui_exercise(u64 elapsed) {
  static int step;static u64 at;static u32 lia;
  if(elapsed<2500 || elapsed<at+900)return NULL;
  assert(W.in_room && net_online());
  const char *shot=NULL;
  switch(step) {
    case 0:
      for(int i=0;i<MAX_ENTS;i++)if(W.ents[i].used && feedback_receptionist(W.ents[i].guest,W.model))lia=W.ents[i].id;
      assert(lia);send_walk(9,5);break;
    case 1:if(elapsed<at+5000)return NULL;shot="reception-lia";break;
    case 2:client_ui_person(lia);break;
    case 3:shot="reception-profile";break;
    case 4:show_tap(220,210);break;
    case 5:shot="suggestion-box";break;
    case 6:show_tap(220,210);break;
    case 7:assert(strstr(feedback.status,"subject"));shot="suggestion-validation";break;
    case 8:show_tap(100,121);break;
    case 9:show_tap(30,96);show_tap(60,96);show_tap(90,96);break;
    case 10:show_tap(292,199);break;
    case 11:assert(strlen(feedback.subject)>=3);show_tap(100,150);break;
    case 12:for(int i=0;i<12;i++)show_tap(30+(i%8)*28,96);break;
    case 13:show_tap(292,199);break;
    case 14:assert(strlen(feedback.message)>=10);shot="suggestion-draft";break;
    case 15:client_ui_close();client_ui_feedback();assert(*feedback.subject && *feedback.message);break;
    case 16:show_tap(220,210);break;
    case 17:
      if(feedback.busy){assert(elapsed<at+25000);return NULL;}
      assert(strstr(feedback.status,"Thank you"));assert(!*feedback.subject && !*feedback.message);
      shot="suggestion-sent";break;
    case 18:client_ui_close();show_toast("You bought: Camera. Find it in your inventory.",now_ms());break;
    case 19:shot="notice-border";break;
    case 20:ui_exercise_done=true;dbg_log("OK: receptionist, Suggestions, validation, keyboard, retained draft and isolated delivery.");break;
  }
  step++;at=elapsed;return shot;
}
