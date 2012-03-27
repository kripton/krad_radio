#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>

#include <jack/jack.h>

#include "krad_mixer.h"


int krad_mixer_jack_callback (jack_nframes_t nframes, void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	
	krad_mixer_process ( krad_mixer, nframes );
	
	return 0;
	
}


void krad_mixer_crossfade_group_create (krad_mixer_t *krad_mixer, portgroup_t *portgroup1, portgroup_t *portgroup2) {

	int x;
	crossfade_group_t *crossfade_group;

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

	for (x = 0; x < PORTGROUP_MAX / 2; x++) {
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

void krad_mixer_crossfade_group_destroy (krad_mixer_t *krad_mixer, crossfade_group_t *crossfade_group) {

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

float portgroup_get_crossfade (portgroup_t *portgroup) {

	if (portgroup->crossfade_group->portgroup[0] == portgroup) {
		return get_fade_out (portgroup->crossfade_group->fade);
	}

	if (portgroup->crossfade_group->portgroup[1] == portgroup) {
		return get_fade_in (portgroup->crossfade_group->fade);
	}
	
	printf ("failed to get portgroup for crossfade!\n");
	exit (1);
	
}

void portgroup_apply_volume (portgroup_t *portgroup, int nframes) {

	int c, s, sign;

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


float read_peak (portgroup_t *portgroup, int channel) {

	float peak;
	
	peak = portgroup->peak[channel];
	portgroup->peak[channel] = 0.0f;

	return peak;
}

float read_stereo_peak (portgroup_t *portgroup) {

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

void compute_peak (portgroup_t *portgroup, int channel, uint32_t nframes) {

	int s;
	float sample;

	for(s = 0; s < nframes; s++) {
		sample = fabs(portgroup->samples[channel][s]);
		if (sample > portgroup->peak[channel]) {
			portgroup->peak[channel] = sample;
		}
	}
}


void portgroup_compute_peaks (portgroup_t *portgroup, uint32_t nframes) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		compute_peak (portgroup, c, nframes);
	}
}

void portgroup_clear_samples (portgroup_t *portgroup, uint32_t nframes) {

	int c;
	int s;

	for (c = 0; c < portgroup->channels; c++) {
		for (s = 0; s < nframes; s++) {
			portgroup->samples[c][s] = 0.0f;
		}
	}
}

void portgroup_update_samples (portgroup_t *portgroup, uint32_t nframes) {

	int c;

	if (portgroup->io_type == JACK) {
		for (c = 0; c < portgroup->channels; c++) {
			portgroup->samples[c] = jack_port_get_buffer (portgroup->ports[c], nframes);
		}
	}
}

void portgroup_mix_samples (portgroup_t *dest_portgroup, portgroup_t *src_portgroup, uint32_t nframes) {

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

void portgroup_copy_samples (portgroup_t *dest_portgroup, portgroup_t *src_portgroup, uint32_t nframes) {

	int c;
	int s;

	if (dest_portgroup->channels != src_portgroup->channels) {
		printf ("oh no channel count not equal!\n");
		exit (1);
	}

	for (c = 0; c < dest_portgroup->channels; c++) {
		for (s = 0; s < nframes; s++) {
			dest_portgroup->samples[c][s] = src_portgroup->samples[c][s];
		}
	}
}

void portgroup_hardlimit (portgroup_t *portgroup, uint32_t nframes) {

	int c;

	for (c = 0; c < portgroup->channels; c++) {	
		hardlimit (portgroup->samples[c], nframes);
	}
}

int krad_mixer_process (krad_mixer_t *krad_mixer, uint32_t nframes) {
	
	int p, m;

	portgroup_t *portgroup = NULL;
	portgroup_t *mixbus = NULL;
	
	// Gets input/output port buffers
	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && ((portgroup->direction == INPUT) || (portgroup->direction == OUTPUT))) {
			portgroup_update_samples (portgroup, nframes);
		}
	}
	
	// apply volume and calc peaks on inputs
	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
			portgroup_apply_volume (portgroup, nframes);
			portgroup_compute_peaks (portgroup, nframes);
		}
	}
	
	// Clear Mixes	
	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->io_type == MIXBUS)) {
			portgroup_clear_samples (portgroup, nframes);
		}
	}

	// Mix
	for (m = 0; m < PORTGROUP_MAX; m++) {
		mixbus = krad_mixer->portgroup[m];
		if ((mixbus != NULL) && (mixbus->active)) {
			for (p = 0; p < PORTGROUP_MAX; p++) {
				portgroup = krad_mixer->portgroup[p];
				if ((portgroup != NULL) && (portgroup->active) && (portgroup->mixbus == mixbus) && (portgroup->direction == INPUT)) {
					portgroup_mix_samples ( mixbus, portgroup, nframes );
				}
			}
		}
	}

	// copy to outputs, hardlimit all outputs
	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == OUTPUT)) {
			portgroup_copy_samples ( portgroup, portgroup->mixbus, nframes );
			portgroup_hardlimit ( portgroup, nframes );
		}
	}
	
	// deactivate ports that need to be deactivated
	for (p = 0; p < PORTGROUP_MAX; p++) {
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


portgroup_t *krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, char *sysname, int direction, int channels, mixbus_t *mixbus, portgroup_io_t io_type) {

	int p;
	int c;
	portgroup_t *portgroup;
	char portname[256];
	int port_direction;
	
	for (p = 0; p < PORTGROUP_MAX; p++) {
		if (krad_mixer->portgroup[p]->active == 0) {
			portgroup = krad_mixer->portgroup[p];
			break;
		}
	}

	portgroup->krad_mixer = krad_mixer;

	strcpy (portgroup->sysname, sysname);
	portgroup->channels = channels;
	portgroup->io_type = io_type;
	portgroup->mixbus = mixbus;
	portgroup->direction = direction;

	if (portgroup->direction == INPUT) {		
		port_direction = JackPortIsInput;
	} else {
		port_direction = JackPortIsOutput;
	}

	for (c = 0; c < portgroup->channels; c++) {
	
		portgroup->volume[c] = 90;
		portgroup->volume_actual[c] = (float)(portgroup->volume[c]/100.0f);
		portgroup->volume_actual[c] *= portgroup->volume_actual[c];
		portgroup->new_volume_actual[c] = portgroup->volume_actual[c];

		if (portgroup->io_type == MIXBUS) {
			portgroup->samples[c] = calloc (1, 16384);
		}

		switch ( portgroup->io_type ) {
			case MIXBUS:
				portgroup->samples[c] = calloc (1, 16384);
				break;
			case JACK:

				strcpy ( portname, sysname );
		
				if (portgroup->channels > 1) {
					strcat ( portname, "_" );
					strcat ( portname, krad_mixer_channel_number_to_string ( c ) );
				}
		
				portgroup->ports[c] = jack_port_register (krad_mixer->jack_client, portname, JACK_DEFAULT_AUDIO_TYPE, port_direction, 0);
	
				if (portgroup->ports[c] == NULL) {
					fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", portname);
					return NULL;
				}
				break;
			case KRAD_LINK:
				break;
		}
	}

	portgroup->active = 1;

	return portgroup;

}


