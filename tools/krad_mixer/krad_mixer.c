#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>

#include "krad_mixer.h"

void krad_mixer_crossfade_group_create (krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup1, krad_mixer_portgroup_t *portgroup2) {

	int x;
	krad_mixer_crossfade_group_t *crossfade_group;

	if (!(((portgroup1->direction == INPUT) || (portgroup1->direction == MIX)) &&
		  ((portgroup1->direction == INPUT) || (portgroup2->direction == MIX)) &&
		  (((portgroup1->io_type == MIXBUS) && (portgroup2->io_type == MIXBUS)) ||
		   ((portgroup1->io_type != MIXBUS) && (portgroup2->io_type != MIXBUS))))) {
		printf ("Invalid crossfade group!\n");
		exit (1);
	}
		
	if (portgroup1->crossfade_group != NULL) {
		krad_mixer_crossfade_group_destroy (krad_mixer, portgroup1->crossfade_group);
	}

	if (portgroup2->crossfade_group != NULL) {
		krad_mixer_crossfade_group_destroy (krad_mixer, portgroup2->crossfade_group);
	}

	for (x = 0; x < KRAD_MIXER_MAX_PORTGROUPS / 2; x++) {
		crossfade_group = &krad_mixer->crossfade_group[x];
		if ((crossfade_group != NULL) && ((crossfade_group->portgroup[0] == NULL) && (crossfade_group->portgroup[1] == NULL))) {
			break;
		}	
	}

	crossfade_group->portgroup[0] = portgroup1;
	crossfade_group->portgroup[1] = portgroup2;
	
	portgroup1->crossfade_group = crossfade_group;
	portgroup2->crossfade_group = crossfade_group;
	
	crossfade_group_set_crossfade (crossfade_group, -100.0f);
	
}

void krad_mixer_crossfade_group_destroy (krad_mixer_t *krad_mixer, krad_mixer_crossfade_group_t *crossfade_group) {

	crossfade_group->portgroup[0]->crossfade_group = NULL;
	crossfade_group->portgroup[1]->crossfade_group = NULL;

	crossfade_group->portgroup[0] = NULL;
	crossfade_group->portgroup[1] = NULL;
	crossfade_group->fade = -100.0f;

}

float get_fade_out (float crossfade_value) {

	float fade_out;

	fade_out = cos (3.14159f*0.5f*((crossfade_value + 100.0f) + 0.5f)/200.0f);
	fade_out = fade_out * fade_out;
		
	return fade_out;
		
}

float get_fade_in (float crossfade_value) {
	
	return 1.0f - get_fade_out (crossfade_value);
	
}

float portgroup_get_crossfade (krad_mixer_portgroup_t *portgroup) {

	if (portgroup->crossfade_group->portgroup[0] == portgroup) {
		return get_fade_out (portgroup->crossfade_group->fade);
	}

	if (portgroup->crossfade_group->portgroup[1] == portgroup) {
		return get_fade_in (portgroup->crossfade_group->fade);
	}
	
	printf ("failed to get portgroup for crossfade!\n");
	exit (1);
	
}

void portgroup_apply_volume (krad_mixer_portgroup_t *portgroup, int nframes) {

	int c, s, sign;
	
	sign = 0;

	for (c = 0; c < portgroup->channels; c++) {

		if (portgroup->new_volume_actual[c] == portgroup->volume_actual[c]) {
			for (s = 0; s < nframes; s++) {
				portgroup->samples[c][s] = portgroup->samples[c][s] * portgroup->volume_actual[c];
			}
		} else {
			
			/* The way the volume change is set up here, the volume can only change once per callback, but thats 
			   allways plenty of times per second */
			
			/* last_sign: 0 = unset, -1 neg, +1 pos */
				
			if (portgroup->last_sign[c] == 0) {
				if (portgroup->samples[c][0] > 0.0f) {
					portgroup->last_sign[c] = 1;
				} else {
					/* Zero counts as negative here, but its moot */
					portgroup->last_sign[c] = -1;
				}
			}
			
			for (s = 0; s < nframes; s++) {
				if (portgroup->last_sign[c] != 0) {
					if (portgroup->samples[c][s] > 0.0f) {
						sign = 1;
					} else {
						sign = -1;
					}
				
					if ((sign != portgroup->last_sign[c]) || (portgroup->samples[c][s] == 0.0f)) {
						portgroup->volume_actual[c] = portgroup->new_volume_actual[c];
						portgroup->last_sign[c] = 0;
					}
				}
				portgroup->samples[c][s] = (portgroup->samples[c][s] * portgroup->volume_actual[c]);
			}

			if (portgroup->last_sign[c] != 0) {
				portgroup->last_sign[c] = sign;
			}
		}
	}
}

