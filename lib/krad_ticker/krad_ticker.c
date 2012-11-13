#include "krad_ticker.h"

static inline uint64_t ts_to_nsec (struct timespec ts) {
	return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
}

static inline struct timespec nsec_to_ts (uint64_t nsecs) {
	struct timespec ts;
	ts.tv_sec = nsecs / (1000000000LL);
	ts.tv_nsec = nsecs % (1000000000LL);
	return ts;
}

inline struct timespec timespec_add_ns (struct timespec ts, uint64_t ns) {
	uint64_t nsecs = ts_to_nsec(ts);
	nsecs += ns;
	return nsec_to_ts (nsecs);
}

inline struct timespec timespec_add_ms (struct timespec ts, uint64_t ms) {
	return timespec_add_ns (ts, ms * 1000000);
}

void krad_ticker_destroy (krad_ticker_t *krad_ticker) {
	free (krad_ticker);
}

krad_ticker_t *krad_ticker_create (int numerator, int denominator) {

	krad_ticker_t *krad_ticker;
	
	krad_ticker = calloc (1, sizeof (krad_ticker_t));
	
	if (krad_ticker == NULL) {
		failfast ("Krad Ticker: Out of memory");
	}
	
	krad_ticker->numerator = numerator;
	krad_ticker->denominator = denominator;

	krad_ticker->period_time_nanosecs = (1000000000 / krad_ticker->numerator) * krad_ticker->denominator;

	//printk ("num %d denom %d nano %llu", numerator, denominator, krad_ticker->period_time_nanosecs);

	return krad_ticker;

}

void krad_ticker_start (krad_ticker_t *krad_ticker) {
	krad_ticker->total_periods = 0;
	clock_gettime (CLOCK_MONOTONIC, &krad_ticker->start_time);
}

void krad_ticker_start_at (krad_ticker_t *krad_ticker, struct timespec start_time) {
	krad_ticker->total_periods = 0;
	memcpy (&krad_ticker->start_time, &start_time, sizeof(struct timespec));
	krad_ticker_wait (krad_ticker);
}


void krad_ticker_wait (krad_ticker_t *krad_ticker) {

	krad_ticker->wakeup_time = timespec_add_ns (krad_ticker->start_time,
									   krad_ticker->period_time_nanosecs * krad_ticker->total_periods);

	if (clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &krad_ticker->wakeup_time, NULL)) {
		failfast ("Krad Ticker: error while clock nanosleeping");
	}

  krad_ticker->total_periods++;

}
