#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include "krad_system.h"
#include "krad_easing.h"

#include "krad_sfx_common.h"

#include "krad_mixer.h"

#ifndef KRAD_ANALOG_H
#define KRAD_ANALOG_H

#define KRAD_ANALOG_CONTROL_DRIVE 671
#define KRAD_ANALOG_CONTROL_BLEND 672

#define KRAD_ANALOG_DRIVE_MIN_OFF 0.0f
#define KRAD_ANALOG_DRIVE_MIN 0.1f
#define KRAD_ANALOG_DRIVE_MAX 10.0f
#define KRAD_ANALOG_BLEND_MIN -10.0f
#define KRAD_ANALOG_BLEND_MAX 10.0f

typedef struct {

  float sample_rate;

  float drive;
  float blend;

	float prev_drive;
	float prev_blend;
  
  krad_easing_t krad_easing_drive;
  krad_easing_t krad_easing_blend;

	float prev_med;
	float prev_out;

	float rdrive;
	float rbdr;
	float kpa;
	float kpb;
	float kna;
	float knb;
	float ap;
	float an;
	float imr;
	float kc;
	float srct;
	float sq;
	float pwrq;
	
  krad_mixer_t *krad_mixer;
  char portgroupname[64];
  kr_address_t address;

} kr_analog_t;

kr_analog_t *kr_analog_create2 (int sample_rate, krad_mixer_t *krad_mixer, char *portgroupname);
kr_analog_t *kr_analog_create (int sample_rate);
void kr_analog_destroy (kr_analog_t *kr_analog);

void kr_analog_set_sample_rate (kr_analog_t *kr_analog, int sample_rate);
//void kr_analog_process (kr_analog_t *kr_analog, float *input, float *output, int num_samples);
void kr_analog_process2 (kr_analog_t *kr_analog, float *input, float *output, int num_samples, int broadcast);

/* Controls */
void kr_analog_set_drive (kr_analog_t *kr_analog, float drive, int duration, krad_ease_t ease);
void kr_analog_set_blend (kr_analog_t *kr_analog, float blend, int duration, krad_ease_t ease);

#endif
