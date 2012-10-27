#include "krad_pass.h"

/* Controls */
void kr_pass_set_type (kr_pass_t *kr_pass, int type) {
  kr_pass->new_type = type;
}

void kr_pass_set_bandwidth (kr_pass_t *kr_pass, float bandwidth) {
  kr_pass->new_bandwidth = bandwidth;
}

void kr_pass_set_hz (kr_pass_t *kr_pass, float hz) {
  kr_pass->new_hz = hz;
}

void kr_pass_process (kr_pass_t *kr_pass, float *input, float *output, int num_samples) {

  int s;
  int recompute;

  recompute = 0;

  if (kr_pass->new_sample_rate != kr_pass->sample_rate) {
    kr_pass->sample_rate = kr_pass->new_sample_rate;
    recompute = 1;
  }

  if (kr_pass->new_type != kr_pass->type) {
    kr_pass->type = kr_pass->new_type;
    recompute = 1;
  }

  if (kr_pass->new_bandwidth != kr_pass->bandwidth) {
    kr_pass->bandwidth = kr_pass->new_bandwidth;
    recompute = 1;
  }

  if (kr_pass->new_hz != kr_pass->hz) {
    kr_pass->hz = kr_pass->new_hz;
    recompute = 1;
  }

  if (recompute == 1) {
    if (kr_pass->type == 0) {
          lp_set_params(&kr_pass->filter,
                        kr_pass->hz,
                        kr_pass->bandwidth,
                        kr_pass->sample_rate);
    } else {
          hp_set_params(&kr_pass->filter,
                        kr_pass->hz,
                        kr_pass->bandwidth,
                        kr_pass->sample_rate);
    }
  }

  for (s = 0; s < num_samples; s++) {
    output[s] = biquad_run(&kr_pass->filter, input[s]);
  }

}

void kr_pass_set_sample_rate (kr_pass_t *kr_pass, int sample_rate) {
  kr_pass->new_sample_rate = sample_rate;
}

kr_pass_t *kr_pass_create (int sample_rate) {

  kr_pass_t *kr_pass = calloc (1, sizeof(kr_pass_t));

  kr_pass->new_sample_rate = sample_rate;
  kr_pass->sample_rate = kr_pass->new_sample_rate;

  return kr_pass;

}

void kr_pass_destroy(kr_pass_t *kr_pass) {
  free (kr_pass);
}
