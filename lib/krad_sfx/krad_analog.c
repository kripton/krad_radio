#include "krad_analog.h"

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

void kr_analog_set_blend (kr_analog_t *kr_analog, float blend, int duration, krad_ease_t ease) {
	blend = LIMIT(blend, KRAD_ANALOG_BLEND_MIN, KRAD_ANALOG_BLEND_MAX);
  krad_easing_set_new_value (&kr_analog->krad_easing_blend, blend, duration, ease);
}

void kr_analog_set_drive (kr_analog_t *kr_analog, float drive, int duration, krad_ease_t ease) {
	drive = LIMIT(drive, KRAD_ANALOG_DRIVE_MIN_OFF, KRAD_ANALOG_DRIVE_MAX);
  krad_easing_set_new_value (&kr_analog->krad_easing_drive, drive, duration, ease);
}

void kr_analog_set_sample_rate (kr_analog_t *kr_analog, int sample_rate) {
  kr_analog->sample_rate = sample_rate;
}

kr_analog_t *kr_analog_create (int sample_rate) {

  kr_analog_t *kr_analog = calloc (1, sizeof(kr_analog_t));

  kr_analog->sample_rate = sample_rate;

  kr_analog->prev_drive = -1.0f;
  kr_analog->prev_blend = -11.0f;

  return kr_analog;
}

kr_analog_t *kr_analog_create2 (int sample_rate, krad_mixer_t *krad_mixer, char *portgroupname) {

  kr_analog_t *kr_analog = calloc (1, sizeof(kr_analog_t));

  kr_analog->krad_mixer = krad_mixer;
  strncpy (kr_analog->portgroupname, portgroupname, sizeof(kr_analog->portgroupname));

  kr_analog->sample_rate = sample_rate;

  kr_analog->prev_drive = -1.0f;
  kr_analog->prev_blend = -11.0f;

  return kr_analog;
}

void kr_analog_destroy(kr_analog_t *kr_analog) {
  free (kr_analog);
}

//void kr_analog_process (kr_analog_t *kr_analog, float *input, float *output, int num_samples) {
void kr_analog_process2 (kr_analog_t *kr_analog, float *input, float *output, int num_samples, int broadcast) {
  int s;
  
	float drive;
	float blend;

	unsigned long sample_rate;

	float rdrive = kr_analog->rdrive;
	float rbdr = kr_analog->rbdr;
	float kpa = kr_analog->kpa;
	float kpb = kr_analog->kpb;
	float kna = kr_analog->kna;
	float knb = kr_analog->knb;
	float ap = kr_analog->ap;
	float an = kr_analog->an;
	float imr = kr_analog->imr;
	float kc = kr_analog->kc;
	float srct = kr_analog->srct;
	float sq = kr_analog->sq;
	float pwrq = kr_analog->pwrq;

	float prev_med;
	float prev_out;
	float in;
	float med;
	float out;
	
  if (kr_analog->krad_easing_blend.active) {
    kr_analog->blend = krad_easing_process (&kr_analog->krad_easing_blend, kr_analog->blend);
    
    if (broadcast == 1) {
      krad_mixer_broadcast_portgroup_effect_control (kr_analog->krad_mixer,
                                                     kr_analog->portgroupname,
                                                     3, 0, BLEND, kr_analog->blend);      
    }
    
  }
  if (kr_analog->krad_easing_drive.active) {
    kr_analog->drive = krad_easing_process (&kr_analog->krad_easing_drive, kr_analog->drive);
    
    if (broadcast == 1) {
      krad_mixer_broadcast_portgroup_effect_control (kr_analog->krad_mixer,
                                                     kr_analog->portgroupname,
                                                     3, 0, DRIVE, kr_analog->drive);      
    }
    
  }

  if (kr_analog->drive < KRAD_ANALOG_DRIVE_MIN) {
    return; 
  }

	drive = LIMIT(kr_analog->drive, KRAD_ANALOG_DRIVE_MIN, KRAD_ANALOG_DRIVE_MAX);
	blend = LIMIT(kr_analog->blend, KRAD_ANALOG_BLEND_MIN, KRAD_ANALOG_BLEND_MAX);

  sample_rate = kr_analog->sample_rate;

	if ((kr_analog->prev_drive != drive) || (kr_analog->prev_blend != blend)) {

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

		kr_analog->prev_drive = drive;
		kr_analog->prev_blend = blend;
	}

	for (s = 0; s < num_samples; s++) {

		in = *(input++);
		prev_med = kr_analog->prev_med;
		prev_out = kr_analog->prev_out;

		if (in >= 0.0f) {
			med = (D(ap + in * (kpa - in)) + kpb) * pwrq;
		} else {
			med = (D(an - in * (kna + in)) + knb) * pwrq * -1.0f;
		}

		out = srct * (med - prev_med + prev_out);

		if (out < -1.0f)
			out = -1.0f;
		
		*(output++) = out;

		kr_analog->prev_med = M(med);
		kr_analog->prev_out = M(out);
	}

	kr_analog->rdrive = rdrive;
	kr_analog->rbdr = rbdr;
	kr_analog->kpa = kpa;
	kr_analog->kpb = kpb;
	kr_analog->kna = kna;
	kr_analog->knb = knb;
	kr_analog->ap = ap;
	kr_analog->an = an;
	kr_analog->imr = imr;
	kr_analog->kc = kc;
	kr_analog->srct = srct;
	kr_analog->sq = sq;
	kr_analog->pwrq = pwrq;
}

