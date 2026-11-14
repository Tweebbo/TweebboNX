// Host test for the shared source/walk_pad.h: same cases as the 3DS host test.
// bash tests/run.sh
#include "../source/walk_pad.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SIDE 16

typedef struct { bool blocked[SIDE][SIDE], goal_only[SIDE][SIDE]; } Grid;
typedef struct { int n, x[64], y[64]; } Sent;

typedef struct { Grid *grid; Sent *sent; } Ctx;
static Grid grid;
static Sent sent;
static WalkPad pad;
static Ctx ctx = {&grid, &sent};

static bool pass(void *c, int x, int y, bool goal) {
  Grid *g = ((Ctx *)c)->grid;
  if (x < 0 || y < 0 || x >= SIDE || y >= SIDE) return false;
  if (g->blocked[y][x]) return false;
  return !g->goal_only[y][x] || goal;
}

static void sent_cb(void *c, int x, int y) {
  Sent *s = ((Ctx *)c)->sent;
  assert(s->n < 64);
  s->x[s->n] = x;
  s->y[s->n] = y;
  s->n++;
}

static void reset(void) {
  memset(&grid, 0, sizeof grid);
  memset(&sent, 0, sizeof sent);
  memset(&pad, 0, sizeof pad);
}

static void tick(float ax, float ay, unsigned dpad, int x, int y, int nx, int ny, u64 now) {
  WalkPadPos pos = {x, y, nx, ny};
  walk_pad_step(&pad, ax, ay, dpad, &pos, now, pass, sent_cb, &ctx);
}

static void aim(float degrees, float *ax, float *ay) {
  float r = degrees * WALK_PAD_PI / 180.0f;
  *ax = sinf(r);
  *ay = cosf(r);
}

// The 3DS table this module replaces; keep both in the same order.
static const int expected[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}};

static void sectors_and_dpad(void) {
  for (int s = 0; s < 8; s++) {
    int dx, dy;
    walk_pad_dir(s, &dx, &dy);
    assert(dx == expected[s][0] && dy == expected[s][1]);
    reset();
    float ax, ay;
    aim(s * 45.0f, &ax, &ay);
    tick(ax, ay, 0, 8, 8, 255, 255, 1000);
    assert(sent.n == 1);
    assert(sent.x[0] == 8 + 2 * dx && sent.y[0] == 8 + 2 * dy);
  }
  struct { unsigned bits; int s; } combos[] = {
    {WALK_PAD_UP, 0}, {WALK_PAD_UP | WALK_PAD_RIGHT, 1}, {WALK_PAD_RIGHT, 2},
    {WALK_PAD_RIGHT | WALK_PAD_DOWN, 3}, {WALK_PAD_DOWN, 4}, {WALK_PAD_DOWN | WALK_PAD_LEFT, 5},
    {WALK_PAD_LEFT, 6}, {WALK_PAD_LEFT | WALK_PAD_UP, 7},
  };
  for (int i = 0; i < 8; i++) {
    reset();
    tick(0, 0, combos[i].bits, 8, 8, 255, 255, 1000);
    int dx, dy;
    walk_pad_dir(combos[i].s, &dx, &dy);
    assert(sent.n == 1);
    assert(sent.x[0] == 8 + 2 * dx && sent.y[0] == 8 + 2 * dy);
  }
  puts("sectors: stick and D-pad directions match the 3DS tile table.");
}

static void angular_hysteresis(void) {
  reset();
  float ax, ay;
  aim(0, &ax, &ay);
  tick(ax, ay, 0, 8, 8, 7, 7, 1000);
  assert(sent.n == 1 && pad.sector == 0);
  for (int i = 0; i < 12; i++) {
    aim(i & 1 ? 17.5f : 27.5f, &ax, &ay);
    tick(ax, ay, 0, 8, 8, 7, 7, 1016u + 16u * (u64)i);
  }
  assert(pad.sector == 0 && sent.n == 1);
  aim(60.0f, &ax, &ay);
  tick(ax, ay, 0, 8, 8, 7, 7, 1300);
  assert(pad.sector == 1 && sent.n == 2);
  assert(sent.x[1] == 8 && sent.y[1] == 6);
  puts("hysteresis: boundary wobble keeps one sector and one request; a sweep changes it.");
}

static void deadzone_hysteresis(void) {
  reset();
  tick(0, 0.25f, 0, 8, 8, 7, 7, 2000);
  assert(!pad.active && sent.n == 0);
  tick(0, 0.31f, 0, 8, 8, 7, 7, 2016);
  assert(pad.active && sent.n == 1);
  tick(0, 0.25f, 0, 8, 8, 7, 7, 2032);
  assert(pad.active && sent.n == 1);
  tick(0, 0.20f, 0, 8, 8, 7, 7, 2048);
  assert(!pad.active);
  tick(0, 0, 0, 8, 8, 7, 7, 2128);
  assert(sent.n == 1);
  tick(0, 0, 0, 8, 8, 7, 7, 2148);
  assert(sent.n == 2 && sent.x[1] == 7 && sent.y[1] == 7);
  tick(0, 0, 0, 8, 8, 7, 7, 2300);
  assert(sent.n == 2);
  puts("deadzone: 0.30 enters, 0.22 holds, release stops once after 90 ms.");
}

