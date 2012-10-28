#include "krad_sfx.h"


kr_effects_t *kr_effects_create (int channels, int sample_rate) {

  kr_effects_t *kr_effects = calloc (1, sizeof(kr_effects_t));

  kr_effects->effect = calloc (KRAD_EFFECTS_MAX, sizeof(kr_effect_t));

  //failcheck alloc

  kr_effects->channels = channels;
  kr_effects->sample_rate = sample_rate;

  return kr_effects;
}

void kr_effects_destroy (kr_effects_t *kr_effects) {

  int x;

  for (x = 0; x < KRAD_EFFECTS_MAX; x++) {
    if (kr_effects->effect[x].active == 1) {
      kr_effects_effect_remove (kr_effects, x);
    }
  }

  free (kr_effects->effect);
  free (kr_effects);
}

void kr_effects_set_sample_rate (kr_effects_t *kr_effects, int sample_rate) {

  int x, c;

  kr_effects->sample_rate = sample_rate;

  for (x = 0; x < KRAD_EFFECTS_MAX; x++) {
    if (kr_effects->effect[x].active == 1) {
      for (c = 0; c < kr_effects->channels; c++) {
        switch (kr_effects->effect[x].effect_type) {
          case KRAD_NOFX:
            break;
          case KRAD_EQ:
            kr_eq_set_sample_rate (kr_effects->effect[x].effect[c], kr_effects->sample_rate);
            break;
          case KRAD_PASS:
            kr_pass_set_sample_rate (kr_effects->effect[x].effect[c], kr_effects->sample_rate);
            break;
        }
      }
    }
  }
}

void kr_effects_process (kr_effects_t *kr_effects, float **input, float **output, int num_samples) {

  int x, c;

  for (x = 0; x < KRAD_EFFECTS_MAX; x++) {
    if (kr_effects->effect[x].active == 1) {
      for (c = 0; c < kr_effects->channels; c++) {
        switch (kr_effects->effect[x].effect_type) {
          case KRAD_NOFX:
            break;
          case KRAD_EQ:
            kr_eq_process (kr_effects->effect[x].effect[c], input[c], output[c], num_samples);
            break;
          case KRAD_PASS:
            kr_pass_process (kr_effects->effect[x].effect[c], input[c], output[c], num_samples);
            break;
        }
      }
    }
  }
}


void kr_effects_effect_add (kr_effects_t *kr_effects, kr_effect_type_t effect) {
 
  int x, c;

  for (x = 0; x < KRAD_EFFECTS_MAX; x++) {
    if (kr_effects->effect[x].active == 0) {
      kr_effects->effect[x].effect_type = effect;
      for (c = 0; c < kr_effects->channels; c++) {
        switch (kr_effects->effect[x].effect_type) {
          case KRAD_NOFX:
            break;
          case KRAD_EQ:
            kr_effects->effect[x].effect[c] = kr_eq_create (kr_effects->sample_rate);
            break;
         case KRAD_PASS:
           kr_effects->effect[x].effect[c] = kr_pass_create (kr_effects->sample_rate);
           break;
        }
      }
      kr_effects->effect[x].active = 1;
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
    }
    kr_effects->effect[effect_num].effect[c] = NULL;
  }

  kr_effects->effect[effect_num].active = 0;

}


void kr_effects_effect_set_control (kr_effects_t *kr_effects, int effect_num, int control, int subunit, float value) {

  int c;

  for (c = 0; c < kr_effects->channels; c++) {
    switch (kr_effects->effect[effect_num].effect_type) {
      case KRAD_NOFX:
        break;
      case KRAD_EQ:
        switch (control) {
          case KRAD_EQ_CONTROL_DB:
            kr_eq_band_set_db (kr_effects->effect[effect_num].effect[c], subunit, value);
            break;
          case KRAD_EQ_CONTROL_BANDWIDTH:
            kr_eq_band_set_bandwidth (kr_effects->effect[effect_num].effect[c], subunit, value);
            break;
          case KRAD_EQ_CONTROL_HZ:
            kr_eq_band_set_hz (kr_effects->effect[effect_num].effect[c], subunit, value);
            break;
          case KRAD_EQ_OPCONTROL_ADDBAND:
            kr_eq_band_add (kr_effects->effect[effect_num].effect[c], value);
            break;
          case KRAD_EQ_OPCONTROL_RMBAND:
            kr_eq_band_add (kr_effects->effect[effect_num].effect[c], value);
            break;
        }
        break;
      case KRAD_PASS:
        switch (control) {
          case KRAD_PASS_CONTROL_TYPE:
            kr_pass_set_type (kr_effects->effect[effect_num].effect[c], value);
            break;
          case KRAD_PASS_CONTROL_BANDWIDTH:
            kr_pass_set_bandwidth (kr_effects->effect[effect_num].effect[c], value);
            break;
          case KRAD_PASS_CONTROL_HZ:
            kr_pass_set_hz (kr_effects->effect[effect_num].effect[c], value);
            break;
        }
        break;
    }
  }
}

kr_effect_type_t kr_effects_string_to_effect (char *string) {

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
	
	  if ((strlen(string) == 7) && (strncmp(string, "addband", 7) == 0)) {
		  return KRAD_EQ_OPCONTROL_ADDBAND;
	  }
	
	  if ((strlen(string) == 6) && (strncmp(string, "rmband", 6) == 0)) {
		  return KRAD_EQ_OPCONTROL_RMBAND;
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

	return 0;
}