float krad_mixer_peak_scale (float value) {
	
	float db;
	float def;

	db = 20.0f * log10f (value * 1.0f);

	if (db < -70.0f) {
		def = 0.0f;
	} else if (db < -60.0f) {
		def = (db + 70.0f) * 0.25f;
	} else if (db < -50.0f) {
		def = (db + 60.0f) * 0.5f + 2.5f;
	} else if (db < -40.0f) {
		def = (db + 50.0f) * 0.75f + 7.5;
	} else if (db < -30.0f) {
		def = (db + 40.0f) * 1.5f + 15.0f;
	} else if (db < -20.0f) {
		def = (db + 30.0f) * 2.0f + 30.0f;
	} else if (db < 0.0f) {
		def = (db + 20.0f) * 2.5f + 50.0f;
	} else {
		def = 100.0f;
	}

	return def;
}

float krad_mixer_portgroup_read_channel_peak (krad_mixer_portgroup_t *portgroup, int channel) {

	float peak;
	
	peak = portgroup->peak[channel];
	portgroup->peak[channel] = 0.0f;

	return peak;
}

float krad_mixer_portgroup_read_peak (krad_mixer_portgroup_t *portgroup) {

	float tmp = portgroup->peak[0];
	portgroup->peak[0] = 0.0f;

	float tmp2 = portgroup->peak[1];
	portgroup->peak[1] = 0.0f;
	if (tmp > tmp2) {
		return tmp;
	} else {
		return tmp2;
	}
}

void krad_mixer_portgroup_compute_channel_peak (krad_mixer_portgroup_t *portgroup, int channel, uint32_t nframes) {

	int s;
	float sample;

	for(s = 0; s < nframes; s++) {
		sample = fabs(portgroup->samples[channel][s]);
		if (sample > portgroup->peak[channel]) {
			portgroup->peak[channel] = sample;
		}
	}
}


void krad_mixer_portgroup_compute_peaks (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		krad_mixer_portgroup_compute_channel_peak (portgroup, c, nframes);
	}
}

void portgroup_clear_samples (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;
	int s;

	for (c = 0; c < portgroup->channels; c++) {
		for (s = 0; s < nframes; s++) {
			portgroup->samples[c][s] = 0.0f;
		}
	}
}

void portgroup_update_samples (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	switch ( portgroup->io_type ) {
		case KRAD_TONE:
			krad_tone_run (portgroup->io_ptr, portgroup->samples[0], nframes);
			memcpy (portgroup->samples[1], portgroup->samples[0], nframes * 4);
			break;
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			krad_audio_portgroup_samples_callback (nframes, portgroup->io_ptr, &portgroup->samples[0]);
			break;
		case KRAD_LINK:
			if (portgroup->direction == INPUT) {
				krad_link_audio_samples_callback (nframes, portgroup->io_ptr, &portgroup->samples[0]);
			}
			break;
	}
	
}

void portgroup_mix_samples (krad_mixer_portgroup_t *dest_portgroup, krad_mixer_portgroup_t *src_portgroup, uint32_t nframes) {

	int c;
	int s;

	if (dest_portgroup->channels != src_portgroup->channels) {
		printf ("oh no channel count not equal!\n");
		exit (1);
	}

	for (c = 0; c < dest_portgroup->channels; c++) {
		for (s = 0; s < nframes; s++) {
			dest_portgroup->samples[c][s] += src_portgroup->samples[c][s];
		}
	}
}

void portgroup_copy_samples (krad_mixer_portgroup_t *dest_portgroup, krad_mixer_portgroup_t *src_portgroup, uint32_t nframes) {

	int c;
	int s;

	if (dest_portgroup->channels != src_portgroup->channels) {
		printf ("oh no channel count not equal!\n");
		exit (1);
	}

	switch ( dest_portgroup->io_type ) {
		case KRAD_TONE:
			break;
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			for (c = 0; c < dest_portgroup->channels; c++) {
				for (s = 0; s < nframes; s++) {
					dest_portgroup->samples[c][s] = src_portgroup->samples[c][s];
				}
			}
			break;
		case KRAD_LINK:
			if (dest_portgroup->direction == OUTPUT) {
				krad_link_audio_samples_callback (nframes, dest_portgroup->io_ptr, src_portgroup->samples);
			}
			break;
	}
}

