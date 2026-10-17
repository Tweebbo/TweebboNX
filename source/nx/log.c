// Asynchronous diagnostic log: a 256 KB ring in RAM drained to the SD card by a
// low-priority thread. Writing (and flushing) to the microSD from the render thread
// blocks the frame for up to ~1 s during the card's internal garbage collection
// (the lesson from NX-torrent-player), so dbg_log never touches the file directly.
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <switch.h>
#include "common.h"

#define LOG_RING 256 * 1024

static char ring[LOG_RING];
static volatile int head, tail;
static bool started;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static void *drain_thread(void *arg);

extern bool store_log_available(void);

void log_start(void) {
  if (started) return;
  started = true;
  pthread_t t;
  pthread_create(&t, NULL, drain_thread, NULL);
  pthread_detach(t);
}

void dbg_log(const char *fmt, ...) {
  char line[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(line, sizeof line - 2, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  line[n++] = '\n';
  pthread_mutex_lock(&mu);
  for (int i = 0; i < n; i++) {
    int next = (head + 1) % LOG_RING;
    if (next == tail) break; /* overrun: drop the line */
    ring[head] = line[i];
    head = next;
  }
  pthread_mutex_unlock(&mu);
}

static void *drain_thread(void *arg) {
  (void)arg;
  static char chunk[8192];
  bool opened = false;
  for (;;) {
    svcSleepThread(500 * 1000 * 1000ull); /* 500 ms */
    pthread_mutex_lock(&mu);
    int n = 0;
    while (n < (int)sizeof chunk && tail != head) {
      chunk[n++] = ring[tail];
      tail = (tail + 1) % LOG_RING;
    }
    pthread_mutex_unlock(&mu);
    if (!n) continue;
    if (!store_log_available()) continue;
    if (!opened) {
      mkdir("sdmc:/switch", 0777);
      mkdir("sdmc:/switch/Tweebbo", 0777);
    }
    /* A fresh file per boot keeps the size bounded. */
    FILE *f = fopen("sdmc:/switch/Tweebbo/log.txt", opened ? "a" : "w");
    if (!f) continue;
    fwrite(chunk, 1, (size_t)n, f);
    fclose(f);
    opened = true;
  }
  return NULL;
}