void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, portgroup_t *portgroup) {

	int c;

	portgroup->active = 2;

	while (portgroup->active != 0) {
		usleep(15000);
	}

	printf("Krad Mixer: Removing %d channel Portgroup %s\n", portgroup->channels, portgroup->sysname);

	for (c = 0; c < portgroup->channels; c++) {
		switch ( portgroup->io_type ) {
			case MIXBUS:
				free ( portgroup->samples[c] );
				break;
			case JACK:
				printf("Krad Mixer: Disconnecting %s Port %d\n", portgroup->sysname, c);
				jack_port_disconnect(krad_mixer->jack_client, portgroup->ports[c]);
				printf("Krad Mixer: Unregistering %s Port %d\n", portgroup->sysname, c);
				jack_port_unregister(krad_mixer->jack_client, portgroup->ports[c]);
				break;
			case KRAD_LINK:
				break;
		}
	}
}

portgroup_t *krad_mixer_get_portgroup_from_sysname (krad_mixer_t *krad_mixer, char *sysname) {

	int p;
	portgroup_t *portgroup;

	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (strncmp(sysname, portgroup->sysname, strlen(sysname)) == 0) {	
				return portgroup;
			}
		}
	}

	return NULL;
}

void portgroup_set_crossfade (portgroup_t *portgroup, float value) {

	if (portgroup->crossfade_group != NULL) {

		portgroup->crossfade_group->fade = value;	

		portgroup_update_volume (portgroup->crossfade_group->portgroup[0]);
		portgroup_update_volume (portgroup->crossfade_group->portgroup[1]);	
	}
}

