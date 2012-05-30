#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <math.h>

#ifndef KRAD_TONE_H
#define KRAD_TONE_H

#define MAX_TONES 32
#define KRAD_TONE_DEFAULT_VOLUME 65

typedef struct {
	float frequency;
	float delta;
	float angle;
	int active;
	int duration;
} tone_t;

typedef struct {

	int s;
	tone_t tones[MAX_TONES];
	int active_tones;
	float sample_rate;
	int volume;
	float volume_actual;
	
} krad_tone_t;

krad_tone_t *krad_tone_create (float sample_rate);
void krad_tone_set_sample_rate (krad_tone_t *krad_tone, float sample_rate);
void krad_tone_add (krad_tone_t *krad_tone, float frequency);
void krad_tone_add_preset (krad_tone_t *krad_tone, char *preset);
void krad_tone_clear (krad_tone_t *krad_tone);
void krad_tone_volume (krad_tone_t *krad_tone, int volume);
void krad_tone_remove (krad_tone_t *krad_tone, float frequency);
void krad_tone_run (krad_tone_t *krad_tone, float *buffer, int samples);
void krad_tone_destroy (krad_tone_t *krad_tone);

#endif
