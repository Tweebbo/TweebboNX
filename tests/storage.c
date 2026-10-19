#include "common.h"
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
void log_start(void);
void log_stop(void);
void store_start(void);
void store_stop(void);
void store_save(const char *, const char *);
bool store_load(const char *, char *, int);
bool store_log_available(void) { return true; }
int __real_rename(const char *, const char *);
int __wrap_rename(const char *old, const char *next) {
  if (access(next, F_OK) == 0) { errno = EEXIST; return -1; }
  return __real_rename(old, next);
}
int main(void) {
  mkdir("sdmc:", 0700);
  log_start();
  store_start();
  char long_line[1000];
  memset(long_line, 'x', sizeof long_line - 1);
  long_line[sizeof long_line - 1] = 0;
  for (int i = 0; i < 300; i++) dbg_log("%s", long_line);
  for (int i = 0; i < 100; i++) {
    char value[32], actual[32];
    snprintf(value, sizeof value, "setting-%d", i);
    store_save("server", value);
    assert(store_load("server", actual, sizeof actual));
    assert(!strcmp(value, actual));
  }
  store_stop();
  log_stop();
  char actual[32];
  assert(store_load("server", actual, sizeof actual));
  assert(!strcmp(actual, "setting-99"));
  assert(rename("sdmc:/switch/Tweebbo/server.txt", "sdmc:/switch/Tweebbo/server.txt.bak") == 0);
  assert(store_load("server", actual, sizeof actual));
  assert(!strcmp(actual, "setting-99"));
  FILE *f = fopen("sdmc:/switch/Tweebbo/log.txt", "r");
  assert(f);
  int lines = 0, ch;
  while ((ch = fgetc(f)) != EOF) {
    assert(ch == 'x' || ch == '\n');
    if (ch == '\n') lines++;
  }
  fclose(f);
  assert(lines == 300);
  puts("OK: long log lines, ring drainage, pending settings reads and orderly shutdown.");
}
