#include <malloc.h>
#include <math.h>
//#include "util/db.h"
//#include "util/rms.h"
#include "util/biquad.h"
#include "util/iir.h"

#define BANDWIDTH 1.5f



typedef struct {

    int pos;
	float rate;
	biquad *high_filter;
	biquad *low_filter;
	float samp;
	float high_freq;
	float low_freq;
	//int low;
	//int high;

	//iir
	int stages;
	
	iirf_t* high_iirf;
	iir_stage_t* high_gt;
	
	iirf_t* low_iirf;
	iir_stage_t* low_gt;
      
} pass_t;


pass_t *pass_create(pass_t *pass);
void pass_run(pass_t *pass, float *input, float *output, int sample_count);
void pass_destroy(pass_t *pass);
