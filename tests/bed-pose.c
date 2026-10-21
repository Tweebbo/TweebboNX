#include <assert.h>
#include <stdio.h>
#include "../source/show.c"

static Item bed;
static FurniDef definition;
Item *world_item_at(int x, int y) {(void)x; (void)y; return &bed;}
const FurniDef *def_get(u16 id) {(void)id; return &definition;}
float ent_height(const Ent *e, u64 now) {(void)now; return e->z;}

int main(void) {
  for (int dir = 0; dir <= 2; dir += 2) {
    for (int z = 0; z <= 64; z += 32) {
      definition.seat = 40;
      bed.z = z;
      Ent e = {.flags = FLAG_LAY, .dir = dir, .z = z};
      PoseInfo p = pose_of(&e, 1000, false);
      assert(p.pose == POSE_LAY && p.lift == z + 8);
      assert(p.dx == (dir == 0 ? -47 : 10));
      assert(p.dy == (dir == 0 ? 32 : 30));
      assert(p.depth == 2 && p.anchor_y == 62);
      assert(pose_of(&e, 1000, true).pose == POSE_WALK);
      e.flags |= FLAG_ROLL;
      assert(pose_of(&e, 1000, true).pose == POSE_LAY);
      e.flags = 0;
      assert(pose_of(&e, 1000, false).pose == POSE_STD);
    }
  }
  puts("Bed poses: classic alignment, stack height, rollers and exit passed");
}
