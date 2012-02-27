#include "fastlimiter.h"


fastlimiter_t *fastlimiter_create() {

	fastlimiter_t *fastlimiter = calloc(1, sizeof(fastlimiter_t));
	
	fastlimiter->rate = 44100.0f;

	fastlimiter->buffer_len = 128;
	fastlimiter->buffer_pos = 0;

	/* Find size for power-of-two interleaved delay buffer */
	while ( fastlimiter->buffer_len < fastlimiter->rate * BUFFER_TIME * 2 ) {
		fastlimiter->buffer_len *= 2;
	}

	fastlimiter->buffer = calloc(fastlimiter->buffer_len, sizeof(float));
	fastlimiter->delay = (int)(0.005 * fastlimiter->rate);

	fastlimiter->chunk_pos = 0;
	fastlimiter->chunk_num = 0;

	/* find a chunk size (in samples) thats roughly 0.5ms */
	fastlimiter->chunk_size = fastlimiter->rate / 2000;
	fastlimiter->chunks = calloc(NUM_CHUNKS, sizeof(float));

	memset(fastlimiter->buffer, 0, NUM_CHUNKS * sizeof(float));

	fastlimiter->chunk_pos = 0;
	fastlimiter->chunk_num = 0;
	fastlimiter->peak = 0.0f;
	fastlimiter->atten = 1.0f;
	fastlimiter->atten_lp = 1.0f;
	fastlimiter->delta = 0.0f;
	
	fastlimiter->ingain = 0.0f;
	fastlimiter->limit = 0.0f;
	//FIXME not broke but NOTE this !!
	fastlimiter->limit = -0.4f;
	fastlimiter->release = 0.25f;
	fastlimiter->attenuation = 0.0f;
	
	fastlimiter->new_limit = fastlimiter->limit;
	fastlimiter->new_release = fastlimiter->release;	
	
	return fastlimiter;

}


void fastlimiter_run(fastlimiter_t *fastlimiter, float *left_input, float *right_input, float *left_output, float *right_output, int sample_count) {

	fastlimiter->limit = fastlimiter->new_limit;
	//FIXME not broke but NOTE this !!
	if (fastlimiter->limit > -0.4f) {
		fastlimiter->limit = -0.4f;
	}
	
	fastlimiter->release = fastlimiter->new_release;

	const float max = DB_CO(fastlimiter->limit);
	const float trim = DB_CO(fastlimiter->ingain);
	float sig;
	unsigned int i;

	for (fastlimiter->pos = 0; fastlimiter->pos < sample_count; fastlimiter->pos++) {
	
		if (fastlimiter->chunk_pos++ == fastlimiter->chunk_size) {
			/* we've got a full chunk */
         
			fastlimiter->delta = (1.0f - fastlimiter->atten) / (fastlimiter->rate * fastlimiter->release);
			round_to_zero(&fastlimiter->delta);

			for (i = 0; i < 10; i++) {

				const int p = (fastlimiter->chunk_num - 9 + i) & (NUM_CHUNKS - 1);
				const float this_delta = (max / fastlimiter->chunks[p] - fastlimiter->atten) / ((float)(i+1) * fastlimiter->rate * 0.0005f + 1.0f);

				if (this_delta < fastlimiter->delta) {
					fastlimiter->delta = this_delta;
				}
			}

			fastlimiter->chunks[fastlimiter->chunk_num++ & (NUM_CHUNKS - 1)] = fastlimiter->peak;
			fastlimiter->peak = 0.0f;
			fastlimiter->chunk_pos = 0;
		
		}

		fastlimiter->buffer[(fastlimiter->buffer_pos * 2) & (fastlimiter->buffer_len - 1)] = left_input[fastlimiter->pos] * trim + 1.0e-30;
		fastlimiter->buffer[(fastlimiter->buffer_pos * 2 + 1) & (fastlimiter->buffer_len - 1)] = right_input[fastlimiter->pos] * trim + 1.0e-30;

		sig = fabs(left_input[fastlimiter->pos]) > fabs(right_input[fastlimiter->pos]) ? fabs(left_input[fastlimiter->pos]) : fabs(right_input[fastlimiter->pos]);
		sig += 1.0e-30;
		if (sig * trim > fastlimiter->peak) {
			fastlimiter->peak = sig * trim;
		}
	
		fastlimiter->atten += fastlimiter->delta;
		fastlimiter->atten_lp = fastlimiter->atten * 0.1f + fastlimiter->atten_lp * 0.9f;

		if (fastlimiter->delta > 0.0f && fastlimiter->atten > 1.0f) {
			fastlimiter->atten = 1.0f;
			fastlimiter->delta = 0.0f;
		}

		left_output[fastlimiter->pos] = fastlimiter->buffer[(fastlimiter->buffer_pos * 2 - fastlimiter->delay * 2) & (fastlimiter->buffer_len - 1)] * fastlimiter->atten_lp;
		right_output[fastlimiter->pos] = fastlimiter->buffer[(fastlimiter->buffer_pos * 2 - fastlimiter->delay * 2 + 1) & (fastlimiter->buffer_len - 1)] * fastlimiter->atten_lp;
	
		round_to_zero(&left_output[fastlimiter->pos]);
		round_to_zero(&right_output[fastlimiter->pos]);

		if (left_output[fastlimiter->pos] < -max) {
			left_output[fastlimiter->pos] = -max;
		} else if (left_output[fastlimiter->pos] > max) {
			left_output[fastlimiter->pos] = max;
		}

		if (right_output[fastlimiter->pos] < -max) {
			right_output[fastlimiter->pos] = -max;
		} else if (right_output[fastlimiter->pos] > max) {
			right_output[fastlimiter->pos] = max;
		}

		fastlimiter->buffer_pos++;

	}

	//plugin_data->buffer_pos = buffer_pos;
	//plugin_data->peak = peak;
	//plugin_data->atten = atten;
	//plugin_data->atten_lp = atten_lp;
	//plugin_data->chunk_pos = chunk_pos;
	//plugin_data->chunk_num = chunk_num;

	//*(plugin_data->attenuation) = -CO_DB(atten);
	//*(plugin_data->latency) = delay;
 
}




void fastlimiter_destroy(fastlimiter_t *fastlimiter)  {

	free ( fastlimiter->chunks );
	free ( fastlimiter->buffer );
	free ( fastlimiter );

}
      
      
      
      
      
      
