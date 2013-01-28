#include "krad_eq.h"

/* OpControls */
void kr_eq_band_add (kr_eq_t *kr_eq, float hz) {

  int b;

  for (b = 0; b < MAX_BANDS; b++) {
    if (kr_eq->band[b].active == 0) {
      biquad_init(&kr_eq->band[b].filter);
      kr_eq->band[b].hz = hz;
      kr_eq->band[b].db = 0.0f;
      kr_eq->band[b].bandwidth = 1.0f;
      eq_set_params(&kr_eq->band[b].filter,
                    kr_eq->band[b].hz,
                    kr_eq->band[b].db,
                    kr_eq->band[b].bandwidth,
                    kr_eq->sample_rate);

      kr_eq->band[b].active = 1;
      break;
    }
  }
}


void kr_eq_band_remove (kr_eq_t *kr_eq, int band_num) {
  if (kr_eq->band[band_num].active == 1) {
    kr_eq->band[band_num].active = 2;
  }
}

/* Controls */
void kr_eq_band_set_db (kr_eq_t *kr_eq, int band_num, float db) {
  if (kr_eq->band[band_num].active == 1) {
    db = LIMIT(db, KRAD_EQ_DB_MIN, KRAD_EQ_DB_MAX);
    krad_easing_set_new_value(&kr_eq->band[band_num].krad_easing_db, db, 200, EASEINOUTSINE);
  }
}

void kr_eq_band_set_bandwidth (kr_eq_t *kr_eq, int band_num, float bandwidth) {
  if (kr_eq->band[band_num].active == 1) {
    bandwidth = LIMIT(bandwidth, KRAD_EQ_BANDWIDTH_MIN, KRAD_EQ_BANDWIDTH_MAX);
    krad_easing_set_new_value(&kr_eq->band[band_num].krad_easing_bandwidth, bandwidth, 200, EASEINOUTSINE);
  }
}

void kr_eq_band_set_hz (kr_eq_t *kr_eq, int band_num, float hz) {
  if (kr_eq->band[band_num].active == 1) {
    hz = LIMIT(hz, KRAD_EQ_HZ_MIN, KRAD_EQ_HZ_MAX);
    krad_easing_set_new_value(&kr_eq->band[band_num].krad_easing_hz, hz, 600, EASEINOUTSINE);
  }
}

void kr_eq_process (kr_eq_t *kr_eq, float *input, float *output, int num_samples) {

  int b, s;
  int recompute;
  int recompute_default;

  if (kr_eq->new_sample_rate != kr_eq->sample_rate) {
    kr_eq->sample_rate = kr_eq->new_sample_rate;
    recompute_default = 1;
  } else {
    recompute_default = 0;
  }

  for (b = 0; b < MAX_BANDS; b++) {
    recompute = recompute_default;
    if (kr_eq->band[b].active == 2) {
      kr_eq->band[b].active = 0;
    }
    if (kr_eq->band[b].active != 1) {
      continue;
    }
    if (kr_eq->band[b].krad_easing_hz.active) {
      kr_eq->band[b].hz = krad_easing_process (&kr_eq->band[b].krad_easing_hz, kr_eq->band[b].hz); 
      recompute = 1;
    }
    if (kr_eq->band[b].krad_easing_db.active) {
      kr_eq->band[b].db = krad_easing_process (&kr_eq->band[b].krad_easing_db, kr_eq->band[b].db); 
      recompute = 1;
    }
    if (kr_eq->band[b].krad_easing_bandwidth.active) {
      kr_eq->band[b].bandwidth = krad_easing_process (&kr_eq->band[b].krad_easing_bandwidth, kr_eq->band[b].bandwidth); 
      recompute = 1;
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

  kr_eq_t *kr_eq = calloc (1, sizeof(kr_eq_t));

  kr_eq->new_sample_rate = sample_rate;
  kr_eq->sample_rate = kr_eq->new_sample_rate;

  return kr_eq;

}

void kr_eq_destroy(kr_eq_t *kr_eq) {
  free (kr_eq);
}
