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
#include "krad_easing.h"

#include "krad_sfx_common.h"

#include "krad_mixer.h"

#ifndef KRAD_PASS_H
#define KRAD_PASS_H


#define KRAD_PASS_CONTROL_TYPE 788
#define KRAD_PASS_CONTROL_BANDWIDTH 789
#define KRAD_PASS_CONTROL_HZ 780

#define KRAD_PASS_BANDWIDTH_MIN 0.1
#define KRAD_PASS_BANDWIDTH_MAX 5.0
#define KRAD_PASS_HZ_MIN 20.0
#define KRAD_PASS_HZ_MAX 20000.0
#define KRAD_LOWPASS_HZ_MAX 12000.0


typedef struct krad_pass_St krad_pass_t;
typedef struct krad_pass_St kr_pass_t;
typedef struct krad_pass_St kr_highpass_t;
typedef struct krad_pass_St kr_lowpass_t;



struct krad_pass_St {

  biquad filter;

  int type;
  float bandwidth;
  float hz;

  int new_type;

  float new_sample_rate;
  float sample_rate;
  
  krad_easing_t krad_easing_bandwidth;
  krad_easing_t krad_easing_hz;
  
  krad_mixer_t *krad_mixer;
  kr_address_t address;
};

kr_pass_t *kr_pass_create2 (int sample_rate, kr_effect_type_t type, krad_mixer_t *krad_mixer, char *portgroupname);
kr_pass_t *kr_pass_create (int sample_rate, kr_effect_type_t type);
void kr_pass_destroy (kr_pass_t *kr_pass);

void kr_pass_set_sample_rate (kr_pass_t *kr_pass, int sample_rate);
//void kr_pass_process (kr_pass_t *kr_pass, float *input, float *output, int num_samples);
void kr_pass_process2 (kr_pass_t *kr_pass, float *input, float *output, int num_samples, int broadcast);
/* Controls */
void kr_pass_set_type (kr_pass_t *kr_pass, kr_effect_type_t type);
void kr_pass_set_bandwidth (kr_pass_t *kr_pass, float bandwidth, int duration, krad_ease_t ease);
void kr_pass_set_hz (kr_pass_t *kr_pass, float hz, int duration, krad_ease_t ease);

#endif
