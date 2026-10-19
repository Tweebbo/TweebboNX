#include "common.h"
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef DATA_DIR
#define DATA_DIR "sdmc:/switch/Tweebbo"
#endif

typedef struct Save {
  struct Save *next;
  char path[160];
  char value[];
} Save;

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static Save *head, *tail;
static unsigned queued;
static bool started, stopping;

static void store_path(const char *key, char *path, size_t cap) {
  char safe[96];
  int n = 0;
  for (const char *k = key; *k && n < (int)sizeof safe - 1; k++)
    safe[n++] = (*k >= 'a' && *k <= 'z') || (*k >= 'A' && *k <= 'Z') ||
                (*k >= '0' && *k <= '9') || *k == '-' || *k == '.' ? *k : '_';
  safe[n] = 0;
  snprintf(path, cap, DATA_DIR "/%s.txt", safe);
}

static bool replace_file(const char *temp, const char *path) {
  if (rename(temp, path) == 0) return true;
  if (errno != EEXIST) return false;
  /* Switch fsdev does not implement POSIX replacement of an existing destination. */
  char backup[168];
  snprintf(backup, sizeof backup, "%s.bak", path);
  if (remove(backup) != 0 && errno != ENOENT) return false;
  if (rename(path, backup) != 0) return false;
  if (rename(temp, path) != 0) {
    rename(backup, path);
    return false;
  }
  remove(backup);
  return true;
}

static void *save_thread(void *arg) {
  (void)arg;
  mkdir("sdmc:/switch", 0777);
  mkdir(DATA_DIR, 0777);
  for (;;) {
    pthread_mutex_lock(&mu);
    while (!head && !stopping) pthread_cond_wait(&wake, &mu);
    Save *job = head;
    pthread_mutex_unlock(&mu);
    if (!job) break;
    char temp[168];
    snprintf(temp, sizeof temp, "%s.tmp", job->path);
    FILE *f = fopen(temp, "w");
    bool ok = false;
    if (f) {
      ok = fputs(job->value, f) >= 0;
      if (fclose(f)) ok = false;
      if (ok) ok = replace_file(temp, job->path);
    }
    if (!ok) dbg_log("Could not save settings to the SD card.");
    pthread_mutex_lock(&mu);
    head = job->next;
    if (!head) tail = NULL;
    queued--;
    pthread_mutex_unlock(&mu);
    free(job);
  }
  return NULL;
}

void store_start(void) {
  if (started) return;
  stopping = false;
  started = pthread_create(&worker, NULL, save_thread, NULL) == 0;
  if (!started) dbg_log("Could not start the settings worker.");
}

void store_stop(void) {
  if (!started) return;
  pthread_mutex_lock(&mu);
  stopping = true;
  pthread_cond_signal(&wake);
  pthread_mutex_unlock(&mu);
  pthread_join(worker, NULL);
  started = false;
}

bool store_load(const char *key, char *out, int cap) {
  if (cap <= 0) return false;
  char path[160];
  store_path(key, path, sizeof path);
  /* A server change is read back before the SD worker necessarily finishes. */
  pthread_mutex_lock(&mu);
  const Save *latest = NULL;
  for (Save *job = head; job; job = job->next)
    if (!strcmp(path, job->path)) latest = job;
  bool found = latest != NULL;
  if (found) snprintf(out, (size_t)cap, "%s", latest->value);
  pthread_mutex_unlock(&mu);
  if (found) return true;
  FILE *f = fopen(path, "r");
  if (!f && errno == ENOENT) {
    char backup[168];
    snprintf(backup, sizeof backup, "%s.bak", path);
    f = fopen(backup, "r");
  }
  if (!f) return false;
  size_t n = fread(out, 1, (size_t)cap - 1, f);
  fclose(f);
  out[n] = 0;
  out[strcspn(out, "\r\n")] = 0;
  return true;
}

void store_save(const char *key, const char *value) {
  Save *job = malloc(sizeof *job + strlen(value) + 1);
  if (!job) return;
  job->next = NULL;
  store_path(key, job->path, sizeof job->path);
  strcpy(job->value, value);
  pthread_mutex_lock(&mu);
  if (!started || stopping || queued >= 256) {
    pthread_mutex_unlock(&mu);
    free(job);
    dbg_log("Settings queue unavailable.");
    return;
  }
  if (tail) tail->next = job; else head = job;
  tail = job;
  queued++;
  pthread_cond_signal(&wake);
  pthread_mutex_unlock(&mu);
}
