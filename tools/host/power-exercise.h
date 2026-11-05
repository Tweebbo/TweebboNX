static bool power_exercise(u64 elapsed) {
  static bool initialized, keyboard;
  if (!W.in_room) return false;
  if (!initialized) {
    snprintf(W.slug, sizeof W.slug, "power-fixture");
    show_room_changed(now_ms());
    client_ui_close();
    show_toast("", 0);
    initialized = true;
  }
  /* Offline bots normally walk and chat. Hold this synthetic room still to
     exercise the shipped idle classifier and scheduler deterministically. */
  for (int i=0; i<MAX_ENTS; i++) {
    Ent *e=&W.ents[i];
    e->fx=e->x; e->fy=e->y; e->fz=e->z;
    e->flags &= ~(FLAG_WAVE | FLAG_DANCE | FLAG_ROLL);
    e->talk_until=e->typing_until=e->drink_until=0; e->action=0;
  }
  if (elapsed >= 9000 && !keyboard) {
    show_action(SHOW_Y);
    keyboard=true;
    assert(client_ui_animation_active());
    return true;
  }
  if (elapsed >= 12000) {
    client_ui_close();
    Ent *me=world_ent(W.me);
    assert(me);
    me->fx=me->x-1; me->t0=now_ms();
    assert(show_animation_active(now_ms()));
  }
  return false;
}
