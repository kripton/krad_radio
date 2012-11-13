#include "krad_timer.h"

static inline uint64_t ts_to_nsec (struct timespec ts) {
	return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
}

static inline uint64_t ts_to_ms (struct timespec ts) {
	return ((ts.tv_sec * 1000000000LL) + ts.tv_nsec) / 1000000;
}

static inline struct timespec nsec_to_ts (uint64_t nsecs) {
	struct timespec ts;
	ts.tv_sec = nsecs / (1000000000LL);
	ts.tv_nsec = nsecs % (1000000000LL);
	return ts;
}

static inline struct timespec add_ts (struct timespec ts, uint64_t add_nsecs) {
	uint64_t nsecs = ts_to_nsec(ts);
	nsecs += add_nsecs;
	return nsec_to_ts (nsecs);
}

static struct timespec timespec_diff (struct timespec start, struct timespec end) {

	struct timespec temp;

	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

krad_timer_t *krad_timer_create() {

	krad_timer_t *krad_timer = calloc(1, sizeof(krad_timer_t));

	return krad_timer;

}

void krad_timer_start(krad_timer_t *krad_timer) {

	krad_timer->duration_ms = 0;
	clock_gettime ( CLOCK_THREAD_CPUTIME_ID, &krad_timer->start);

}

void krad_timer_finish(krad_timer_t *krad_timer) {

	clock_gettime ( CLOCK_THREAD_CPUTIME_ID, &krad_timer->finish);

}

int krad_timer_duration_ms (krad_timer_t *krad_timer) {
	krad_timer->duration = timespec_diff (krad_timer->start, krad_timer->finish);
	krad_timer->duration_ms = ts_to_ms (krad_timer->duration);
	return krad_timer->duration_ms;
}

void krad_timer_destroy(krad_timer_t *krad_timer) {

	free (krad_timer);

}
