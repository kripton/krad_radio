#include "krad_mixer_common.h"

kr_mixer_portgroup_control_rep_t *kr_mixer_portgroup_control_rep_create () {
  kr_mixer_portgroup_control_rep_t *kr_mixer_portgroup_control_rep = (kr_mixer_portgroup_control_rep_t *) calloc (1, sizeof (kr_mixer_portgroup_control_rep_t));
  return kr_mixer_portgroup_control_rep;
}

void kr_mixer_portgroup_control_rep_destroy (kr_mixer_portgroup_control_rep_t *kr_mixer_portgroup_control_rep) {
  free (kr_mixer_portgroup_control_rep);
}

krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep_create() {
  krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep = (krad_mixer_portgroup_rep_t *) calloc (1, sizeof (krad_mixer_portgroup_rep_t));
  krad_mixer_portgroup_rep->mixbus_rep = krad_mixer_mixbus_rep_create();
  return krad_mixer_portgroup_rep;
}

void krad_mixer_portgroup_rep_destroy (krad_mixer_portgroup_rep_t *portgroup_rep) {
  if (portgroup_rep->crossfade_group_rep != NULL) {
    krad_mixer_crossfade_group_rep_destroy (portgroup_rep->crossfade_group_rep);
  }
  krad_mixer_mixbus_rep_destroy (portgroup_rep->mixbus_rep);
  free (portgroup_rep);
}

krad_mixer_mixbus_rep_t *krad_mixer_mixbus_rep_create () {
  krad_mixer_mixbus_rep_t *mixbus_rep = (krad_mixer_mixbus_rep_t *) calloc (1, sizeof (krad_mixer_mixbus_rep_t));
  return mixbus_rep;
}

void krad_mixer_mixbus_rep_destroy (krad_mixer_mixbus_rep_t *mixbus_rep) {
  free (mixbus_rep);
}

krad_mixer_crossfade_group_rep_t *krad_mixer_crossfade_rep_create (krad_mixer_portgroup_rep_t *portgroup_rep1, krad_mixer_portgroup_rep_t *portgroup_rep2) {
  krad_mixer_crossfade_group_rep_t *crossfade_group_rep = (krad_mixer_crossfade_group_rep_t *) calloc (1, sizeof (krad_mixer_crossfade_group_rep_t));
 
  if (portgroup_rep1->crossfade_group_rep != NULL) {
		krad_mixer_crossfade_group_rep_destroy (portgroup_rep1->crossfade_group_rep);
	}

	if (portgroup_rep2->crossfade_group_rep != NULL) {
		krad_mixer_crossfade_group_rep_destroy (portgroup_rep2->crossfade_group_rep);
	}

	crossfade_group_rep->portgroup_rep[0] = portgroup_rep1;
	crossfade_group_rep->portgroup_rep[1] = portgroup_rep2;
	
	portgroup_rep1->crossfade_group_rep = crossfade_group_rep;
	portgroup_rep2->crossfade_group_rep = crossfade_group_rep;
	
	crossfade_group_rep->fade = -100.0f;
  
  return crossfade_group_rep;
}

void krad_mixer_crossfade_group_rep_destroy (krad_mixer_crossfade_group_rep_t *crossfade_group_rep) {
  free (crossfade_group_rep);
}

void krad_mixer_portgroup_rep_to_ebml (krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep, krad_ebml_t *krad_ebml) {

  uint64_t portgroup;
  
  char *crossfade_name;
	float crossfade_value;
  int has_xmms2;
  int i;
  
  crossfade_name = "";
  crossfade_value = 0.0f;
  has_xmms2 = 0;
  
  if (krad_mixer_portgroup_rep->has_xmms2 > 0) {
    has_xmms2 = 1;
  }
  if (krad_mixer_portgroup_rep->crossfade_group_rep != NULL) {
  
    if (krad_mixer_portgroup_rep->crossfade_group_rep->portgroup_rep[0] == krad_mixer_portgroup_rep) {
      crossfade_name = krad_mixer_portgroup_rep->crossfade_group_rep->portgroup_rep[1]->sysname;
      crossfade_value = krad_mixer_portgroup_rep->crossfade_group_rep->fade;
    }
  }
  
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP, &portgroup);	

	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, krad_mixer_portgroup_rep->sysname);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS, krad_mixer_portgroup_rep->channels);
	if (krad_mixer_portgroup_rep->io_type == 0) {
		krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_TYPE, "Jack");
	} else {
		krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_TYPE, "Internal");
	}
  for (i = 0; i < krad_mixer_portgroup_rep->channels; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME, krad_mixer_portgroup_rep->volume[i]);	    
  }
  for (i = 0; i < krad_mixer_portgroup_rep->channels; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME, krad_mixer_portgroup_rep->peak[i]); //FIXME: VOLUME -> PEAK  
  }
  for (i = 0; i < krad_mixer_portgroup_rep->channels; i++) {
    krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME, krad_mixer_portgroup_rep->rms[i]);	    
  }
 
  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS, krad_mixer_portgroup_rep->mixbus_rep->sysname);			

	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, crossfade_name);
	krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE, crossfade_value);	
  
	krad_ebml_write_int8 (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2, has_xmms2);
	krad_ebml_finish_element (krad_ebml, portgroup);
  
}

