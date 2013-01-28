#include "krad_tapetube.h"

#define EPS 0.000000001f

static inline float M(float x) {
	if ((x > EPS) || (x < -EPS))
		return x;
	else
		return 0.0f;
}

static inline float D(float x) {
	if (x > EPS)
		return sqrt(x);
	else if (x < -EPS)
		return sqrt(-x);
	else
		return 0.0f;
}

void kr_tapetube_set_blend (kr_tapetube_t *kr_tapetube, float blend) {
  krad_easing_set_new_value(&kr_tapetube->krad_easing_blend, blend, 600, EASEINOUTSINE);
}

void kr_tapetube_set_drive (kr_tapetube_t *kr_tapetube, float drive) {
  krad_easing_set_new_value(&kr_tapetube->krad_easing_drive, drive, 600, EASEINOUTSINE);
}

void kr_tapetube_set_sample_rate (kr_tapetube_t *kr_tapetube, int sample_rate) {
  kr_tapetube->sample_rate = sample_rate;
}

kr_tapetube_t *kr_tapetube_create (int sample_rate) {

  kr_tapetube_t *kr_tapetube = calloc (1, sizeof(kr_tapetube_t));

  kr_tapetube->sample_rate = sample_rate;

  kr_tapetube->prev_drive = -1.0f;
  kr_tapetube->prev_blend = -11.0f;

  return kr_tapetube;
}

void kr_tapetube_destroy(kr_tapetube_t *kr_tapetube) {
  free (kr_tapetube);
}

void kr_tapetube_process (kr_tapetube_t *kr_tapetube, float *input, float *output, int num_samples) {

  int s;
  
	float drive;
	float blend;

	unsigned long sample_rate;

	float rdrive = kr_tapetube->rdrive;
	float rbdr = kr_tapetube->rbdr;
	float kpa = kr_tapetube->kpa;
	float kpb = kr_tapetube->kpb;
	float kna = kr_tapetube->kna;
	float knb = kr_tapetube->knb;
	float ap = kr_tapetube->ap;
	float an = kr_tapetube->an;
	float imr = kr_tapetube->imr;
	float kc = kr_tapetube->kc;
	float srct = kr_tapetube->srct;
	float sq = kr_tapetube->sq;
	float pwrq = kr_tapetube->pwrq;

	float prev_med;
	float prev_out;
	float in;
	float med;
	float out;
	
  if (kr_tapetube->krad_easing_blend.active) {
    kr_tapetube->blend = krad_easing_process (&kr_tapetube->krad_easing_blend, kr_tapetube->blend); 
  }
  if (kr_tapetube->krad_easing_drive.active) {
    kr_tapetube->drive = krad_easing_process (&kr_tapetube->krad_easing_drive, kr_tapetube->drive); 
  }

	drive = LIMIT(kr_tapetube->drive, KRAD_TAPETUBE_DRIVE_MIN, KRAD_TAPETUBE_DRIVE_MAX);
	blend = LIMIT(kr_tapetube->blend, KRAD_TAPETUBE_BLEND_MIN, KRAD_TAPETUBE_BLEND_MAX);

  sample_rate = kr_tapetube->sample_rate;

	if ((kr_tapetube->prev_drive != drive) || (kr_tapetube->prev_blend != blend)) {

		rdrive = 12.0f / drive;
		rbdr = rdrive / (10.5f - blend) * 780.0f / 33.0f;
		kpa = D(2.0f * (rdrive*rdrive) - 1.0f) + 1.0f;
		kpb = (2.0f - kpa) / 2.0f;
		ap = ((rdrive*rdrive) - kpa + 1.0f) / 2.0f;
		kc = kpa / D(2.0f * D(2.0f * (rdrive*rdrive) - 1.0f) - 2.0f * rdrive*rdrive);

		srct = (0.1f * sample_rate) / (0.1f * sample_rate + 1.0f);
		sq = kc*kc + 1.0f;
		knb = -1.0f * rbdr / D(sq);
		kna = 2.0f * kc * rbdr / D(sq);
		an = rbdr*rbdr / sq;
		imr = 2.0f * knb + D(2.0f * kna + 4.0f * an - 1.0f);
		pwrq = 2.0f / (imr + 1.0f);

		kr_tapetube->prev_drive = drive;
		kr_tapetube->prev_blend = blend;
	}

	for (s = 0; s < num_samples; s++) {

		in = *(input++);
		prev_med = kr_tapetube->prev_med;
		prev_out = kr_tapetube->prev_out;

		if (in >= 0.0f) {
			med = (D(ap + in * (kpa - in)) + kpb) * pwrq;
		} else {
			med = (D(an - in * (kna + in)) + knb) * pwrq * -1.0f;
		}

		out = srct * (med - prev_med + prev_out);

		if (out < -1.0f)
			out = -1.0f;
		
		*(output++) = out;

		kr_tapetube->prev_med = M(med);
		kr_tapetube->prev_out = M(out);
	}

	kr_tapetube->rdrive = rdrive;
	kr_tapetube->rbdr = rbdr;
	kr_tapetube->kpa = kpa;
	kr_tapetube->kpb = kpb;
	kr_tapetube->kna = kna;
	kr_tapetube->knb = knb;
	kr_tapetube->ap = ap;
	kr_tapetube->an = an;
	kr_tapetube->imr = imr;
	kr_tapetube->kc = kc;
	kr_tapetube->srct = srct;
	kr_tapetube->sq = sq;
	kr_tapetube->pwrq = pwrq;
}

