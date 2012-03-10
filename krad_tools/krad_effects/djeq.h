#include <malloc.h>
#include <math.h>
#include "util/db.h"
#include "util/rms.h"
#include "util/biquad.h"


typedef struct {

    int i;
	float rate;
	biquad *filters;
	unsigned long pos;
	float samp;
	
	float low;
	float mid;
	float high;
      
} djeq_t;


djeq_t *djeq_create();
void djeq_run(djeq_t *djeq, float *left_input, float *right_input, float *left_output, float *right_output, int sample_count);
void djeq_destroy(djeq_t *djeq);
