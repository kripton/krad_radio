#include "krad_sfx.h"

kr_effects_t *kr_effects_create (int channels, int sample_rate) {

  kr_effects_t *kr_effects = calloc (1, sizeof(kr_effects_t));

  kr_effects->effect = calloc (KRAD_EFFECTS_MAX, sizeof(kr_effect_t));

  kr_effects->channels = channels;
  kr_effects->sample_rate = sample_rate;

  return kr_effects;
}

void kr_effects_destroy (kr_effects_t *kr_effects) {

  int e;

  for (e = 0; e < KRAD_EFFECTS_MAX; e++) {
    if (kr_effects->effect[e].active == 1) {
      kr_effects_effect_remove (kr_effects, e);
    }
  }

  free (kr_effects->effect);
  free (kr_effects);
}

void kr_effects_set_sample_rate (kr_effects_t *kr_effects, int sample_rate) {

  int e, c;

  kr_effects->sample_rate = sample_rate;

  for (e = 0; e < KRAD_EFFECTS_MAX; e++) {
    if (kr_effects->effect[e].active == 1) {
      for (c = 0; c < kr_effects->channels; c++) {
        switch (kr_effects->effect[e].effect_type) {
          case KRAD_NOFX:
            break;
          case KRAD_EQ:
            kr_eq_set_sample_rate (kr_effects->effect[e].effect[c], kr_effects->sample_rate);
            break;
          case KRAD_PASS:
            kr_pass_set_sample_rate (kr_effects->effect[e].effect[c], kr_effects->sample_rate);
            break;
          case KRAD_TAPETUBE:
            kr_tapetube_set_sample_rate (kr_effects->effect[e].effect[c], kr_effects->sample_rate);
            break;
        }
      }
    }
  }
}

void kr_effects_process (kr_effects_t *kr_effects, float **input, float **output, int num_samples) {

  int e, c;

  for (e = 0; e < KRAD_EFFECTS_MAX; e++) {
    if (kr_effects->effect[e].active == 1) {
      for (c = 0; c < kr_effects->channels; c++) {
        switch (kr_effects->effect[e].effect_type) {
          case KRAD_NOFX:
            break;
          case KRAD_EQ:
            kr_eq_process (kr_effects->effect[e].effect[c], input[c], output[c], num_samples);
            break;
          case KRAD_PASS:
            kr_pass_process (kr_effects->effect[e].effect[c], input[c], output[c], num_samples);
            break;
          case KRAD_TAPETUBE:
            kr_tapetube_process (kr_effects->effect[e].effect[c], input[c], output[c], num_samples);
            break;
        }
      }
    }
  }
}


void kr_effects_effect_add (kr_effects_t *kr_effects, kr_effect_type_t effect) {
 
  int e, c;

  for (e = 0; e < KRAD_EFFECTS_MAX; e++) {
    if (kr_effects->effect[e].active == 0) {
      kr_effects->effect[e].effect_type = effect;
      for (c = 0; c < kr_effects->channels; c++) {
        switch (kr_effects->effect[e].effect_type) {
          case KRAD_NOFX:
            break;
          case KRAD_EQ:
            kr_effects->effect[e].effect[c] = kr_eq_create (kr_effects->sample_rate);
            break;
         case KRAD_PASS:
           kr_effects->effect[e].effect[c] = kr_pass_create (kr_effects->sample_rate);
           break;
         case KRAD_TAPETUBE:
           kr_effects->effect[e].effect[c] = kr_tapetube_create (kr_effects->sample_rate);
           break;
        }
      }
      kr_effects->effect[e].active = 1;
      break;
    }
  }
}

void kr_effects_effect_remove (kr_effects_t *kr_effects, int effect_num) {

  int c;

  for (c = 0; c < kr_effects->channels; c++) {
    switch (kr_effects->effect[effect_num].effect_type) {
      case KRAD_NOFX:
        break;
      case KRAD_EQ:
        kr_eq_destroy(kr_effects->effect[effect_num].effect[c]);
        break;
      case KRAD_PASS:
        kr_pass_destroy(kr_effects->effect[effect_num].effect[c]);
        break;
      case KRAD_TAPETUBE:
        kr_tapetube_destroy(kr_effects->effect[effect_num].effect[c]);
        break;
    }
    kr_effects->effect[effect_num].effect[c] = NULL;
  }

  kr_effects->effect[effect_num].active = 0;
}


