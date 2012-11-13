#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include "biquad.h"

#include "krad_system.h"


#define KRAD_PASS_CONTROL_TYPE 788
#define KRAD_PASS_CONTROL_BANDWIDTH 789
#define KRAD_PASS_CONTROL_HZ 780

typedef struct {

  biquad filter;

  int type;
  float bandwidth;
  float hz;

  int new_type;
  float new_bandwidth;
  float new_hz;

  float new_sample_rate;
  float sample_rate;
      
} kr_pass_t;


kr_pass_t *kr_pass_create (int sample_rate);
void kr_pass_destroy (kr_pass_t *kr_pass);

void kr_pass_set_sample_rate (kr_pass_t *kr_pass, int sample_rate);
void kr_pass_process (kr_pass_t *kr_pass, float *input, float *output, int num_samples);

/* Controls */
void kr_pass_set_type (kr_pass_t *kr_pass, int type);
void kr_pass_set_bandwidth (kr_pass_t *kr_pass, float bandwidth);
void kr_pass_set_hz (kr_pass_t *kr_pass, float hz);
