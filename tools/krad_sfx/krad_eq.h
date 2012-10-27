#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include "biquad.h"

#define MAX_BANDS 32

typedef struct {

  biquad filter;

  float db;
  float bandwidth;
  float hz;

  float new_db;
  float new_bandwidth;
  float new_hz;

  int active;

} kr_eq_band_t;

typedef struct {

  float new_sample_rate;
  float sample_rate;
  kr_eq_band_t band[MAX_BANDS];
      
} kr_eq_t;


kr_eq_t *kr_eq_create (int sample_rate);
void kr_eq_destroy (kr_eq_t *kr_eq);

void kr_eq_set_sample_rate (kr_eq_t *kr_eq, int sample_rate);
void kr_eq_process (kr_eq_t *kr_eq, float *input, float *output, int num_samples);

/* OpControls */
void kr_eq_band_add (kr_eq_t *kr_eq, float hz);
void kr_eq_band_remove (kr_eq_t *kr_eq, int band_num);

/* Controls */
void kr_eq_band_set_db (kr_eq_t *kr_eq, int band_num, float db);
void kr_eq_band_set_bandwidth (kr_eq_t *kr_eq, int band_num, float bandwidth);
void kr_eq_band_set_hz (kr_eq_t *kr_eq, int band_num, float hz);
