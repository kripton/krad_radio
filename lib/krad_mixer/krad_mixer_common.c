#include "krad_mixer_common.h"

krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep_create () {
  krad_mixer_portgroup_rep_t *portgroup_rep;
  portgroup_rep = calloc (1, sizeof (krad_mixer_portgroup_rep_t));
  return portgroup_rep;
}

void krad_mixer_portgroup_rep_destroy (krad_mixer_portgroup_rep_t *portgroup_rep) {
  free (portgroup_rep);
}

void krad_mixer_portgroup_rep_to_ebml (kr_portgroup_t *portgroup_rep, krad_ebml_t *krad_ebml) {

  uint64_t portgroup;
  int i;

  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP, &portgroup);

	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_rep->sysname);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS, portgroup_rep->channels);
	if (portgroup_rep->io_type == 0) {
		krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_TYPE, "Jack");
	} else {
		krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_TYPE, "Internal");
	}
  for (i = 0; i < portgroup_rep->channels; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME, portgroup_rep->volume[i]);	    
  }
  for (i = 0; i < portgroup_rep->channels; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME, portgroup_rep->peak[i]);
  }
  for (i = 0; i < portgroup_rep->channels; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME, portgroup_rep->rms[i]);	    
  }
 
  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS, portgroup_rep->mixbus);			

	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, portgroup_rep->crossfade_group);
	krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE, portgroup_rep->fade);	
  
	krad_ebml_write_int8 (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2, portgroup_rep->has_xmms2);
	if (portgroup_rep->has_xmms2 == 1) {
	  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2, portgroup_rep->xmms2_ipc_path);
  }

 /* 
  for (i = 0; i < KRAD_EQ_MAX_BANDS; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, krad_mixer_portgroup_rep->eq.band[i].db);
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, krad_mixer_portgroup_rep->eq.band[i].bandwidth);
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, krad_mixer_portgroup_rep->eq.band[i].hz);
    // ("NOW hz is %f %f\n", krad_mixer_portgroup_rep->eq.band[i].hz); 
  }
  */

  krad_ebml_write_data (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, &portgroup_rep->eq, sizeof(kr_eq_rep_t));
  
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, portgroup_rep->lowpass.hz);
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, portgroup_rep->lowpass.bandwidth);

  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, portgroup_rep->highpass.hz);
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, portgroup_rep->highpass.bandwidth);

  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, portgroup_rep->analog.drive);
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_EFFECT_CONTROL, portgroup_rep->analog.blend);
	
	krad_ebml_finish_element (krad_ebml, portgroup);
  
}

char *krad_mixer_channel_number_to_string (int channel) {

  switch ( channel ) {
    case 0:
      return "Left";
    case 1:
      return "Right";
    case 2:
      return "RearLeft";
    case 3:
      return "RearRight";
    case 4:
      return "Center";
    case 5:
      return "Sub";
    case 6:
      return "BackLeft";
    case 7:
      return "BackRight";
    default:
      return "Unknown";
  }
}

char *effect_control_to_string (kr_mixer_effect_control_t effect_control) {

  switch (effect_control) {
    case DB:
      return "db";
    case HZ:
      return "hz";
    case BANDWIDTH:
      return "bandwidth";
    case TYPE:
      return "type";
    case DRIVE:
      return "drive";
    case BLEND:
      return "blend";
  }
  return "Unknown";
}

krad_mixer_rep_t *krad_mixer_rep_create () {
  krad_mixer_rep_t *mixer_rep = (krad_mixer_rep_t *) calloc (1, sizeof (krad_mixer_rep_t));
  return mixer_rep;
}

void krad_mixer_rep_destroy (krad_mixer_rep_t *mixer_rep) {
  free (mixer_rep);
}
