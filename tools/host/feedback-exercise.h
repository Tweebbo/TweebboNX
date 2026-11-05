#include "avatar.h"
static const char *feedback_exercise(u64 elapsed) {
  static unsigned step;static u64 at;
  if(!W.in_room)return NULL;
  Ent *me=world_ent(W.me);if(!me)return NULL;
  if(!step) {
    tw_language_request(TW_FR);tw_language_update();
    assert(tw_language()==TW_FR);
    assert(!strcmp(TR("Hit"),"Hit"));assert(!strcmp(TR("Stand"),"Stand"));
    assert(!strcmp(TR("Deal again"),"Miser à nouveau"));
    assert(strcmp(TR("Talk/use"),"Talk/use"));
    Look look=look_decode(me->look);
    for(int i=0;i<wardrobe_tops_count;i++)if(wardrobe_tops[i].layers&&strstr(wardrobe_tops[i].layers,"rc:")){look.shirt_style=i;break;}
    me->look=look_encode(&look);me->drink=8;me->drink_until=now_ms()+60000;
    me->typing_until=now_ms()+60000;me->device=DEVICE_SWITCH;
    client_ui_close();at=elapsed;step++;return NULL;
  }
  if(step==1&&elapsed>at+1000){step++;return "feedback-fr-room";}
  return NULL;
}