void portgroup_hardlimit (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;

	for (c = 0; c < portgroup->channels; c++) {	
		hardlimit (portgroup->samples[c], nframes);
	}
}

int krad_mixer_process (uint32_t nframes, krad_mixer_t *krad_mixer) {
	
	int p, m;

	krad_mixer_portgroup_t *portgroup = NULL;
	krad_mixer_portgroup_t *mixbus = NULL;
	
	if (krad_mixer->push_tone != NULL) {
		krad_tone_add_preset (krad_mixer->tone_port->io_ptr, krad_mixer->push_tone);
		krad_mixer->push_tone = NULL;
	}
	
	// Gets input/output port buffers
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && ((portgroup->direction == INPUT) || (portgroup->direction == OUTPUT))) {
			portgroup_update_samples (portgroup, nframes);
		}
	}

	// apply volume and calc peaks on inputs
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
			portgroup_apply_volume (portgroup, nframes);
			krad_mixer_portgroup_compute_peaks (portgroup, nframes);
			//printf("%f\n", portgroup->samples[0][128]);
		}
	}
	
	// Clear Mixes	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->io_type == MIXBUS)) {
			portgroup_clear_samples (portgroup, nframes);
		}
	}

	// Mix
	for (m = 0; m < KRAD_MIXER_MAX_PORTGROUPS; m++) {
		mixbus = krad_mixer->portgroup[m];
		if ((mixbus != NULL) && (mixbus->active) && (mixbus->io_type == MIXBUS)) {
			for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
				portgroup = krad_mixer->portgroup[p];
				if ((portgroup != NULL) && (portgroup->active) && (portgroup->mixbus == mixbus) && (portgroup->direction == INPUT)) {
					portgroup_mix_samples ( mixbus, portgroup, nframes );
				}
			}
		}
	}

	// copy to outputs, hardlimit all outputs
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == OUTPUT)) {
			portgroup_hardlimit ( portgroup->mixbus, nframes );
			portgroup_copy_samples ( portgroup, portgroup->mixbus, nframes );
		}
	}
	
	// deactivate ports that need to be deactivated
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (portgroup->active == 2) {
				portgroup->active = 0;
			}
		}
	}

	return 0;      

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


krad_mixer_portgroup_t *krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, char *sysname, int direction, int channels, 
													 krad_mixer_mixbus_t *mixbus, krad_mixer_portgroup_io_t io_type, 
													 void *io_ptr, krad_audio_api_t api) {

	int p;
	int c;
	krad_mixer_portgroup_t *portgroup;
	
	portgroup = NULL;

	/* prevent dupe names */
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_mixer->portgroup[p]->active != 0) {
			if (strcmp(sysname, krad_mixer->portgroup[p]->sysname) == 0) {
				return NULL;
			}
		}
	}
	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_mixer->portgroup[p]->active == 0) {
			portgroup = krad_mixer->portgroup[p];
			break;
		}
	}
	
	if (portgroup == NULL) {
		return NULL;
	}

	portgroup->krad_mixer = krad_mixer;

	strcpy (portgroup->sysname, sysname);
	portgroup->channels = channels;
	portgroup->io_type = io_type;
	portgroup->mixbus = mixbus;
	portgroup->direction = direction;

	for (c = 0; c < portgroup->channels; c++) {
	
		portgroup->volume[c] = 90;
		portgroup->volume_actual[c] = (float)(portgroup->volume[c]/100.0f);
		portgroup->volume_actual[c] *= portgroup->volume_actual[c];
		portgroup->new_volume_actual[c] = portgroup->volume_actual[c];

		switch ( portgroup->io_type ) {
			case KRAD_TONE:
				portgroup->samples[c] = calloc (1, 16384);			
				break;		
			case MIXBUS:
				portgroup->samples[c] = calloc (1, 16384);
				break;
			case KRAD_AUDIO:
				break;
			case KRAD_LINK:
				portgroup->samples[c] = calloc (1, 16384);
				break;
		}
	}
	
	switch ( portgroup->io_type ) {
		case KRAD_TONE:
			portgroup->io_ptr = krad_tone_create (krad_mixer->sample_rate);
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			portgroup->io_ptr = krad_audio_portgroup_create (krad_mixer->krad_audio, portgroup->sysname, 
															 portgroup->direction, portgroup->channels, api);
			break;
		case KRAD_LINK:
			portgroup->io_ptr = io_ptr;
			break;
	}
	
	if (portgroup->io_type != KRAD_LINK) {
		portgroup->krad_tags = krad_tags_create ();
	} else {
		portgroup->krad_tags = krad_link_get_tags (portgroup->io_ptr);
	}

	if (portgroup->krad_tags == NULL) {
		failfast ("Oh I couldn't find me tags\n");
	}

	portgroup->active = 1;

	return portgroup;

}


