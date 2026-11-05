/* Visual fixtures exercise the shipped editor; network behavior is tested separately. */
static const char *magic_height_exercise(u64 elapsed){
  static unsigned step;static u64 next;
  if(elapsed<next||!W.in_room||step>=6)return NULL;
  next=elapsed+900;
  switch(step++){
    case 0:
      client_ui_close();
      furniture_receive("{\"op\":\"open\",\"kind\":\"magic_tile\",\"name\":\"Stacking Helper 1x1\",\"item\":42,\"definition\":40498,\"editable\":true,\"owned\":true,\"placed\":true,\"data\":{\"height\":1,\"pixelHeight\":0.32,\"minimum\":0,\"maximum\":4000}}");
      assert(furniture_panel.kind==F_MAGIC_TILE&&furniture_panel.field_count==2);
      client_ui_update();return "magic-height-editor";
    case 1:
      client_furniture_action(17005,1);
      assert(!strcmp(furniture_choice_label(1),"Match stack below"));
      return "magic-height-match-below";
    case 2:
      furniture_receive("{\"op\":\"saved\",\"kind\":\"magic_tile\",\"item\":42,\"data\":{\"height\":4,\"pixelHeight\":1.28,\"minimum\":2,\"maximum\":4000}}");
      assert(!strcmp(furniture_panel.fields[0].value,"0.04"));
      client_ui_update();return "magic-height-canonical-reply";
    case 3:
      client_ui_close();
      furniture_receive("{\"op\":\"open\",\"kind\":\"magic_tile\",\"name\":\"Walking Helper 1x1\",\"item\":42,\"definition\":40501,\"editable\":false,\"owned\":false,\"placed\":true,\"data\":{\"height\":100,\"pixelHeight\":32,\"minimum\":0}}");
      assert(!furniture_panel.editable&&!furniture_panel.fields[0].editable);
      client_ui_update();return "magic-height-readonly";
    case 4:
      client_ui_close();
      furniture_receive("{\"op\":\"open\",\"kind\":\"magic_tile\",\"name\":\"Stacking Helper 1x1\",\"item\":42,\"definition\":40498,\"editable\":false,\"owned\":true,\"placed\":false,\"data\":{}}");
      assert(!furniture_panel.editable&&!furniture_panel.field_count);
      client_ui_update();return "magic-height-inventory";
    default:
      ui_exercise_done=true;dbg_log("OK: exact height editor, Match Below, canonical saved reply, read-only and inventory visual states completed.");
      return "magic-height-complete";
  }
}
