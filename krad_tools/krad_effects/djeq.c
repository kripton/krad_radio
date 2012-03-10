#include "djeq.h"

#define BANDS 3
#define LOW_BW 1.0f
#define MID_BW 3.0f
#define HIGH_BW 3.0f
#define CHANNELS 2

djeq_t *djeq_create() {

	djeq_t *djeq = calloc(1, sizeof(djeq_t));
	
	djeq->rate = 44100.0f;

	djeq->filters = calloc(BANDS * CHANNELS, sizeof(biquad));
      
	for (djeq->i = 0; djeq->i < CHANNELS; djeq->i++) {
		biquad_init(&djeq->filters[djeq->i*BANDS + 0]);
		eq_set_params(&djeq->filters[djeq->i*BANDS + 0], 100.0f, 0.0f, LOW_BW, djeq->rate);
		biquad_init(&djeq->filters[djeq->i*BANDS + 1]);
		eq_set_params(&djeq->filters[djeq->i*BANDS + 1], 1000.0f, 0.0f, MID_BW, djeq->rate);
		biquad_init(&djeq->filters[djeq->i*BANDS + 2]);
		eq_set_params(&djeq->filters[djeq->i*BANDS + 2], 10000.0f, 0.0f, HIGH_BW, djeq->rate);
	}
	
	return djeq;

}

void djeq_run(djeq_t *djeq, float *left_input, float *right_input, float *left_output, float *right_output, int sample_count) {

	for (djeq->i = 0; djeq->i < CHANNELS; djeq->i++) {
		eq_set_params(&djeq->filters[djeq->i*BANDS + 0], 100.0f, djeq->low, LOW_BW, djeq->rate);
		eq_set_params(&djeq->filters[djeq->i*BANDS + 1], 1000.0f, djeq->mid, MID_BW, djeq->rate);
		eq_set_params(&djeq->filters[djeq->i*BANDS + 2], 10000.0f, djeq->high, HIGH_BW, djeq->rate);
	}

	for (djeq->pos = 0; djeq->pos < sample_count; djeq->pos++) {
		djeq->samp = biquad_run(&djeq->filters[0], left_input[djeq->pos]);
		djeq->samp = biquad_run(&djeq->filters[1], djeq->samp);
		djeq->samp = biquad_run(&djeq->filters[2], djeq->samp);
		left_output[djeq->pos] = djeq->samp;

		djeq->samp = biquad_run(&djeq->filters[3], right_input[djeq->pos]);
		djeq->samp = biquad_run(&djeq->filters[4], djeq->samp);
		djeq->samp = biquad_run(&djeq->filters[5], djeq->samp);
		right_output[djeq->pos] = djeq->samp;
	}

      //*(plugin_data->latency) = 3; //XXX is this right?

}

void djeq_destroy(djeq_t *djeq) {

	free (djeq->filters);
	free (djeq);

}
