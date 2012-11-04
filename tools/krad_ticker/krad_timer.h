#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

#include "krad_system.h"

#ifndef KRAD_TIMER_H
#define KRAD_TIMER_H

typedef struct krad_timer_St krad_timer_t;

struct krad_timer_St {

	struct timespec start;
	struct timespec finish;
	struct timespec duration;
	const char *note;
	uint64_t duration_ms;

};

krad_timer_t *krad_timer_create();
void krad_timer_start (krad_timer_t *krad_timer);
void krad_timer_finish (krad_timer_t *krad_timer);
int krad_timer_duration_ms (krad_timer_t *krad_timer);
void krad_timer_destroy (krad_timer_t *krad_timer);

#endif
