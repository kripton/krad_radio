#include <malloc.h>
#include <math.h>
#include <string.h>
#include "util/db.h"
#include "util/rms.h"
#include "util/biquad.h"


#define NUM_CHUNKS 16
#define BUFFER_TIME 0.0053

typedef struct {

    int i;
	float rate;
	unsigned long pos;
	float samp;

	float *buffer;
	unsigned int buffer_len;
	unsigned int buffer_pos;

	float atten;
	float atten_lp;
	float peak;
	float delta;
	unsigned int delay;
	unsigned int chunk_num;
	unsigned int chunk_pos;
	unsigned int chunk_size;
	float *chunks;
      
	float ingain;
	float limit;
	float release;
	float attenuation;
   
	float new_limit;
	float new_release;   
      
} fastlimiter_t;


fastlimiter_t *fastlimiter_create();
void fastlimiter_run(fastlimiter_t *fastlimiter, float *left_input, float *right_input, float *left_output, float *right_output, int sample_count);
void fastlimiter_destroy(fastlimiter_t *fastlimiter);






