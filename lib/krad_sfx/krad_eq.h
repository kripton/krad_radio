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

#include "biquad.h"

#define KRAD_EQ_CONTROL_DB 666
#define KRAD_EQ_CONTROL_BANDWIDTH 667
#define KRAD_EQ_CONTROL_HZ 668

#define KRAD_EQ_BANDWIDTH_MIN 0.1
#define KRAD_EQ_BANDWIDTH_MAX 5.0
#define KRAD_EQ_DB_MIN -50.0
#define KRAD_EQ_DB_MAX 20.0
#define KRAD_EQ_HZ_MIN 20.0
#define KRAD_EQ_HZ_MAX 20000.0

typedef struct {

  biquad filter;

  float db;
  float bandwidth;
  float hz;
  
  krad_easing_t krad_easing_db;
  krad_easing_t krad_easing_bandwidth;
  krad_easing_t krad_easing_hz;
  
//  int active;

} kr_eq_band_t;

typedef struct {

  float new_sample_rate;
  float sample_rate;
  kr_eq_band_t band[KRAD_EQ_MAX_BANDS];
      
} kr_eq_t;


kr_eq_t *kr_eq_create (int sample_rate);
void kr_eq_destroy (kr_eq_t *kr_eq);

void kr_eq_set_sample_rate (kr_eq_t *kr_eq, int sample_rate);
void kr_eq_process (kr_eq_t *kr_eq, float *input, float *output, int num_samples);

void kr_eq_band_set_db (kr_eq_t *kr_eq, int band_num, float db, int duration, krad_ease_t ease);
void kr_eq_band_set_bandwidth (kr_eq_t *kr_eq, int band_num, float bandwidth, int duration, krad_ease_t ease);
void kr_eq_band_set_hz (kr_eq_t *kr_eq, int band_num, float hz, int duration, krad_ease_t ease);
