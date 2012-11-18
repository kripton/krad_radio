#include "krad_timer.h"

static inline uint64_t ts_to_ms (struct timespec ts) {
  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

krad_timer_t *krad_timer_create() {
  krad_timer_t *krad_timer = calloc(1, sizeof(krad_timer_t));
  return krad_timer;
}

void krad_timer_start (krad_timer_t *krad_timer) {
  clock_gettime ( CLOCK_MONOTONIC, &krad_timer->start);
}

void krad_timer_finish (krad_timer_t *krad_timer) {
  clock_gettime ( CLOCK_MONOTONIC, &krad_timer->finish);
}

uint64_t krad_timer_sample_duration_ms (krad_timer_t *krad_timer) {
  clock_gettime ( CLOCK_MONOTONIC, &krad_timer->sample );
  return ts_to_ms (krad_timer->sample) - ts_to_ms (krad_timer->start);
}

uint64_t krad_timer_duration_ms (krad_timer_t *krad_timer) {
  return ts_to_ms (krad_timer->finish) - ts_to_ms (krad_timer->start);
}

void krad_timer_destroy(krad_timer_t *krad_timer) {
  free (krad_timer);
}
