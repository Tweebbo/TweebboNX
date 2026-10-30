static const char *room_controls_exercise(u64 elapsed) {
  static int step;
  static u64 at;
  if(!W.in_room || elapsed<1500 || elapsed-at<650)return NULL;
  at=elapsed;
  Ent *me=world_ent(W.me);
  assert(me);
  switch(step++) {
    case 0: {
      Ent saved=*me;
      memset(W.ents,0,sizeof W.ents);memset(W.items,0,sizeof W.items);
      saved.x=saved.fx=4;saved.y=saved.fy=4;saved.z=saved.fz=0;
      saved.flags=0;saved.action=0;saved.talk_until=saved.drink_until=0;
      saved.typing_until=now_ms()+30000;saved.look=0;
      W.ents[0]=saved;
      W.ents[1]=saved;W.ents[1].id=saved.id+100;W.ents[1].guest=123;
      W.ents[1].x=W.ents[1].fx=5;W.ents[1].typing_until=0;
      snprintf(W.ents[1].name,sizeof W.ents[1].name,"Test player");
      W.items[0]=(Item){.used=true,.id=900,.def=1,.x=3,.y=5,.owner=W.guest_id};
      W.items[1]=(Item){.used=true,.id=901,.def=61001,.x=5,.y=6,.owner=W.guest_id};
      W.owner=W.guest_id;W.room_owner=true;
      snprintf(W.model,sizeof W.model,"model_a");snprintf(W.slug,sizeof W.slug,"room-controls-fixture");
      snprintf(W.title,sizeof W.title,"Controls test fixture");
      W.w=W.h=12;memset(W.rows,'0',144);
      show_room_changed(now_ms());client_ui_close();show_toast("",0);
      return NULL;
    }
    case 1:assert(show_zoom()==1);return "room-standard";
    case 2:show_action(SHOW_L);assert(show_zoom()<1);return NULL;
    case 3:show_action(SHOW_L);assert(show_zoom()<1);return "room-overview";
    case 4:show_action(SHOW_R);assert(show_zoom()==1);return NULL;
    case 5:show_action(SHOW_R);assert(show_zoom()==2);return NULL;
    case 6:show_action(SHOW_R);assert(show_zoom()==2);return "room-close";
    case 7: {
      float x,y;show_to_screen(32,(5+4)*16+16,&x,&y);
      show_tap(x,y-70*show_zoom());assert(client_ui_room_left()>0);
      return "room-player-menu";
    }
    case 8:client_ui_close();show_action(SHOW_L);return NULL;
    case 9: {
      const Item *it=world_item(900);
      const FurniVariant *v=variant_frame(it->def,it->rot,it->state,0);assert(v);
      for(int j=0;j<v->count;j++) {
        const FurniPart *p=&v->parts[j];if((p->flags&PART_SHADOW)||!p->alpha)continue;
        float x,y;show_to_screen((it->x-it->y)*32+p->ox+p->w*.5f,(it->x+it->y)*16+p->oy+p->h*.5f,&x,&y);
        show_tap(x,y);assert(client_ui_room_left()>0);break;
      }
      return "room-furniture-menu";
    }
    case 10:client_ui_close();return NULL;
    case 11: {
      Item *it=world_item(901);it->fx=it->x;it->fy=it->y;it->slide_at=now_ms();it->x++;it->state=3;
      return "room-ball-start";
    }
    case 12:return "room-ball-end";
    case 13:ui_exercise_done=true;dbg_log("PASS: room overview/standard/close-up and limits, touch player/furniture menus, typing bubble and ball slide rendered at Vita resolution.");return NULL;
  }
  return NULL;
}