void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup) {

	int c;

	if (portgroup == NULL) {
		return;
	}

	portgroup->active = 2;

	while (portgroup->active != 0) {
		if (krad_mixer->pusher == 0) {
			portgroup->active = 0;
			break;
		}
		usleep(15000);
	}

	printf("Krad Mixer: Removing %d channel Portgroup %s\n", portgroup->channels, portgroup->sysname);

	for (c = 0; c < portgroup->channels; c++) {
		switch ( portgroup->io_type ) {
			case KRAD_TONE:
				free ( portgroup->samples[c] );			
				break;
			case MIXBUS:
				free ( portgroup->samples[c] );
				break;
			case KRAD_AUDIO:
				break;
			case KRAD_LINK:
				free ( portgroup->samples[c] );			
				break;
		}
	}
	

	switch ( portgroup->io_type ) {
		case KRAD_TONE:
			krad_tone_destroy (portgroup->io_ptr);
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			krad_audio_portgroup_destroy (portgroup->io_ptr);
			break;
		case KRAD_LINK:
			break;
	}
	
	if (portgroup->io_type != KRAD_LINK) {
		krad_tags_destroy (portgroup->krad_tags);	
	}	
	
}

krad_mixer_portgroup_t *krad_mixer_get_portgroup_from_sysname (krad_mixer_t *krad_mixer, char *sysname) {

	int p;
	krad_mixer_portgroup_t *portgroup;

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (strcmp(sysname, portgroup->sysname) == 0) {	
				return portgroup;
			}
		}
	}

	return NULL;
}

void portgroup_set_crossfade (krad_mixer_portgroup_t *portgroup, float value) {

	if (portgroup->crossfade_group != NULL) {

		portgroup->crossfade_group->fade = value;	

		portgroup_update_volume (portgroup->crossfade_group->portgroup[0]);
		portgroup_update_volume (portgroup->crossfade_group->portgroup[1]);	
	}
}

void crossfade_group_set_crossfade (krad_mixer_crossfade_group_t *crossfade_group, float value) {

	crossfade_group->fade = value;	

	if ((crossfade_group->portgroup[0] != NULL) && (crossfade_group->portgroup[1] != NULL)) {
		portgroup_update_volume (crossfade_group->portgroup[0]);
		portgroup_update_volume (crossfade_group->portgroup[1]);	
	}
}

void portgroup_set_volume (krad_mixer_portgroup_t *portgroup, float value) {

	int c;
	float volume_temp;

	volume_temp = (value/100.0f);
	volume_temp *= volume_temp;

	if (portgroup->crossfade_group != NULL) {
		volume_temp = volume_temp * portgroup_get_crossfade (portgroup);
	}

	for (c = 0; c < portgroup->channels; c++) {
		portgroup->volume[c] = value;
		portgroup->new_volume_actual[c] = volume_temp;
	}

}

void portgroup_set_channel_volume (krad_mixer_portgroup_t *portgroup, int channel, float value) {

	float volume_temp;

	portgroup->volume[channel] = value;
	volume_temp = (portgroup->volume[channel]/100.0f);
	volume_temp *= volume_temp;

	if (portgroup->crossfade_group != NULL) {
		volume_temp = volume_temp * portgroup_get_crossfade (portgroup);
	}

	portgroup->new_volume_actual[channel] = volume_temp;

}

