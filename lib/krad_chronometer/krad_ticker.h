#include <time.h>

#include "krad_system.h"

typedef struct krad_ticker_St krad_ticker_t;

struct krad_ticker_St {

  int numerator;
  int denominator;

  struct timespec start_time;
  struct timespec wakeup_time;

  uint64_t period_time_nanosecs;
  uint64_t total_periods;

};


inline struct timespec timespec_add_ms (struct timespec ts, uint64_t ms);
inline struct timespec timespec_add_ns (struct timespec ts, uint64_t ns);

void krad_ticker_destroy (krad_ticker_t *krad_ticker);
krad_ticker_t *krad_ticker_create (int numerator, int denominator);

void krad_ticker_start_at (krad_ticker_t *krad_ticker, struct timespec start_time);
void krad_ticker_start (krad_ticker_t *krad_ticker);
void krad_ticker_wait (krad_ticker_t *krad_ticker);

