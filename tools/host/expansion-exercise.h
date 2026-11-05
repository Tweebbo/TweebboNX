/* Visual fixtures use the actual portable furniture controls and renderer. */
#include "furniture.h"
#include "client_furniture.h"
static void expansion_variable_open(const char *name){
  char path[512];snprintf(path,sizeof path,"%s/variable-editors.json",data_dir);
  json_t *editors=json_load_file(path,JSON_REJECT_DUPLICATES,NULL);assert(editors);
  json_t *editor=json_object_get(editors,name);assert(editor);char *raw=json_dumps(editor,JSON_COMPACT);assert(raw);
  client_ui_close();furniture_receive(raw);free(raw);json_decref(editors);client_ui_update();
}
static const char *expansion_ui_exercise(u64 elapsed) {
  static unsigned step;
  static u64 next;
  static const char *kinds[]={"note","trophy","moodlight","gift","mannequin","changing","wired"};
  static const char *names[]={"Sticky Note","Permanent Trophy","Mood Light","Gift","Mannequin","Changing Booth","Wired: Toggle state"};
  static char shot[64];
  if(elapsed<next || !W.in_room || step>=95)return NULL;
  next=elapsed+900;
  if(step<7){
    client_ui_close();
    json_t *data=json_object();
    if(step==0 || step==1 || step==3){
      char message[3500];message[0]=0;
      for(int i=0;i<12;i++)strcat(message,"An original furniture interaction, with readable text across small screens. ");
      json_object_set_new(data,step==0?"text":"message",json_string(message));
      json_object_set_new(data,"sender",json_string("Fixture Player"));
      if(step==3)json_object_set_new(data,"filled",json_true());
    }
    if(step==2){
      json_object_set_new(data,"light",json_pack("{s:b,s:s,s:i,s:b}","enabled",1,"color","#225577","intensity",96,"backgroundOnly",1));
    }
    if(step==4 || step==5){json_object_set_new(data,"look",json_integer(0));json_object_set_new(data,"name",json_string("Evening outfit"));}
    json_t *o=json_pack("{s:s,s:s,s:s,s:i,s:i,s:b,s:b,s:b,s:o}","op","open","kind",kinds[step],"name",names[step],"item",42,"definition",1,"editable",1,"owned",1,"placed",1,"data",data);
    if(step==6){
      json_t *wired=json_pack("{s:b,s:o,s:o,s:o}","supported",1,
        "config",json_pack("{s:s,s:s,s:i,s:[],s:[],s:[i,i,i,i]}","operation","wf_act_toggle_state","text","","delay",0,"targets","users","params",0,0,0,0),
        "operations",json_pack("[{s:s,s:s,s:b}]","name","wf_act_toggle_state","label","Toggle state","supported",1),
        "items",json_pack("[{s:i,s:s}]","id",88,"name","Lamp"));
      assert(wired);json_object_set_new(o,"wired",wired);
    }
    char *raw=json_dumps(o,JSON_COMPACT);assert(raw);furniture_receive(raw);free(raw);json_decref(o);
    client_ui_update();snprintf(shot,sizeof shot,"expansion-%s",kinds[step]);
  }else switch(step){
    case 7:client_furniture_action(17003,1);snprintf(shot,sizeof shot,"expansion-wired-page2");break;
    case 8:client_furniture_action(17007,0);assert(furniture_target_check()==1);snprintf(shot,sizeof shot,"expansion-wired-selection");break;
    case 9:
      client_ui_close();furniture_receive("{\"op\":\"open\",\"kind\":\"note\",\"name\":\"Sticky Note\",\"item\":42,\"editable\":true,\"owned\":true,\"data\":{\"text\":\"A note with several lines that can be read in the original native window. The reader preserves every word, including the end of a long inscription.\"}}");client_ui_update();
      snprintf(shot,sizeof shot,"expansion-note-reader-ready");break;
    case 10:client_furniture_action(17009,0);snprintf(shot,sizeof shot,"expansion-note-reader");break;
    case 11:client_furniture_action(17010,0);snprintf(shot,sizeof shot,"expansion-note-reader-back");break;
    case 12:
      client_ui_close();furniture_receive("{\"op\":\"state\",\"light\":{\"enabled\":true,\"color\":\"#225577\",\"intensity\":96,\"backgroundOnly\":true},\"arena\":{\"kind\":\"freeze\",\"running\":true,\"remaining\":120000,\"scores\":[30,20,0,0],\"players\":{}}}");
      snprintf(shot,sizeof shot,"expansion-light-and-freeze");break;
    case 13:
      client_ui_close();furniture_receive("{\"op\":\"state\",\"light\":{\"enabled\":false,\"color\":\"#FFFFFF\",\"intensity\":255,\"backgroundOnly\":false},\"arena\":{\"kind\":\"banzai\",\"running\":true,\"remaining\":120000,\"scores\":[12,8,0,0],\"players\":{}}}");
      snprintf(shot,sizeof shot,"expansion-banzai");break;
    case 14:
      furniture_receive("{\"op\":\"open\",\"kind\":\"changing\",\"name\":\"Changing Booth\",\"item\":42,\"editable\":false,\"owned\":false,\"data\":{}}");client_ui_update();
      snprintf(shot,sizeof shot,"expansion-changing-visitor");break;
    case 15:
      client_furniture_action(17014,0);snprintf(shot,sizeof shot,"expansion-changing-wardrobe");break;
    case 16:{
      static const char *labels[]={"Hour: any 0, exact 1, range 2","Hour from (0-23)","Hour to (0-23)","Minute: any 0, exact 1, range 2","Minute from (0-59)","Minute to (0-59)","Second: any 0, exact 1, range 2","Second from (0-59)","Second to (0-59)"};
      json_t *config=json_pack("{s:s,s:s,s:i,s:[],s:[],s:[]}","operation","wf_cnd_match_time","text","America/Santiago","delay",0,"targets","users","params");
      json_t *parameters=json_array();
      for(int i=0;i<9;i++){json_array_append_new(json_object_get(config,"params"),json_integer(0));json_array_append_new(parameters,json_pack("{s:s,s:i}","label",labels[i],"default",0));}
      json_t *wired=json_pack("{s:b,s:s,s:o,s:o,s:o,s:[],s:o}","supported",1,"textLabel","IANA time zone (empty: UTC)","config",config,"parameters",parameters,
        "operations",json_pack("[{s:s,s:s,s:b}]","name","wf_cnd_match_time","label","Time matches","supported",1),"items","users",json_pack("[{s:i,s:s}]","id",77,"name","Selected Player"));
      json_t *o=json_pack("{s:s,s:s,s:s,s:i,s:b,s:{},s:o}","op","open","kind","wired","name","Wired: Time matches","item",42,"editable",1,"data","wired",wired);
      assert(o);char *raw=json_dumps(o,JSON_COMPACT);furniture_receive(raw);free(raw);json_decref(o);client_ui_update();assert(furniture_panel.field_count==12);
      snprintf(shot,sizeof shot,"expansion-calendar-page1");break;
    }
    case 17:case 18:case 19:case 20:case 21:
      client_furniture_action(17003,1);snprintf(shot,sizeof shot,"expansion-calendar-page%u",step-15);break;
    case 22:
      furniture_panel.selected=11;assert(furniture_edit("59"));
      client_furniture_action(17013,0);client_furniture_action(17007,0);assert(furniture_target_check()==1&&!strcmp(furniture_target_label(),"Selected Player"));
      snprintf(shot,sizeof shot,"expansion-calendar-player");break;
    case 23:
      client_ui_close();
      furniture_receive("{\"op\":\"open\",\"kind\":\"clock\",\"name\":\"Small Wired Counter\",\"item\":42,\"editable\":true,\"data\":{\"time\":\"1:01\",\"running\":true}}");client_ui_update();
      client_furniture_action(17004,1);assert(!strcmp(furniture_choice_label(1),"Set time"));
      snprintf(shot,sizeof shot,"expansion-clock");break;
    case 24:
      client_furniture_action(17003,1);furniture_panel.selected=2;assert(furniture_edit("1"));furniture_panel.selected=3;assert(furniture_edit("3"));
      snprintf(shot,sizeof shot,"expansion-clock-adjust");break;
    case 25:
      client_ui_close();snprintf(shot,sizeof shot,"expansion-counter-scene");
      break;
    case 26:{
      json_t *config=json_pack("{s:s,s:s,s:i,s:[],s:[],s:[]}","operation","wf_cnd_match_time","text","UTC","delay",0,"targets","users","params");
      json_t *parameters=json_array();
      for(int i=0;i<32;i++){char label[48];snprintf(label,sizeof label,"Advanced setting %d",i+1);json_array_append_new(json_object_get(config,"params"),json_integer(i));json_array_append_new(parameters,json_pack("{s:s,s:i}","label",label,"default",0));}
      json_t *wired=json_pack("{s:b,s:o,s:o,s:[],s:[],s:[]}","supported",1,"config",config,"parameters",parameters,"operations","items","users");
      json_t *o=json_pack("{s:s,s:s,s:s,s:i,s:b,s:{},s:o}","op","open","kind","wired","name","Advanced Wired settings","item",42,"editable",1,"data","wired",wired);
      assert(o);char *raw=json_dumps(o,JSON_COMPACT);assert(raw);furniture_receive(raw);free(raw);json_decref(o);client_ui_update();assert(furniture_panel.field_count==35);
      snprintf(shot,sizeof shot,"expansion-wired-max-first");break;
    }
    case 27:
      furniture_panel.selected=34;assert(furniture_edit("9876543210"));client_furniture_action(17003,99);
      snprintf(shot,sizeof shot,"expansion-wired-max-last");break;
    case 28:
      client_ui_close();
      furniture_receive("{\"op\":\"open\",\"kind\":\"football_gate\",\"name\":\"Football Gate\",\"item\":42,\"editable\":true,\"data\":{\"name\":\"Home kit\",\"gender\":\"Female\",\"look\":1234}}");client_ui_update();
      assert(furniture_panel.kind==F_FOOTBALL_GATE&&furniture_panel.field_count==2&&!furniture_panel.can_apply);
      snprintf(shot,sizeof shot,"expansion-football-gate");break;
    case 29:
      client_furniture_action(17014,0);
      snprintf(shot,sizeof shot,"expansion-football-wardrobe");break;
    case 30:
      client_ui_close();
      furniture_receive("{\"op\":\"open\",\"kind\":\"wired\",\"item\":47,\"name\":\"Change variable\",\"editable\":true,\"data\":{},\"wired\":{\"supported\":true,\"config\":{\"operation\":\"wf_act_change_var_val\",\"text\":\"counter\",\"delay\":0,\"targets\":[],\"users\":[],\"params\":[9223372036854775807,1]},\"parameters\":[{\"label\":\"Operand\"},{\"label\":\"Operation\",\"choices\":[{\"value\":0,\"label\":\"Set\"},{\"value\":1,\"label\":\"Add\"},{\"value\":112,\"label\":\"Find set bit\"}]}],\"operations\":[{\"name\":\"wf_act_change_var_val\",\"label\":\"Change variable value\",\"supported\":true}],\"items\":[],\"users\":[]}}");client_ui_update();
      assert(furniture_has_choices(4)&&!strcmp(furniture_choice_label(4),"Add"));
      snprintf(shot,sizeof shot,"expansion-arithmetic-open");break;
    case 31:
      client_furniture_action(17003,1);client_furniture_action(17003,1);
      snprintf(shot,sizeof shot,"expansion-arithmetic");break;
    case 32:
      client_furniture_action(17005,4);assert(!strcmp(furniture_choice_label(4),"Find set bit"));
      snprintf(shot,sizeof shot,"expansion-arithmetic-choice");break;
    case 33:
      expansion_variable_open("user");snprintf(shot,sizeof shot,"expansion-variable-definition");break;
    case 34:
      client_furniture_action(17003,1);assert(furniture_has_choices(4)&&!strcmp(furniture_choice_label(4),"Persistent"));
      snprintf(shot,sizeof shot,"expansion-variable-availability");break;
    case 35:
      expansion_variable_open("change");snprintf(shot,sizeof shot,"expansion-variable-open");break;
    case 36:
      client_furniture_action(17003,1);client_furniture_action(17003,1);
      assert(furniture_has_choices(5)&&strstr(furniture_choice_label(5),"personal_total"));
      snprintf(shot,sizeof shot,"expansion-variable-change");break;
    case 37:
      client_furniture_action(17005,5);assert(strstr(furniture_choice_label(5),"room_total"));
      snprintf(shot,sizeof shot,"expansion-variable-choice");break;
    case 38:
      expansion_variable_open("condition");snprintf(shot,sizeof shot,"expansion-variable-condition-open");break;
    case 39:
      client_furniture_action(17003,1);client_furniture_action(17003,1);
      assert(furniture_has_choices(4)&&!strcmp(furniture_choice_label(4),"Equal"));
      snprintf(shot,sizeof shot,"expansion-variable-condition");break;
    case 40:
      expansion_variable_open("context");snprintf(shot,sizeof shot,"expansion-variable-context");break;
    case 41:
      expansion_variable_open("referencedChange");snprintf(shot,sizeof shot,"expansion-operand-open");break;
    case 42:
      for(int i=0;i<4;i++)client_furniture_action(17003,1);
      assert(furniture_has_choices(8)&&strstr(furniture_choice_label(8),"factor"));
      snprintf(shot,sizeof shot,"expansion-operand");break;
    case 43:
      client_furniture_action(17003,1);snprintf(shot,sizeof shot,"expansion-operand-same-target");break;
    case 44:
      expansion_variable_open("userSelector");snprintf(shot,sizeof shot,"expansion-selector-open");break;
    case 45:
      for(int i=0;i<3;i++)client_furniture_action(17003,1);
      assert(furniture_has_choices(6)&&strstr(furniture_choice_label(6),"personal_total"));
      assert(furniture_has_choices(7)&&strstr(furniture_choice_label(7),"factor"));
      snprintf(shot,sizeof shot,"expansion-selector-variable");break;
    case 46:
      client_furniture_action(17003,1);client_furniture_action(17003,1);
      assert(strstr(furniture_panel.fields[10].label,"Filter"));
      snprintf(shot,sizeof shot,"expansion-selector-flags");break;
    case 47:
      expansion_variable_open("furniSelector");snprintf(shot,sizeof shot,"expansion-selector-furniture");break;
    case 48:
      expansion_variable_open("animationTime");assert(furniture_panel.compact_motion&&furniture_display_count()==1);
      snprintf(shot,sizeof shot,"expansion-animation-time");break;
    case 49:
      furniture_panel.selected=3;assert(furniture_edit("2000"));client_ui_changed();
      snprintf(shot,sizeof shot,"expansion-animation-adjust");break;
    case 50:
      expansion_variable_open("instantMotion");assert(furniture_panel.compact_motion&&!furniture_display_count());
      snprintf(shot,sizeof shot,"expansion-instant-motion");break;
    case 51:
      expansion_variable_open("echo");assert(furniture_panel.variable_definition&&furniture_display_count()==2);
      assert(strstr(furniture_choice_label(3),"remote_total"));snprintf(shot,sizeof shot,"expansion-variable-echo");break;
    case 52:
      expansion_variable_open("reference");assert(furniture_panel.variable_definition&&furniture_display_count()==3);
      assert(atoll(furniture_panel.fields[3].value)>(1LL<<34));assert(strstr(furniture_choice_label(3),"shared_total"));
      snprintf(shot,sizeof shot,"expansion-variable-reference");break;
    case 53:
      furniture_choice(4,1);assert(!strcmp(furniture_choice_label(4),"Read only"));client_furniture_action(17003,1);
      snprintf(shot,sizeof shot,"expansion-variable-readonly");break;
    case 54:
      furniture_save();snprintf(shot,sizeof shot,"expansion-variable-reference-request");break;
    case 55:
      expansion_variable_open("moveAvatar");assert(furniture_player_motion()&&furniture_display_count()==4);
      snprintf(shot,sizeof shot,"expansion-avatar-move");break;
    case 56:
      client_furniture_action(17003,1);assert(!strcmp(furniture_choice_label(4),"Clockwise"));
      snprintf(shot,sizeof shot,"expansion-avatar-direction");break;
    case 57:
      furniture_choice(4,1);assert(!strcmp(furniture_choice_label(4),"Counterclockwise"));client_ui_changed();
      snprintf(shot,sizeof shot,"expansion-avatar-rotation");break;
    case 58:
      furniture_panel.selected=5;assert(furniture_edit("100"));furniture_target(0,true);assert(furniture_target_check()==1);furniture_save();
      snprintf(shot,sizeof shot,"expansion-avatar-picks-request");break;
    case 59:
      expansion_variable_open("carryUsers");assert(furniture_player_motion()&&furniture_display_count()==2);
      assert(!strcmp(furniture_choice_label(4),"All players"));snprintf(shot,sizeof shot,"expansion-carry-users");break;
    case 60:
      furniture_choice(3,1);client_ui_changed();assert(!strcmp(furniture_choice_label(3),"Anywhere on the same tiles"));
      snprintf(shot,sizeof shot,"expansion-carry-mode");break;
    case 61:
      furniture_save();snprintf(shot,sizeof shot,"expansion-carry-request");break;
    case 62:
      expansion_variable_open("physics");assert(furniture_panel.compact_motion&&furniture_target_picker()&&!furniture_player_motion());
      assert(furniture_display_count()==7);snprintf(shot,sizeof shot,"expansion-physics");break;
    case 63:
      client_furniture_action(17003,1);snprintf(shot,sizeof shot,"expansion-physics-page2");break;
    case 64:
      for(int i=0;i<4;i++)client_furniture_action(17003,1);
      snprintf(shot,sizeof shot,"expansion-physics-last");break;
    case 65:
      furniture_target(0,true);assert(!strcmp(furniture_target_kind(),"Primary"));snprintf(shot,sizeof shot,"expansion-physics-primary");break;
    case 66:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Secondary"));snprintf(shot,sizeof shot,"expansion-physics-secondary");break;
    case 67:
      furniture_target(0,true);snprintf(shot,sizeof shot,"expansion-physics-secondary-pick");break;
    case 68:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Players"));snprintf(shot,sizeof shot,"expansion-physics-players");break;
    case 69:
      furniture_target(0,true);furniture_save();snprintf(shot,sizeof shot,"expansion-physics-request");break;
    case 70:
      expansion_variable_open("furniToFurni");assert(furniture_display_count()==3&&furniture_display_index(0)==2);
      snprintf(shot,sizeof shot,"expansion-furni-destination");break;
    case 71:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Secondary"));client_ui_changed();
      snprintf(shot,sizeof shot,"expansion-furni-destination-secondary");break;
    case 72:
      furniture_target(0,true);furniture_save();snprintf(shot,sizeof shot,"expansion-furni-destination-request");break;
    case 73:
      expansion_variable_open("furniToUser");assert(furniture_display_count()==3);snprintf(shot,sizeof shot,"expansion-player-destination");break;
    case 74:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Players"));client_ui_changed();snprintf(shot,sizeof shot,"expansion-player-destination-picks");break;
    case 75:
      furniture_target(0,true);furniture_save();snprintf(shot,sizeof shot,"expansion-player-destination-request");break;
    case 76:
      expansion_variable_open("groupMovement");assert(furniture_display_count()==8);snprintf(shot,sizeof shot,"expansion-group-destination");break;
    case 77:
      client_furniture_action(17003,1);snprintf(shot,sizeof shot,"expansion-group-offset");break;
    case 78:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Secondary"));client_ui_changed();snprintf(shot,sizeof shot,"expansion-group-destination-picks");break;
    case 79:
      expansion_variable_open("moveToOffset");assert(furniture_display_count()==5);snprintf(shot,sizeof shot,"expansion-destination-offset");break;
    case 80:
      client_furniture_action(17003,1);snprintf(shot,sizeof shot,"expansion-destination-spacing");break;
    case 81:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Secondary"));furniture_target(0,true);furniture_save();snprintf(shot,sizeof shot,"expansion-destination-offset-request");break;
    case 82:
      expansion_variable_open("headingMovement");assert(furniture_display_count()==5&&!furniture_target_switchable());snprintf(shot,sizeof shot,"expansion-heading");break;
    case 83:
      client_furniture_action(17003,1);assert(!strcmp(furniture_choice_label(4),"Turn right 90 degrees"));snprintf(shot,sizeof shot,"expansion-heading-rules");break;
    case 84:
      furniture_choice(4,1);client_ui_changed();assert(!strcmp(furniture_choice_label(4),"Turn left 45 degrees"));snprintf(shot,sizeof shot,"expansion-heading-turn");break;
    case 85:
      furniture_toggle_target_kind();assert(!strcmp(furniture_target_kind(),"Primary"));furniture_target(0,true);furniture_save();snprintf(shot,sizeof shot,"expansion-heading-request");break;
    case 86:
      expansion_variable_open("chaseMovement");assert(furniture_display_count()==2&&!furniture_target_switchable());snprintf(shot,sizeof shot,"expansion-chase");break;
    case 87:
      furniture_choice(3,1);client_ui_changed();assert(!strcmp(furniture_choice_label(3),"Selectors"));snprintf(shot,sizeof shot,"expansion-chase-source");break;
    case 88:
      furniture_save();snprintf(shot,sizeof shot,"expansion-chase-request");break;
    case 89:
      expansion_variable_open("fleeMovement");assert(furniture_display_count()==2);snprintf(shot,sizeof shot,"expansion-flee");break;
    case 90:
      furniture_choice(3,-1);client_ui_changed();assert(!strcmp(furniture_choice_label(3),"Trigger"));snprintf(shot,sizeof shot,"expansion-flee-source");break;
    case 91:
      furniture_save();snprintf(shot,sizeof shot,"expansion-flee-request");break;
    case 92:
      expansion_variable_open("collisionTrigger");assert(!furniture_display_count()&&!furniture_target_picker());assert(strstr(furniture_help_text(),"collision"));snprintf(shot,sizeof shot,"expansion-collision-trigger");break;
    case 93:
      furniture_save();snprintf(shot,sizeof shot,"expansion-collision-request");break;
    case 94:
      client_ui_close();snprintf(shot,sizeof shot,"expansion-complete");
      ui_exercise_done=true;dbg_log("OK: furniture, reader, wardrobe, football kit, calendar, 32-setting editor, selections, clocks, arithmetic, actual-server scalar forms, shared references and Echo, typed selectors and movement timing/destinations, independent source/target scopes, retained heading rules, nearest-player motion and collision forms, counter scene, moodlight and arena visual fixtures completed.");break;
  }
  step++;return shot;
}
