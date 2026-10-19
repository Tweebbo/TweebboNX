#define NO_TLS
#include <assert.h>
#include <stdatomic.h>
#include <time.h>
static atomic_bool cancelled;
#include "../source/nx/conn.c"
bool net_cancelled(void) { return atomic_load(&cancelled); }
u64 now_ms(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (u64)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
static void *cancel_soon(void *arg) {
  (void)arg;
  usleep(50000);
  atomic_store(&cancelled, true);
  return NULL;
}
int main(void) {
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  assert(listener >= 0);
  struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
  assert(bind(listener, (struct sockaddr *)&addr, sizeof addr) == 0);
  assert(listen(listener, 1) == 0);
  socklen_t len = sizeof addr;
  assert(getsockname(listener, (struct sockaddr *)&addr, &len) == 0);
  Endpoint ep = {.port = ntohs(addr.sin_port)};
  strcpy(ep.host, "127.0.0.1");
  char err[128];
  Conn *first = conn_open(&ep, 500, err, sizeof err);
  assert(first);
  Conn *second = conn_open(&ep, 500, err, sizeof err);
  assert(second);
  /* With the accept queue full, Linux drops the next SYN: exercise a real timeout. */
  u64 before = now_ms();
  assert(!conn_open(&ep, 120, err, sizeof err));
  assert(now_ms() - before >= 100 && now_ms() - before < 1000);
  pthread_t thread;
  assert(pthread_create(&thread, NULL, cancel_soon, NULL) == 0);
  before = now_ms();
  assert(!conn_open(&ep, 5000, err, sizeof err));
  assert(now_ms() - before < 500);
  pthread_join(thread, NULL);
  assert(conn_read(first, err, sizeof err, 5000) == -1);
  assert(!conn_write(first, "x", 1, 5000));
  conn_close(first);
  conn_close(second);
  close(listener);
  puts("OK: TCP connection deadline and cancellation during connect/read/write.");
}
