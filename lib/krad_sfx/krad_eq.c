#include "krad_eq.h"

void kr_eq_band_set_db (kr_eq_t *kr_eq, int band_num, float db, int duration, krad_ease_t ease) {
  db = LIMIT(db, KRAD_EQ_DB_MIN, KRAD_EQ_DB_MAX);
  krad_easing_set_new_value(&kr_eq->band[band_num].krad_easing_db, db, duration, ease, NULL);
}

void kr_eq_band_set_bandwidth (kr_eq_t *kr_eq, int band_num, float bandwidth, int duration, krad_ease_t ease) {
  bandwidth = LIMIT(bandwidth, KRAD_EQ_BANDWIDTH_MIN, KRAD_EQ_BANDWIDTH_MAX);
  krad_easing_set_new_value(&kr_eq->band[band_num].krad_easing_bandwidth, bandwidth, duration, ease, NULL);
}

void kr_eq_band_set_hz (kr_eq_t *kr_eq, int band_num, float hz, int duration, krad_ease_t ease) {
  hz = LIMIT(hz, KRAD_EQ_HZ_MIN, KRAD_EQ_HZ_MAX);
  krad_easing_set_new_value(&kr_eq->band[band_num].krad_easing_hz, hz, duration, ease, NULL);
}

//void kr_eq_process (kr_eq_t *kr_eq, float *input, float *output, int num_samples) {

void kr_eq_process2 (kr_eq_t *kr_eq, float *input, float *output, int num_samples, int broadcast) {

  int b, s;
  int recompute;
  int recompute_default;
  void *ptr;

	ptr = NULL;

  if (kr_eq->new_sample_rate != kr_eq->sample_rate) {
    kr_eq->sample_rate = kr_eq->new_sample_rate;
    recompute_default = 1;
  } else {
    recompute_default = 0;
  }

  for (b = 0; b < KRAD_EQ_MAX_BANDS; b++) {
    //if ((kr_eq->band[b].db == 0.0f) && (!kr_eq->band[b].krad_easing_db.active)) {
    //  continue;
    //}
    recompute = recompute_default;
    if (kr_eq->band[b].krad_easing_hz.active) {
      kr_eq->band[b].hz = krad_easing_process (&kr_eq->band[b].krad_easing_hz, kr_eq->band[b].hz, &ptr); 
      recompute = 1;
      if (broadcast == 1) {
        krad_mixer_broadcast_portgroup_effect_control (kr_eq->krad_mixer, kr_eq->portgroupname, 0, b, HZ, kr_eq->band[b].hz);      
      }
    }
    if (kr_eq->band[b].krad_easing_db.active) {
      kr_eq->band[b].db = krad_easing_process (&kr_eq->band[b].krad_easing_db, kr_eq->band[b].db, &ptr); 
      recompute = 1;
      if (broadcast == 1) {
        krad_mixer_broadcast_portgroup_effect_control (kr_eq->krad_mixer, kr_eq->portgroupname, 0, b, DB, kr_eq->band[b].db);      
      }
    }
    if (kr_eq->band[b].krad_easing_bandwidth.active) {
      kr_eq->band[b].bandwidth = krad_easing_process (&kr_eq->band[b].krad_easing_bandwidth, kr_eq->band[b].bandwidth, &ptr); 
      recompute = 1;
      if (broadcast == 1) {
        krad_mixer_broadcast_portgroup_effect_control (kr_eq->krad_mixer, kr_eq->portgroupname, 0, b, BANDWIDTH, kr_eq->band[b].bandwidth);      
      }
    }

    if (recompute == 1) {
        eq_set_params(&kr_eq->band[b].filter,
                      kr_eq->band[b].hz,
                      kr_eq->band[b].db,
                      kr_eq->band[b].bandwidth,
                      kr_eq->sample_rate);
    }
    if (kr_eq->band[b].db != 0.0f) {
      for (s = 0; s < num_samples; s++) {
        output[s] = biquad_run(&kr_eq->band[b].filter, input[s]);
      }
    }
  }
}

void kr_eq_set_sample_rate (kr_eq_t *kr_eq, int sample_rate) {
  kr_eq->new_sample_rate = sample_rate;
}

kr_eq_t *kr_eq_create (int sample_rate) {
  
  int b;
  float hz;
  kr_eq_t *kr_eq = calloc (1, sizeof(kr_eq_t));

  kr_eq->new_sample_rate = sample_rate;
  kr_eq->sample_rate = kr_eq->new_sample_rate;

  hz = 30.0;
  for (b = 0; b < KRAD_EQ_MAX_BANDS; b++) {
    kr_eq->band[b].db = 0.0f,
    kr_eq->band[b].bandwidth = 1.0f;
    kr_eq->band[b].hz = floor(hz);
    if (hz < 1000.0f) {
      if (hz < 150.0f) {
        hz = hz + 15.0f;
      } else {
        if (hz < 600.0f) {
          hz = hz + 50.0f;
        } else {
          hz = hz + 200.0f;
        }
      }
    } else {
      hz = hz + 1500.0f;
    }
  }

  return kr_eq;
}

kr_eq_t *kr_eq_create2 (int sample_rate, krad_mixer_t *krad_mixer, char *portgroupname) {
  
  int b;
  float hz;
  kr_eq_t *kr_eq = calloc (1, sizeof(kr_eq_t));

  kr_eq->new_sample_rate = sample_rate;
  kr_eq->sample_rate = kr_eq->new_sample_rate;

  kr_eq->krad_mixer = krad_mixer;
  strncpy (kr_eq->portgroupname, portgroupname, sizeof(kr_eq->portgroupname));

  hz = 30.0;
  for (b = 0; b < KRAD_EQ_MAX_BANDS; b++) {
    kr_eq->band[b].db = 0.0f,
    kr_eq->band[b].bandwidth = 1.0f;
    kr_eq->band[b].hz = floor(hz);
    if (hz < 1000.0f) {
      if (hz < 150.0f) {
        hz = hz + 15.0f;
      } else {
        if (hz < 600.0f) {
          hz = hz + 50.0f;
        } else {
          hz = hz + 200.0f;
        }
      }
    } else {
      hz = hz + 1500.0f;
    }
  }

  return kr_eq;
}

void kr_eq_destroy (kr_eq_t *kr_eq) {
  free (kr_eq);
}
