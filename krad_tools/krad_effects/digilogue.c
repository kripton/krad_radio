#include "digilogue.h"

// port_range_hints[DRIVE].LowerBound = 0.1f;
// port_range_hints[DRIVE].UpperBound = 10.0f;
// port_range_hints[BLEND].LowerBound = -10.0f;
// port_range_hints[BLEND].UpperBound = 10.0f;

digilogue_t *digilogue_create(digilogue_t *digilogue) {
	
	digilogue = calloc(1, sizeof(digilogue_t));
	
	digilogue->sample_rate = 44100.0f;
	//digilogue->run_adding_gain = 1.0f;

	digilogue->prev_med = 0.0f;
	digilogue->prev_out = 0.0f;

	digilogue->rdrive = 0.0f;
	digilogue->rbdr = 0.0f;
	digilogue->kpa = 0.0f;
	digilogue->kpb = 0.0f;
	digilogue->kna = 0.0f;
	digilogue->knb = 0.0f;
	digilogue->ap = 0.0f;
	digilogue->an = 0.0f;
	digilogue->imr = 0.0f;
	digilogue->kc = 0.0f;
	digilogue->srct = 0.0f;
	digilogue->sq = 0.0f;
	digilogue->pwrq = 0.0f;

	/* These are out of band to force param recalc upon first run() */
	digilogue->prev_drive = -1.0f;
	digilogue->prev_blend = -11.0f;

	return digilogue;

}

static inline float
M(float x) {

	if ((x > EPS) || (x < -EPS))
		return x;
	else
		return 0.0f;
}

static inline float
D(float x) {

	if (x > EPS)
		return sqrt(x);
	else if (x < -EPS)
		return sqrt(-x);
	else
		return 0.0f;
}

void digilogue_run(digilogue_t *digilogue, float *input, float *output, int sample_count) {
  
	digilogue->drive = LIMIT((digilogue->drive),0.1f,10.0f);
	digilogue->blend = LIMIT((digilogue->blend),-10.0f,10.0f);

	/*
	LADSPA_Data rdrive = ptr->rdrive;
	LADSPA_Data rbdr = ptr->rbdr;
	LADSPA_Data kpa = ptr->kpa;
	LADSPA_Data kpb = ptr->kpb;
	LADSPA_Data kna = ptr->kna;
	LADSPA_Data knb = ptr->knb;
	LADSPA_Data ap = ptr->ap;
	LADSPA_Data an = ptr->an;
	LADSPA_Data imr = ptr->imr;
	LADSPA_Data kc = ptr->kc;
	LADSPA_Data srct = ptr->srct;
	LADSPA_Data sq = ptr->sq;
	LADSPA_Data pwrq = ptr->pwrq;

	LADSPA_Data prev_med;
	LADSPA_Data prev_out;
	LADSPA_Data in;
	LADSPA_Data med;
	LADSPA_Data out;
	*/
	if ((digilogue->prev_drive != digilogue->drive) || (digilogue->prev_blend != digilogue->blend)) {

		digilogue->rdrive = 12.0f / digilogue->drive;
		digilogue->rbdr = digilogue->rdrive / (10.5f - digilogue->blend) * 780.0f / 33.0f;
		digilogue->kpa = D(2.0f * (digilogue->rdrive*digilogue->rdrive) - 1.0f) + 1.0f;
		digilogue->kpb = (2.0f - digilogue->kpa) / 2.0f;
		digilogue->ap = ((digilogue->rdrive*digilogue->rdrive) - digilogue->kpa + 1.0f) / 2.0f;
		digilogue->kc = digilogue->kpa / D(2.0f * D(2.0f * (digilogue->rdrive*digilogue->rdrive) - 1.0f) - 2.0f * digilogue->rdrive*digilogue->rdrive);

		digilogue->srct = (0.1f * digilogue->sample_rate) / (0.1f * digilogue->sample_rate + 1.0f);
		digilogue->sq = digilogue->kc*digilogue->kc + 1.0f;
		digilogue->knb = -1.0f * digilogue->rbdr / D(digilogue->sq);
		digilogue->kna = 2.0f * digilogue->kc * digilogue->rbdr / D(digilogue->sq);
		digilogue->an = digilogue->rbdr*digilogue->rbdr / digilogue->sq;
		digilogue->imr = 2.0f * digilogue->knb + D(2.0f * digilogue->kna + 4.0f * digilogue->an - 1.0f);
		digilogue->pwrq = 2.0f / (digilogue->imr + 1.0f);

		digilogue->prev_drive = digilogue->drive;
		digilogue->prev_blend = digilogue->blend;
	}

	for (digilogue->sample_index = 0; digilogue->sample_index < sample_count; digilogue->sample_index++) {

		digilogue->in = *(input++);

		//prev_med = digilogue->prev_med;
		//prev_out = digilogue->prev_out;

		if (digilogue->in >= 0.0f) {
			digilogue->med = (D(digilogue->ap + digilogue->in * (digilogue->kpa - digilogue->in)) + digilogue->kpb) * digilogue->pwrq;
		} else {
			digilogue->med = (D(digilogue->an - digilogue->in * (digilogue->kna + digilogue->in)) + digilogue->knb) * digilogue->pwrq * -1.0f;
		}

		digilogue->out = digilogue->srct * (digilogue->med - digilogue->prev_med + digilogue->prev_out);

		if (digilogue->out < -1.0f)
			digilogue->out = -1.0f;
		
		*(output++) = digilogue->out;

		digilogue->prev_med = M(digilogue->med);
		digilogue->prev_out = M(digilogue->out);
	}

	/*
	ptr->rdrive = rdrive;
	ptr->rbdr = rbdr;
	ptr->kpa = kpa;
	ptr->kpb = kpb;
	ptr->kna = kna;
	ptr->knb = knb;
	ptr->ap = ap;
	ptr->an = an;
	ptr->imr = imr;
	ptr->kc = kc;
	ptr->srct = srct;
	ptr->sq = sq;
	ptr->pwrq = pwrq;
	*/

}



void digilogue_destroy(digilogue_t *digilogue) {

	free (digilogue);

}