void portgroup_update_volume (krad_mixer_portgroup_t *portgroup) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		portgroup_set_channel_volume (portgroup, c, portgroup->volume[c]);
	}	
}		

int krad_mixer_set_portgroup_control (krad_mixer_t *krad_mixer, char *sysname, char *control, float value) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, sysname);
	
	if (portgroup != NULL) {
			
		if ((strncmp(control, "volume", 6) == 0) && (strlen(control) == 6)) {
			portgroup_set_volume (portgroup, value);
			return 1;
		}

		if ((strncmp(control, "crossfade", 9) == 0) && (strlen(control) == 9)) {
			portgroup_set_crossfade (portgroup, value);
			return 1;
		}				

		if (strncmp(control, "volume_left", 11) == 0) {
			portgroup_set_channel_volume (portgroup, 0, value);
			return 1;	
		}
		
		if (strncmp(control, "volume_right", 12) == 0) {
			portgroup_set_channel_volume (portgroup, 1, value);
			return 1;
		}
	}
		
	return 0;
}

void krad_mixer_destroy (krad_mixer_t *krad_mixer) {

	int p;

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_mixer->portgroup[p]->active == 1) {
			krad_mixer_portgroup_destroy (krad_mixer, krad_mixer->portgroup[p]);
		}
	}
	
	free ( krad_mixer->crossfade_group );

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		free ( krad_mixer->portgroup[p] );
	}
	
	free ( krad_mixer->name );

	free ( krad_mixer );
	
}

void krad_mixer_unset_pusher (krad_mixer_t *krad_mixer) {
	krad_mixer->pusher = 0;
}

int krad_mixer_has_pusher (krad_mixer_t *krad_mixer) {
	if (krad_mixer->pusher == 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}

krad_audio_api_t krad_mixer_get_pusher (krad_mixer_t *krad_mixer) {
	return krad_mixer->pusher;
}

void krad_mixer_set_pusher (krad_mixer_t *krad_mixer, krad_audio_api_t pusher) {
	if (pusher == 0) {
		krad_mixer_unset_pusher (krad_mixer);
	} else {
		krad_mixer->pusher = pusher;
	}
}

int krad_mixer_get_sample_rate (krad_mixer_t *krad_mixer) {
	return krad_mixer->sample_rate;
}

void krad_mixer_set_sample_rate (krad_mixer_t *krad_mixer, int sample_rate) {
	krad_mixer->sample_rate = sample_rate;
	krad_tone_set_sample_rate (krad_mixer->tone_port->io_ptr, krad_mixer->sample_rate);
}

krad_mixer_t *krad_mixer_create (char *name) {

	int p;

	krad_mixer_t *krad_mixer;

	if ((krad_mixer = calloc (1, sizeof (krad_mixer_t))) == NULL) {
		fprintf (stderr, "Krad Mixer memory alloc failure\n");
		exit (1);
	}
	
	krad_mixer->name = strdup (name);
	krad_mixer->sample_rate = KRAD_MIXER_DEFAULT_SAMPLE_RATE;
	
	krad_mixer->crossfade_group = calloc (KRAD_MIXER_MAX_PORTGROUPS / 2, sizeof (krad_mixer_crossfade_group_t));

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		krad_mixer->portgroup[p] = calloc (1, sizeof (krad_mixer_portgroup_t));
	}
	
	krad_mixer->krad_audio = krad_audio_create (krad_mixer);
	
	krad_mixer->master_mix = krad_mixer_portgroup_create (krad_mixer, "Master", MIX, 2, NULL, MIXBUS, NULL, 0);
	
	krad_mixer->tone_port = krad_mixer_portgroup_create (krad_mixer, "DTMF", INPUT, 2,
										 				 krad_mixer->master_mix, KRAD_TONE, NULL, 0);
	
	return krad_mixer;
	
}