void crossfade_group_set_crossfade (crossfade_group_t *crossfade_group, float value) {

	crossfade_group->fade = value;	

	if ((crossfade_group->portgroup[0] != NULL) && (crossfade_group->portgroup[1] != NULL)) {
		portgroup_update_volume (crossfade_group->portgroup[0]);
		portgroup_update_volume (crossfade_group->portgroup[1]);	
	}
}

void portgroup_set_volume (portgroup_t *portgroup, float value) {

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

void portgroup_set_channel_volume (portgroup_t *portgroup, int channel, float value) {

	float volume_temp;

	portgroup->volume[channel] = value;
	volume_temp = (portgroup->volume[channel]/100.0f);
	volume_temp *= volume_temp;

	if (portgroup->crossfade_group != NULL) {
		volume_temp = volume_temp * portgroup_get_crossfade (portgroup);
	}

	portgroup->new_volume_actual[channel] = volume_temp;

}

void portgroup_update_volume (portgroup_t *portgroup) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		portgroup_set_channel_volume (portgroup, c, portgroup->volume[c]);
	}	
}		

int krad_mixer_set_portgroup_control (krad_mixer_t *krad_mixer, char *sysname, char *control, float value) {

	portgroup_t *portgroup;

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

	krad_mixer->shutdown = 1;

	printf("krad_mixer Shutting Down\n");
	
	//jack_client_close (krad_mixer->jack_client);
	//printf("krad_mixer Jack Client Closed\n");
	//pthread_cancel(krad_mixer->levels_thread);
	//pthread_cancel(krad_mixer->active_input_thread);
	
	free ( krad_mixer->crossfade_group );
	free ( krad_mixer );
	
}

