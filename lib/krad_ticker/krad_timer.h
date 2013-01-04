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
	struct timespec sample;	
	struct timespec finish;
	const char *name;
};

krad_timer_t *krad_timer_create ();
krad_timer_t *krad_timer_create_with_name (const char *name);
void krad_timer_status (krad_timer_t *krad_timer);
void krad_timer_start (krad_timer_t *krad_timer);
uint64_t krad_timer_sample_duration_ms (krad_timer_t *krad_timer);
void krad_timer_finish (krad_timer_t *krad_timer);
uint64_t krad_timer_duration_ms (krad_timer_t *krad_timer);
void krad_timer_destroy (krad_timer_t *krad_timer);

#endif
