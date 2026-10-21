/* Synthetic room snapshot for the real native renderer; never sent to the hotel. */
static const char *bed_ui_exercise(u64 elapsed) {
  static bool initialized;
  if (elapsed < 2500 || !W.in_room) return NULL;
  if (!initialized) {
    initialized = true;
    memset(W.items, 0, sizeof W.items);
    memset(W.ents, 0, sizeof W.ents);
    W.me = 1; W.w = W.h = 10; W.door_x = 0; W.door_y = 5;
    memset(W.rows, '0', 100);
    snprintf(W.model, sizeof W.model, "model_a");
    snprintf(W.title, sizeof W.title, "Bed alignment fixture");
    for (int i = 0; i < 2; i++) {
      W.items[i] = (Item){.used=true,.id=900+i,.def=1020,.x=2+i*4,.y=3,.rot=i};
      W.ents[i] = (Ent){.used=true,.id=1+i,.guest=1+i,.x=2+i*4,.y=3,.fx=2+i*4,.fy=3,.dir=i*2,.flags=FLAG_LAY,.move_dir=-1};
      snprintf(W.ents[i].name, sizeof W.ents[i].name, "Pillow %d", i+1);
    }
    gfx_furni_preload(1020);
    show_room_changed(now_ms());
  }
  if (elapsed > 6000) {ui_exercise_done=true; return "beds";}
  return NULL;
}
