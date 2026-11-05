static const char *magic_gameplay_exercise(u64 elapsed){
  static unsigned step;static u64 next,deadline;static u32 helper_id;
  if(elapsed<next||!W.in_room||strcmp(W.slug,"magic-gameplay"))return NULL;
  Ent *me=world_ent(W.me);if(!me)return NULL;
  switch(step){
    case 0:{
      const Item *helper=world_item_at(6,4);assert(helper&&def_height_helper(def_get(helper->def))==2&&item_height100(helper)==128000);
      helper_id=helper->id;furniture_open(helper_id);deadline=elapsed+20000;step=1;return NULL;
    }
    case 1:
      assert(elapsed<deadline);
      if(furniture_panel.item!=helper_id||furniture_panel.field_count<1)return NULL;
      furniture_panel.selected=0;assert(furniture_edit("0.00"));furniture_save();step=2;return NULL;
    case 2:
      assert(elapsed<deadline);
      if(item_height100(world_item(helper_id))!=0)return NULL;
      client_ui_close();show_walk(6-me->x,4-me->y);step=3;return NULL;
    case 3:
      assert(elapsed<deadline);
      if(me->x!=6||me->y!=4||ent_moving(me,now_ms()))return NULL;
      assert(me->z==0&&me->z_fine==0);furniture_open(helper_id);step=4;return NULL;
    case 4:
      assert(elapsed<deadline);
      if(furniture_panel.item!=helper_id||furniture_panel.field_count<1)return NULL;
      furniture_panel.selected=0;assert(furniture_edit("40.00"));furniture_save();step=5;return NULL;
    case 5:
      assert(elapsed<deadline);
      if(me->z!=1280||me->z_fine!=0||ent_moving(me,now_ms()))return NULL;
      assert(me->z==1280&&me->z_fine==0&&!(me->flags&(FLAG_SIT|FLAG_LAY)));
      client_ui_close();step=6;next=elapsed+1600;return NULL;
    case 6:
      step=7;next=elapsed+900;return "magic-gameplay-height-40";
    case 7:
      show_action(SHOW_R);step=8;next=elapsed+1200;return NULL;
    case 8:
      step=9;return "magic-gameplay-height-40-zoom";
    case 9:
      show_walk(5-me->x,4-me->y);deadline=elapsed+20000;step=10;return NULL;
    case 10:
      assert(elapsed<deadline);
      if(me->x!=5||me->y!=4||ent_moving(me,now_ms()))return NULL;
      assert(me->z==0&&me->z_fine==0);step=11;return "magic-gameplay-descended";
    default:
      ui_exercise_done=true;dbg_log("OK: native walk input, actual helper at 40 source units, exact 1280-pixel avatar state, clear seat flags, both camera views and descent to ground completed.");
      return "magic-gameplay-complete";
  }
}
