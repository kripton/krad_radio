#include "krad_eq.h"



kr_eq_t *kr_eq_create (int sample_rate) {

  int i;

  kr_eq_t *kr_eq = calloc (1, sizeof(kr_eq_t));
	
  kr_eq->sample_rate = sample_rate;

/*  
	for (i = 0; i < CHANNELS; i++) {
		biquad_init(&kr_eq->filters[i*BANDS + 0]);
		eq_set_params(&kr_eq->filters[i*BANDS + 0], 100.0f, 0.0f, LOW_BW, kr_eq->sample_rate);
		biquad_init(&kr_eq->filters[i*BANDS + 1]);
		eq_set_params(&kr_eq->filters[i*BANDS + 1], 1000.0f, 0.0f, MID_BW, kr_eq->sample_rate);
		biquad_init(&kr_eq->filters[i*BANDS + 2]);
		eq_set_params(&kr_eq->filters[i*BANDS + 2], 10000.0f, 0.0f, HIGH_BW, kr_eq->sample_rate);
	}
*/	
	return kr_eq;

}

void kr_eq_process (kr_eq_t *kr_eq, float *input, float *output, int num_samples) {

  int i;
  float sample;
/*
	for (i = 0; i < CHANNELS; i++) {
		eq_set_params(&kr_eq->filters[i*BANDS + 0], 100.0f, kr_eq->low, LOW_BW, kr_eq->sample_rate);
		eq_set_params(&kr_eq->filters[i*BANDS + 1], 1000.0f, kr_eq->mid, MID_BW, kr_eq->sample_rate);
		eq_set_params(&kr_eq->filters[i*BANDS + 2], 10000.0f, kr_eq->high, HIGH_BW, kr_eq->sample_rate);
	}

	for (kr_eq->pos = 0; kr_eq->pos < sample_count; kr_eq->pos++) {
		sample = biquad_run(&kr_eq->filters[0], left_input[kr_eq->pos]);
		sample = biquad_run(&kr_eq->filters[1], sample);
		sample = biquad_run(&kr_eq->filters[2], sample);
		left_output[kr_eq->pos] = sample;

		sample = biquad_run(&kr_eq->filters[3], right_input[kr_eq->pos]);
		sample = biquad_run(&kr_eq->filters[4], sample);
		sample = biquad_run(&kr_eq->filters[5], sample);
		right_output[kr_eq->pos] = sample;
	}
*/
}

void kr_eq_destroy(kr_eq_t *kr_eq) {

  free (kr_eq);

}