krad_mixer_t *krad_mixer_create (char *sysname) {

	int p;
	mixbus_t *mixbus;
	char jack_client_name_string[48] = "";

	krad_mixer_t *krad_mixer;

	if ((krad_mixer = calloc (1, sizeof (krad_mixer_t))) == NULL) {
		fprintf (stderr, "mem alloc fail\n");
		exit (1);
	}
	
	//if ((krad_mixer->portgroup = calloc (PORTGROUP_MAX, sizeof (portgroup_t))) == NULL) {
	//		fprintf(stderr, "mem alloc fail\n");
	//			exit (1);
	//}
	
	for (p = 0; p < PORTGROUP_MAX; p++) {
		krad_mixer->portgroup[p] = calloc (1, sizeof (portgroup_t));
	}
	
	krad_mixer->crossfade_group = calloc (PORTGROUP_MAX / 2, sizeof (crossfade_group_t));
	
	strcpy (krad_mixer->sysname, sysname);
	
	strcat (jack_client_name_string, "krad_mixer_");
	strcat (jack_client_name_string, sysname);
	krad_mixer->jack_client_name = jack_client_name_string;

	krad_mixer->jack_server_name = NULL;
	krad_mixer->jack_options = JackNoStartServer;

	// Connect up to the JACK server 

	krad_mixer->jack_client = jack_client_open (krad_mixer->jack_client_name, krad_mixer->jack_options, &krad_mixer->jack_status, krad_mixer->jack_server_name);

	if (krad_mixer->jack_client == NULL) {
		fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", krad_mixer->jack_status);
		if (krad_mixer->jack_status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	
	if (krad_mixer->jack_status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}

	if (krad_mixer->jack_status & JackNameNotUnique) {
		krad_mixer->jack_client_name = jack_get_client_name (krad_mixer->jack_client);
		fprintf(stderr, "unique name `%s' assigned\n", krad_mixer->jack_client_name);
	}

	// Set up Callbacks

	jack_set_process_callback (krad_mixer->jack_client, krad_mixer_jack_callback, krad_mixer);
	//jack_on_shutdown (krad_mixer->jack_client, krad_mixer_jack_shutdown, krad_mixer);
	//jack_set_xrun_callback (krad_mixer->jack_client, krad_mixer_jack_xrun_callback, krad_mixer);

	// Activate

	if (jack_activate (krad_mixer->jack_client)) {
		printf("cannot activate client\n");
		exit (1);
	}


	portgroup_t *music1, *music2;

	mixbus = krad_mixer_portgroup_create (krad_mixer, "MainMix", MIX, 2, NULL, MIXBUS);

	music1 = krad_mixer_portgroup_create (krad_mixer, "Music1", INPUT, 2, mixbus, JACK);
	music2 = krad_mixer_portgroup_create (krad_mixer, "Music2", INPUT, 2, mixbus, JACK);
	krad_mixer_portgroup_create (krad_mixer, "Main", OUTPUT, 2, mixbus, JACK);	
	
	krad_mixer_crossfade_group_create (krad_mixer, music1, music2);
	
	return krad_mixer;
	
}


int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc ) {

	uint32_t command;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
//	uint64_t number;
	portgroup_t *portgroup;
	uint64_t element;
	
	uint64_t response;
//	uint64_t broadcast;
	
	int p;
	
	ebml_id = 0;
	
	char portname[1024];
	char controlname[1024];	
	float floatval;
	
	//i = 0;
	
	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {
	
		case EBML_ID_KRAD_MIXER_CMD_GET_CONTROL:
			printf("krad mixer handler! got get control\n");			
			//printf("GET CONTROL! %d\n", krad_radio_station->test_value);
			//krad_ipc_server_respond_number ( krad_radio_station->ipc, EBML_ID_KRAD_RADIO_CONTROL, krad_radio_station->test_value);
			return 1;
			break;	
		case EBML_ID_KRAD_MIXER_CMD_SET_CONTROL:
			printf("krad mixer handler! got set control\n");			

		
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
				printf("hrm wtf2\n");
			} else {
				//printf("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portname, ebml_data_size);
	
			printf("setcontrol portgroup %s\n", portname);
	
	
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
				printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, controlname, ebml_data_size);
	
			printf(" control -- %s\n", controlname);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
				printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			floatval = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
	
			printf("value %f\n", floatval);

			krad_mixer_set_portgroup_control (krad_mixer, portname, controlname, floatval);

			krad_ipc_server_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
			//*output_len = 666;
			return 2;
			break;	
		case EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS:

			printf("get PORTGROUPS list\n");

			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_MIXER_PORTGROUP_LIST, &element);
			
			for (p = 0; p < PORTGROUP_MAX; p++) {
				portgroup = krad_mixer->portgroup[p];
				if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
					krad_ipc_server_response_add_portgroup ( krad_ipc, portgroup->sysname, portgroup->channels,
											  				 portgroup->io_type, portgroup->volume[0],  portgroup->mixbus->sysname );
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );
			return 1;
			break;
	}

	return 0;

}

