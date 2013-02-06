#include "krad_pass.h"

/* Controls */
void kr_pass_set_type (kr_pass_t *kr_pass, kr_effect_type_t type) {
  if ((type != KRAD_LOWPASS) && (type != KRAD_HIGHPASS)) {
    return;
  }
  kr_pass->new_type = type;
}

void kr_pass_set_bandwidth (kr_pass_t *kr_pass, float bandwidth, int duration, krad_ease_t ease) {
  bandwidth = LIMIT(bandwidth, KRAD_PASS_BANDWIDTH_MIN, KRAD_PASS_BANDWIDTH_MAX);
  krad_easing_set_new_value(&kr_pass->krad_easing_bandwidth, bandwidth, duration, ease, NULL);
}

void kr_pass_set_hz (kr_pass_t *kr_pass, float hz, int duration, krad_ease_t ease) {
  if (kr_pass->type == KRAD_LOWPASS) {
    hz = LIMIT(hz, KRAD_PASS_HZ_MIN, KRAD_LOWPASS_HZ_MAX);
  } else {
    hz = LIMIT(hz, KRAD_PASS_HZ_MIN, KRAD_PASS_HZ_MAX);
  }
  krad_easing_set_new_value(&kr_pass->krad_easing_hz, hz, duration, ease, NULL);
}

//void kr_pass_process (kr_pass_t *kr_pass, float *input, float *output, int num_samples) {
void kr_pass_process2 (kr_pass_t *kr_pass, float *input, float *output, int num_samples, int broadcast) {

  int s;
  int recompute;
  void *ptr;

	ptr = NULL;
  recompute = 0;

  if (kr_pass->new_sample_rate != kr_pass->sample_rate) {
    kr_pass->sample_rate = kr_pass->new_sample_rate;
    recompute = 1;
  }

  if (kr_pass->new_type != kr_pass->type) {
    kr_pass->type = kr_pass->new_type;
    recompute = 1;
  }

  if (kr_pass->krad_easing_bandwidth.active) {
    kr_pass->bandwidth = krad_easing_process (&kr_pass->krad_easing_bandwidth, kr_pass->bandwidth, &ptr); 
    recompute = 1;
    
    if (broadcast == 1) {
      krad_radio_broadcast_subunit_control (kr_pass->krad_mixer->broadcaster, &kr_pass->address, BANDWIDTH, kr_pass->bandwidth, NULL);
    }
  }

  if (kr_pass->krad_easing_hz.active) {
    kr_pass->hz = krad_easing_process (&kr_pass->krad_easing_hz, kr_pass->hz, &ptr);
    recompute = 1;
    
    if (broadcast == 1) {
      krad_radio_broadcast_subunit_control (kr_pass->krad_mixer->broadcaster, &kr_pass->address, HZ, kr_pass->hz, NULL);
    }
  }

  if (recompute == 1) {
    if (kr_pass->type == KRAD_LOWPASS) {
          lp_set_params(&kr_pass->filter,
                        kr_pass->hz,
                        kr_pass->bandwidth,
                        kr_pass->sample_rate);
    } else {
          hp_set_params(&kr_pass->filter,
                        kr_pass->hz,
                        kr_pass->bandwidth,
                        kr_pass->sample_rate);
    }
  }
  
  if (((kr_pass->type == KRAD_LOWPASS) && (kr_pass->hz == KRAD_LOWPASS_HZ_MAX)) ||
      ((kr_pass->type == KRAD_HIGHPASS) && (kr_pass->hz == KRAD_PASS_HZ_MIN))) {
    return;
  }

  for (s = 0; s < num_samples; s++) {
    output[s] = biquad_run(&kr_pass->filter, input[s]);
  }
}

void kr_pass_set_sample_rate (kr_pass_t *kr_pass, int sample_rate) {
  kr_pass->new_sample_rate = sample_rate;
}

kr_pass_t *kr_pass_create2 (int sample_rate, kr_effect_type_t type, krad_mixer_t *krad_mixer, char *portgroupname) {

  if ((type != KRAD_LOWPASS) && (type != KRAD_HIGHPASS)) {
    return NULL;
  }

  kr_pass_t *kr_pass = calloc (1, sizeof(kr_pass_t));

  kr_pass->krad_mixer = krad_mixer;

  kr_pass->address.path.unit = KR_MIXER;
  kr_pass->address.path.subunit.mixer_subunit = KR_EFFECT;
  strncpy (kr_pass->address.id.name, portgroupname, sizeof(kr_pass->address.id.name));

  if (kr_pass->type == KRAD_LOWPASS) {
    kr_pass->address.sub_id = 1;
  }
  if (kr_pass->type == KRAD_HIGHPASS) {
    kr_pass->address.sub_id = 2;
  }
  kr_pass->address.sub_id2 = 0;

  kr_pass->new_sample_rate = sample_rate;

  kr_pass->type = type;
  kr_pass->new_type = kr_pass->type;
  kr_pass->bandwidth = 1;
  
  if (kr_pass->type == KRAD_LOWPASS) {
    kr_pass->hz = KRAD_LOWPASS_HZ_MAX;
  }
  if (kr_pass->type == KRAD_HIGHPASS) {
    kr_pass->hz = KRAD_PASS_HZ_MIN;
  }

  return kr_pass;
}

kr_pass_t *kr_pass_create (int sample_rate, kr_effect_type_t type) {

  if ((type != KRAD_LOWPASS) && (type != KRAD_HIGHPASS)) {
    return NULL;
  }

  kr_pass_t *kr_pass = calloc (1, sizeof(kr_pass_t));

  kr_pass->new_sample_rate = sample_rate;

  kr_pass->type = type;
  kr_pass->new_type = kr_pass->type;
  kr_pass->bandwidth = 1;
  
  if (kr_pass->type == KRAD_LOWPASS) {
    kr_pass->hz = KRAD_LOWPASS_HZ_MAX;
  }
  if (kr_pass->type == KRAD_HIGHPASS) {
    kr_pass->hz = KRAD_PASS_HZ_MIN;
  }

  return kr_pass;
}

void kr_pass_destroy(kr_pass_t *kr_pass) {
  free (kr_pass);
}
