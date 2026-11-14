// The gate between menus and room walking in show_walk_pad, with the client UI stubbed.
#include <assert.h>
#include <stdio.h>
#include "../source/show.c"

static bool ui_active;
static int ui_moves, room_sends, room_x, room_y;

bool client_ui_pad_active(void) { return ui_active; }
void client_ui_pad_input(void) {}
bool client_ui_move(int dx, int dy) { (void)dx; (void)dy; ui_moves++; return true; }
int pool_mode(void) { return 0; }
void net_activity(void) {}
void send_walk(int x, int y) { room_sends++; room_x = x; room_y = y; }
bool world_walkable_tile(int x, int y) { (void)x; (void)y; return true; }
bool world_walk_passable(int x, int y, bool goal) { (void)x; (void)y; (void)goal; return true; }

World W;
static Ent me = {.used = true, .id = 1, .x = 4, .y = 4, .next_x = 255, .next_y = 255};
Ent *world_ent(u32 id) { return id == 1 ? &me : NULL; }

int main(void) {
  W.in_room = true;
  W.me = 1;

  ui_active = true;
  show_walk_pad(1, 0, 0, 1000);
  assert(ui_moves == 1 && room_sends == 0);

  ui_active = false;
  show_walk_pad(0, 0.31f, 0, 2000);  // below the old cardinal threshold, above the radial deadzone
  assert(ui_moves == 1 && room_sends == 1 && room_x == 2 && room_y == 2);

  ui_active = true;
  show_walk_pad(0, 0, WALK_PAD_RIGHT, 3000);
  assert(ui_moves == 2 && room_sends == 1);

  puts("PASS: a menu-consumed pad frame never reaches the room walk.");
  return 0;
}