void kr_effects_effect_set_control (kr_effects_t *kr_effects, int effect_num, int control, int subunit, float value,
                                    int duration, krad_ease_t ease) {

  int c;

  for (c = 0; c < kr_effects->channels; c++) {
    switch (kr_effects->effect[effect_num].effect_type) {
      case KRAD_NOFX:
        break;
      case KRAD_EQ:
        switch (control) {
          case KRAD_EQ_CONTROL_DB:
            kr_eq_band_set_db (kr_effects->effect[effect_num].effect[c], subunit, value, duration, ease);
            break;
          case KRAD_EQ_CONTROL_BANDWIDTH:
            kr_eq_band_set_bandwidth (kr_effects->effect[effect_num].effect[c], subunit, value, duration, ease);
            break;
          case KRAD_EQ_CONTROL_HZ:
            kr_eq_band_set_hz (kr_effects->effect[effect_num].effect[c], subunit, value, duration, ease);
            break;
        }
        break;
      case KRAD_PASS:
        switch (control) {
          case KRAD_PASS_CONTROL_TYPE:
            kr_pass_set_type (kr_effects->effect[effect_num].effect[c], value);
            break;
          case KRAD_PASS_CONTROL_BANDWIDTH:
            kr_pass_set_bandwidth (kr_effects->effect[effect_num].effect[c], value, duration, ease);
            break;
          case KRAD_PASS_CONTROL_HZ:
            kr_pass_set_hz (kr_effects->effect[effect_num].effect[c], value, duration, ease);
            break;
        }
        break;
      case KRAD_TAPETUBE:
        switch (control) {
          case KRAD_TAPETUBE_CONTROL_BLEND:
            kr_tapetube_set_blend (kr_effects->effect[effect_num].effect[c], value, duration, ease);
            break;
          case KRAD_TAPETUBE_CONTROL_DRIVE:
            kr_tapetube_set_drive (kr_effects->effect[effect_num].effect[c], value, duration, ease);
            break;
        }
        break;
    }
  }
}

kr_effect_type_t kr_effects_string_to_effect (char *string) {

	if ((strlen(string) == 8) && (strncmp(string, "tapetube", 4) == 0)) {
		return KRAD_TAPETUBE;
	}
	if ((strlen(string) == 4) && (strncmp(string, "pass", 4) == 0)) {
		return KRAD_PASS;
	}
	if ((strlen(string) == 2) && (strncmp(string, "eq", 2) == 0)) {
		return KRAD_EQ;
	}
	return KRAD_NOFX;
}

int kr_effects_string_to_effect_control (kr_effect_type_t effect_type, char *string) {

  if (effect_type == KRAD_EQ) {
	  if ((strlen(string) == 2) && (strncmp(string, "db", 2) == 0)) {
		  return KRAD_EQ_CONTROL_DB;
	  }
	  if ((strlen(string) == 9) && (strncmp(string, "bandwidth", 9) == 0)) {
		  return KRAD_EQ_CONTROL_BANDWIDTH;
	  }
	  if ((strlen(string) == 2) && (strncmp(string, "hz", 2) == 0)) {
		  return KRAD_EQ_CONTROL_HZ;
	  }
  }

  if (effect_type == KRAD_PASS) {
	  if ((strlen(string) == 4) && (strncmp(string, "type", 4) == 0)) {
		  return KRAD_PASS_CONTROL_TYPE;
	  }
	  if ((strlen(string) == 9) && (strncmp(string, "bandwidth", 9) == 0)) {
		  return KRAD_PASS_CONTROL_BANDWIDTH;
	  }
	  if ((strlen(string) == 2) && (strncmp(string, "hz", 2) == 0)) {
		  return KRAD_PASS_CONTROL_HZ;
	  }
  }
  
  if (effect_type == KRAD_TAPETUBE) {
	  if ((strlen(string) == 5) && (strncmp(string, "blend", 5) == 0)) {
		  return KRAD_TAPETUBE_CONTROL_BLEND;
	  }
	  if ((strlen(string) == 5) && (strncmp(string, "drive", 5) == 0)) {
		  return KRAD_TAPETUBE_CONTROL_DRIVE;
	  }
  }

	return 0;
}
