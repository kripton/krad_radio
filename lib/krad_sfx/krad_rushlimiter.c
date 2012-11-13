#include "krad_rushlimiter.h"

kr_rushlimiter_t *kr_rushlimiter_create (int sample_rate) {

  kr_rushlimiter_t *kr_rushlimiter = calloc (1, sizeof(kr_rushlimiter_t));

  kr_rushlimiter->gain = 1.0f;

  return kr_rushlimiter;

}

void kr_rushlimiter_destroy(kr_rushlimiter_t *kr_rushlimiter) {
  free (kr_rushlimiter);
}

static void kr_rushlimiter_limit (kr_rushlimiter_t *kr_rushlimiter, float *input, float *output, int num_samples) {

  int s;
	float peak;
  float sample;
  float gain;
  float bottom;

  bottom = 1.0f;
  peak = 0.0f;
  sample = 0.0f;

	for (s = 0; s < num_samples; s++) {
		sample = fabs(input[s]);
		if (sample > peak) {
			peak = sample;
		}
	}

  gain = 1.0f / peak;
  bottom = 1.0f - gain;

  if (gain < kr_rushlimiter->gain) {
    kr_rushlimiter->gain = gain;
  }

  if (peak < 1.0f) {
    kr_rushlimiter->hold = kr_rushlimiter->hold - num_samples;
    if (kr_rushlimiter->hold < 72000) {
      kr_rushlimiter->gain += (1.0f - kr_rushlimiter->gain) / (72000 / num_samples);
    }
    if ((kr_rushlimiter->hold <= 0) || (kr_rushlimiter->gain >= 1.0f)) {
      kr_rushlimiter->hold = 0;
      kr_rushlimiter->gain = 1.0f;
    }
  }

  for (s = 0; s < num_samples; s++) {
    if ((input[s] > bottom) || (input[s] < (bottom * -1.0f))) {
      output[s] = input[s] * kr_rushlimiter->gain;
    } else {
      output[s] = input[s];
    }
  }
  
}

void kr_rushlimiter_process (kr_rushlimiter_t *kr_rushlimiter, float *input, float *output, int num_samples) {

  int s;
  int limit;

  limit = 0;

  for (s = 0; s < num_samples; s++ ) {
	  if (input[s] < -1.0f) {
      limit = 1;
      break;
	  } else if (input[s] > 1.0f) {
      limit = 1;
      break;
	  }
  }

  if (limit == 1) {
    kr_rushlimiter->hold = 96000;
  }

  if ((limit == 1) || (kr_rushlimiter->hold > 0)) {
    kr_rushlimiter_limit (kr_rushlimiter, input, output, num_samples);
  }

}