krad_mixer_portgroup_rep_t *krad_mixer_ebml_to_krad_mixer_portgroup_rep (krad_ebml_t *krad_ebml, uint64_t *data_read, krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep) {
  
  uint32_t ebml_id;
	uint64_t ebml_data_size;
  krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep_ret; 
  krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep_crossfade;
  
  int i;
  
  char string[1024];
  
  char crossfade_name[128];
	float crossfade_value;
  
  if (krad_mixer_portgroup_rep == NULL) {
    krad_mixer_portgroup_rep_ret = krad_mixer_portgroup_rep_create();
  } else {
    krad_mixer_portgroup_rep_ret = krad_mixer_portgroup_rep;
  }
  
	krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
	
	*data_read = ebml_data_size;
	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP) {
		//printk ("hrm wtf");
	} else {
		//printk ("tag size %"PRIu64"", ebml_data_size);
	}
	
	krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (krad_ebml, krad_mixer_portgroup_rep_ret->sysname, ebml_data_size);
	
	//printk ("Input name: %s", portname);
	
	krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS) {
		//printk ("hrm wtf3");
	} 
	krad_mixer_portgroup_rep_ret->channels = krad_ebml_read_number (krad_ebml, ebml_data_size);
	
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_TYPE) {
		//printk ("hrm wtf2");
	}
	krad_ebml_read_string (krad_ebml, string, ebml_data_size);
  if (strncmp (string, "Jack", 4) == 0) {
    krad_mixer_portgroup_rep_ret->io_type = 0;
  } else {
    krad_mixer_portgroup_rep_ret->io_type = 1;    
  }
	
  for (i = 0; i < krad_mixer_portgroup_rep_ret->channels; i++) {
    krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
    if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
      //printk ("hrm wtf3");
    } 
    krad_mixer_portgroup_rep_ret->volume[i] = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
	
  for (i = 0; i < krad_mixer_portgroup_rep_ret->channels; i++) {
    krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
    if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
      //printk ("hrm wtf3");
    } 
    krad_mixer_portgroup_rep_ret->peak[i] = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  for (i = 0; i < krad_mixer_portgroup_rep_ret->channels; i++) {
    krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
    if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
      //printk ("hrm wtf3");
    } 
    krad_mixer_portgroup_rep_ret->rms[i] = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS) {
		//printk ("hrm wtf2");
	} 
	krad_ebml_read_string (krad_ebml, krad_mixer_portgroup_rep_ret->mixbus_rep->sysname, ebml_data_size);	
	
	//printk ("Bus: %s", string);

	krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (krad_ebml, crossfade_name, ebml_data_size);	

	krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}

	crossfade_value = krad_ebml_read_float (krad_ebml, ebml_data_size);
	
	if (strlen(crossfade_name)) {
    krad_mixer_portgroup_rep_crossfade = krad_mixer_portgroup_rep_create ();
    strcpy (krad_mixer_portgroup_rep_crossfade->sysname, crossfade_name);
    
    krad_mixer_portgroup_rep_ret->crossfade_group_rep = krad_mixer_crossfade_rep_create (krad_mixer_portgroup_rep_ret, krad_mixer_portgroup_rep_crossfade);
    krad_mixer_portgroup_rep_ret->crossfade_group_rep->fade = crossfade_value;
	} 
	
	krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}
	
	krad_mixer_portgroup_rep_ret->has_xmms2 = krad_ebml_read_number (krad_ebml, ebml_data_size);
	
	return krad_mixer_portgroup_rep_ret;
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