int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc ) {

	uint32_t command;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
//	uint64_t number;
	krad_mixer_portgroup_t *portgroup;
	krad_mixer_portgroup_t *portgroup2;
	uint64_t element;
	
	uint64_t response;
//	uint64_t broadcast;
	
	char *crossfade_name;
	float crossfade_value;
	int p;
	
	ebml_id = 0;
	
	char portname[1024];
	char portgroupname[1024];
	char controlname[1024];	
	float floatval;

	char string[1024];
	int direction;

	direction = 0;
	crossfade_name = NULL;
	//i = 0;
	
	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {
	
		case EBML_ID_KRAD_MIXER_CMD_GET_CONTROL:
			//printk ("Get Control\n");
			return 1;
			break;	
		case EBML_ID_KRAD_MIXER_CMD_SET_CONTROL:
			//printf("krad mixer handler! got set control\n");			

		
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
				printke ("hrm wtf2\n");
			} else {
				//printf("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portname, ebml_data_size);
	
			//printk ("Set Control: %s / ", portname);
	
	
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
				printke ("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, controlname, ebml_data_size);
	
			//printk ("%s = ", controlname);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
				floatval = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
	
				//printk ("%f\n", floatval);

				krad_mixer_set_portgroup_control (krad_mixer, portname, controlname, floatval);

				krad_ipc_server_mixer_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
			} else {

			}

			return 2;
			
		case EBML_ID_KRAD_MIXER_CMD_PUSH_TONE:
		
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_TONE_NAME) {
				printke ("hrm wtf2\n");
			} else {
				//printf("tag name size %zu\n", ebml_data_size);
			}
			if (krad_mixer->push_tone == NULL) {
				krad_ebml_read_string (krad_ipc->current_client->krad_ebml, krad_mixer->push_tone_value, ebml_data_size);
				krad_mixer->push_tone = krad_mixer->push_tone_value;
			} else {
				krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
			}
			break;
		case EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS:

			//printk ("List Portgroups\n");

			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_MIXER_PORTGROUP_LIST, &element);
			
			for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
				portgroup = krad_mixer->portgroup[p];
				if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
					crossfade_name = "";
					crossfade_value = 0.0f;
					if (portgroup->crossfade_group != NULL) {
					
						if (portgroup->crossfade_group->portgroup[0] == portgroup) {
							crossfade_name = portgroup->crossfade_group->portgroup[1]->sysname;
							crossfade_value = portgroup->crossfade_group->fade;
						}
					}
				
					krad_ipc_server_response_add_portgroup ( krad_ipc, portgroup->sysname, portgroup->channels,
											  				 portgroup->io_type, portgroup->volume[0],  portgroup->mixbus->sysname, crossfade_name, crossfade_value );
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );
			return 1;
			break;
		// FIXME create / remove portgroup need broadcast	
		case EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP:
		
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
				printke ("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION ) {
				printke ("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

			if (strncmp(string, "output", 6) == 0) {
				direction = OUTPUT;
			} else {
				direction = INPUT;
			}

			portgroup = krad_mixer_portgroup_create (krad_mixer, portgroupname, direction, 2,
										 krad_mixer->master_mix, KRAD_AUDIO, NULL, JACK);			

			if (portgroup != NULL) {

				krad_ipc_server_broadcast_portgroup_created ( krad_ipc, portgroup->sysname, portgroup->channels,
												  	   		  portgroup->io_type, portgroup->volume[0], portgroup->mixbus->sysname );
			}
		
			break;
		case EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP:	
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
				printke ("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
			
			krad_mixer_portgroup_destroy (krad_mixer, krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname));
		
			krad_ipc_server_simple_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);		
		
			break;
		case EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP:			
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
				printke ("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);

			// set tag / add/remove effect / set/rm crossfade group partner

			if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
			
				krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
				
				portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);
				
				if (portgroup != NULL) {
					if (portgroup->crossfade_group != NULL) {
				
						krad_mixer_crossfade_group_destroy (krad_mixer, portgroup->crossfade_group);
				
						if (strlen(string) == 0) {
							krad_ipc_server_mixer_broadcast2 ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED, portgroupname, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, "");		
							return 0;
						}
					}

					if (strlen(string) > 0) {
				
						portgroup2 = krad_mixer_get_portgroup_from_sysname (krad_mixer, string);
				
						if (portgroup2 != NULL) {
							if (portgroup2->crossfade_group != NULL) {
								krad_mixer_crossfade_group_destroy (krad_mixer, portgroup2->crossfade_group);
							}
						
							if (portgroup != portgroup2) {
						
								krad_mixer_crossfade_group_create (krad_mixer, portgroup, portgroup2);

								krad_ipc_server_mixer_broadcast2 ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED, portgroupname, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, string);
							}
						}
					}
				}
			}

			break;
	}

	return 0;

}

