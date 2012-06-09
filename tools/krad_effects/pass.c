
#include "pass.h"

#undef USE_BIQUAD

pass_t *pass_create(pass_t *pass) {

	pass = calloc(1, sizeof(pass_t));
	
	pass->rate = 44100.0f;

	pass->low_freq = 20000.0f;
	pass->high_freq = 20.0f;
	
#ifdef USE_BIQUAD
	pass->high_filter = calloc(1, sizeof(biquad));
	pass->low_filter = calloc(1, sizeof(biquad));
	biquad_init(pass->high_filter);
	biquad_init(pass->low_filter);

	hp_set_params(pass->high_filter, pass->high_freq, BANDWIDTH, pass->rate);
	lp_set_params(pass->low_filter, pass->low_freq, BANDWIDTH, pass->rate);
#else
	pass->stages = 10.0f;
	
	pass->high_gt = init_iir_stage(IIR_STAGE_HIGHPASS,10,3,2);
	pass->high_iirf = init_iirf_t(pass->high_gt);
	chebyshev(pass->high_iirf, pass->high_gt, 2*CLAMP(f_round((pass->stages)),1,10), IIR_STAGE_HIGHPASS, pass->high_freq/pass->rate, 0.5f);

	pass->low_gt = init_iir_stage(IIR_STAGE_LOWPASS,10,3,2);
	pass->low_iirf = init_iirf_t(pass->low_gt);
	chebyshev(pass->low_iirf, pass->low_gt, 2*CLAMP(f_round((pass->stages)),1,10), IIR_STAGE_LOWPASS, pass->low_freq/pass->rate, 0.5f);
#endif

	return pass;

}

void pass_run(pass_t *pass, float *input, float *output, int sample_count) {

#ifdef USE_BIQUAD
	hp_set_params(pass->high_filter, pass->high_freq, BANDWIDTH, pass->rate);
	lp_set_params(pass->low_filter, pass->low_freq, BANDWIDTH, pass->rate);
	
	for (pass->pos = 0; pass->pos < sample_count; pass->pos++) {
		pass->samp = biquad_run(pass->high_filter, input[pass->pos]);
		pass->samp = biquad_run(pass->low_filter, pass->samp);
		output[pass->pos] = pass->samp;
	}

#else	

	chebyshev(pass->high_iirf, pass->high_gt, 2*CLAMP((int)pass->stages,1,10), IIR_STAGE_HIGHPASS, pass->high_freq/pass->rate, 0.5f);
	iir_process_buffer_ns_5(pass->high_iirf, pass->high_gt, input, output, sample_count, 0);

	chebyshev(pass->low_iirf, pass->low_gt, 2*CLAMP((int)pass->stages,1,10), IIR_STAGE_LOWPASS, pass->low_freq/pass->rate, 0.5f);
	iir_process_buffer_ns_5(pass->low_iirf, pass->low_gt, input, output, sample_count, 0);

#endif

}

void pass_destroy(pass_t *pass) {

#ifdef USE_BIQUAD
	free (pass->high_filter);
	free (pass->low_filter);
#else
	free_iirf_t(pass->high_iirf, pass->high_gt);
	free_iir_stage(pass->high_gt);
	
	free_iirf_t(pass->low_iirf, pass->low_gt);
	free_iir_stage(pass->low_gt);
#endif

	free (pass);

}