static void release_grace(void) {
  reset();
  float ax, ay;
  aim(0, &ax, &ay);
  tick(ax, ay, 0, 8, 8, 7, 7, 3000);
  assert(sent.n == 1);
  tick(0, 0, 0, 8, 8, 7, 7, 3016);
  tick(0, 0, 0, 8, 8, 7, 7, 3032);
  tick(0, 0, 0, 8, 8, 7, 7, 3048);
  aim(180, &ax, &ay);
  tick(ax, ay, 0, 8, 8, 7, 7, 3064);
  assert(sent.n == 1 && pad.active);
  tick(ax, ay, 0, 8, 8, 7, 7, 3100);
  assert(sent.n == 2 && sent.x[1] == 10 && sent.y[1] == 10);
  for (int i = 0; i < 2; i++) assert(!(sent.x[i] == 7 && sent.y[i] == 7));
  puts("grace: a direction change through the deadzone sends no stop.");
}

static void neutral_without_path(void) {
  reset();
  float ax, ay;
  aim(0, &ax, &ay);
  tick(ax, ay, 0, 8, 8, 255, 255, 4000);
  assert(sent.n == 1);
  tick(0, 0, 0, 8, 8, 255, 255, 4016);
  tick(0, 0, 0, 8, 8, 255, 255, 4216);
  assert(sent.n == 1);
  puts("grace: neutral with no pending path sends nothing.");
}

static void send_policy(void) {
  reset();
  tick(0, 1, 0, 8, 8, 255, 255, 5000);
  tick(0, 1, 0, 8, 8, 255, 255, 5016);
  assert(sent.n == 1);
  tick(0, 1, 0, 8, 8, 255, 255, 5100);
  assert(sent.n == 1);
  tick(0, 1, 0, 8, 8, 255, 255, 5200);
  assert(sent.n == 1);
  tick(0, 1, 0, 8, 8, 255, 255, 5300);
  assert(sent.n == 1);
  tick(0, 1, 0, 8, 8, 255, 255, 5350);
  assert(sent.n == 2);
  tick(0, 1, 0, 8, 8, 255, 255, 5450);
  assert(sent.n == 2);
  tick(0, 1, 0, 8, 8, 255, 255, 5700);
  assert(sent.n == 3);

  reset();
  for (u64 t = 6000; t <= 6500; t += 16) tick(0, 1, 0, 8, 8, 7, 7, t);
  assert(sent.n == 1);

  reset();
  tick(0, 1, 0, 8, 8, 7, 7, 7000);
  tick(1, 0, 0, 8, 8, 7, 7, 7050);
  assert(sent.n == 1);
  tick(1, 0, 0, 8, 8, 7, 7, 7100);
  assert(sent.n == 2 && sent.x[1] == 10 && sent.y[1] == 6);

  reset();
  tick(0, 1, 0, 8, 8, 255, 255, 7200);
  tick(1, 0, 0, 8, 8, 255, 255, 7250);
  assert(sent.n == 1);
  tick(1, 0, 0, 8, 8, 255, 255, 7300);
  assert(sent.n == 2 && sent.x[1] == 10 && sent.y[1] == 6);
  puts("send: no duplicate while the path runs; an unanswered target retries at 350 ms; a new target at 100 ms.");
}

static void corners(void) {
  reset();
  grid.blocked[8][7] = true;  // (7,8) blocks the up-left diagonal; (8,7) is open
  tick(0.3f, 0.95f, 0, 8, 8, 7, 7, 8000);
  assert(sent.n == 1 && sent.x[0] == 8 && sent.y[0] == 6);

  reset();
  grid.blocked[8][7] = true;
  grid.blocked[7][8] = true;
  tick(0.3f, 0.95f, 0, 8, 8, 7, 7, 8100);
  assert(sent.n == 0);

  reset();
  grid.blocked[7][6] = true;  // second diagonal step (6,7) is blocked
  tick(0, 1, 0, 8, 8, 7, 7, 8200);
  assert(sent.n == 1 && sent.x[0] == 7 && sent.y[0] == 7);
  puts("corners: a blocked diagonal slides by stick projection; a blocked corner is not cut.");
}

static void goal_tiles(void) {
  reset();
  grid.goal_only[6][8] = true;
  tick(0.7071f, 0.7071f, 0, 8, 8, 7, 7, 9000);
  assert(sent.n == 1 && sent.x[0] == 8 && sent.y[0] == 6);

  reset();
  grid.goal_only[7][8] = true;
  grid.blocked[6][8] = true;
  tick(0.7071f, 0.7071f, 0, 8, 8, 7, 7, 9100);
  assert(sent.n == 1 && sent.x[0] == 8 && sent.y[0] == 7);
  puts("goal: seats and teleporters are only targeted at the last look-ahead tile.");
}

int main(void) {
  sectors_and_dpad();
  angular_hysteresis();
  deadzone_hysteresis();
  release_grace();
  neutral_without_path();
  send_policy();
  corners();
  goal_tiles();
  puts("PASS: walk_pad sectors, hysteresis, corners, goal tiles and send policy.");
  return 0;
}
