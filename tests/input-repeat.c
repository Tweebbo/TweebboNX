#include "input_repeat.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  InputRepeat r = {0};
  assert(input_repeat(&r, 1, 0, 1000, 180));
  assert(!input_repeat(&r, 1, 0, 1016, 180));
  assert(input_repeat(&r, -1, 0, 1032, 180));
  assert(!input_repeat(&r, 0, 0, 1048, 180));
  assert(input_repeat(&r, -1, 0, 1064, 180));
  assert(!input_repeat(&r, -1, 0, 1243, 180));
  assert(input_repeat(&r, -1, 0, 1244, 180));
  puts("Immediate presses, direction changes, release/repress and held repeats passed.");
}
