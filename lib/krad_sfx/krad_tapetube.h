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

#define KRAD_TAPETUBE_CONTROL_DRIVE 671
#define KRAD_TAPETUBE_CONTROL_BLEND 672

#define KRAD_TAPETUBE_DRIVE_MIN 0.1f
#define KRAD_TAPETUBE_DRIVE_MAX 10.0f
#define KRAD_TAPETUBE_BLEND_MIN -10.0f
#define KRAD_TAPETUBE_BLEND_MAX 10.0f

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

} kr_tapetube_t;


kr_tapetube_t *kr_tapetube_create (int sample_rate);
void kr_tapetube_destroy (kr_tapetube_t *kr_tapetube);

void kr_tapetube_set_sample_rate (kr_tapetube_t *kr_tapetube, int sample_rate);
void kr_tapetube_process (kr_tapetube_t *kr_tapetube, float *input, float *output, int num_samples);

/* Controls */
void kr_tapetube_set_drive (kr_tapetube_t *kr_tapetube, float drive);
void kr_tapetube_set_blend (kr_tapetube_t *kr_tapetube, float blend);


