#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>

#include <jack/jack.h>

#include "krad_mixer.h"

void connect_port (jack_client_t *client, char *port_one, char *port_two) {

	jack_port_t *port1;
	jack_port_t *port2;
	
	// Get the port we are connecting to
	port1 = jack_port_by_name(client, port_one);
	if (port1 == NULL) {
		fprintf(stderr, "Can't find port '%s'\n", port_one);
		return;
	}
	
	// Get the port we are connecting to
	port2 = jack_port_by_name(client, port_two);
	if (port2 == NULL) {
		fprintf(stderr, "Can't find port '%s'\n", port_two);
		return;
	}

	// Connect the port to our input port
	fprintf(stderr,"Connecting '%s' to '%s'...\n", jack_port_name(port1), jack_port_name(port2));
	if (jack_connect(client, jack_port_name(port1), jack_port_name(port2))) {
		fprintf(stderr, "Cannot connect port '%s' to '%s'\n", jack_port_name(port1), jack_port_name(port2));
		return;
	}
}

int krad_mixer_jack_xrun_callback (void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;

	krad_mixer->xruns++;
	
	printf("%s xrun number %d!\n", krad_mixer->client_name, krad_mixer->xruns);

	return 0;

}

/*
void *active_input_thread(void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	
	while (krad_mixer->ipc == NULL) {
		usleep(1000000);
	}
	
	while (krad_mixer->shutdown == 0) {
	
		if (krad_mixer->ipc->broadcast_clients) {
			
			if (krad_mixer->active_input != krad_mixer->last_active_input) {
				printf("Active Input: %d\n", krad_mixer->active_input);
				krad_mixer->active_input_bytes = sprintf(krad_mixer->active_input_data, "Active Input: %d\n", krad_mixer->active_input);
				krad_ipc_server_client_broadcast(krad_mixer->ipc, krad_mixer->active_input_data, krad_mixer->active_input_bytes, 1);
				krad_mixer->last_active_input = krad_mixer->active_input;
			}
		
			usleep(50000);
		
		} else {
		
			usleep(1000000);
		
		}
	
	}
		
}


void *levels_thread(void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	
	while (krad_mixer->ipc == NULL) {
		usleep(1000000);
	}
	
	while (krad_mixer->shutdown == 0) {
	
		if (krad_mixer->ipc->broadcast_clients_level[2]) {
		
			krad_mixer->level_bytes = 0;
			krad_mixer->level_bytes += sprintf(krad_mixer->level_data + krad_mixer->level_bytes, "krad_mixer_peaks:");
			for (krad_mixer->level_pg = 0; krad_mixer->level_pg < PORTGROUP_MAX; krad_mixer->level_pg++) {
				krad_mixer->level_portgroup = krad_mixer->portgroup[krad_mixer->level_pg];
				if ((krad_mixer->level_portgroup != NULL) && (krad_mixer->level_portgroup->active) && (krad_mixer->level_portgroup->direction == INPUT)) {
					krad_mixer->level_float_value = read_stereo_peak(krad_mixer->level_portgroup);
					krad_mixer->level_bytes += sprintf(krad_mixer->level_data + krad_mixer->level_bytes, "%s/%f*", krad_mixer->level_portgroup->name, krad_mixer->level_float_value);
				}
			}
			krad_ipc_server_client_broadcast(krad_mixer->ipc, krad_mixer->level_data, krad_mixer->level_bytes, 2);
		
			usleep(50000);
		
		} else {
		
			usleep(1000000);
		
		}
	
	}
		
}	

*/

float get_fade_in_amount (float crossfade_value, float in_fade_amount, float in_fade_pos) {

	const float total_samples = 200.0f;
	
	in_fade_pos = (crossfade_value + 1.0f) * 100.0f;
	in_fade_amount = cos(3.14159*0.5*(in_fade_pos + 0.5f)/total_samples);
	in_fade_amount = in_fade_amount * in_fade_amount;
	in_fade_amount = 1.0f - in_fade_amount;
	
	return in_fade_amount;
	
}

float get_fade_out_amount (float crossfade_value, float out_fade_amount, float out_fade_pos) {

	const float total_samples = 200.0f;

	out_fade_pos = (crossfade_value + 1.0f) * 100.0f;
	out_fade_amount = cos(3.14159*0.5*(out_fade_pos + 0.5f)/total_samples);
	out_fade_amount = out_fade_amount * out_fade_amount;
		
	return out_fade_amount;
		
}

void apply_volume (portgroup_t *portgroup, int nframes) {

	int c, s, sign;

	for (c = 0; c < 2; c++) {

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

	float tmp = portgroup->peak[channel];
	portgroup->peak[channel] = 0.0f;

	return tmp;
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

void compute_peak (portgroup_t *portgroup, int channel, int sample_count) {

	int i;
	

	for(i = 0; i < sample_count; i++) {
		const float s = fabs(portgroup->samples[channel][i]);
		if (s > portgroup->peak[channel]) {
			portgroup->peak[channel] = s;
		}
	}
}


void compute_peaks (portgroup_t *portgroup, int sample_count) {

	int i;
	
	for (i = 0; i < portgroup->channels; i++) {
		compute_peak (portgroup, i, sample_count);
	}
}

int krad_mixer_jack_callback (jack_nframes_t nframes, void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	
	krad_mixer_process ( krad_mixer, nframes );
	
	return 0;
	
}



int krad_mixer_process (krad_mixer_t *krad_mixer, uint32_t nframes) {
	
	int s, c, p, m;

	portgroup_t *portgroup = NULL;
	mixgroup_t *mixgroup = NULL;
	
	// Gets input/output port buffers and hardlimits inputs
	for (p = 0; p < PORTGROUP_MAX; p++) {
	
		portgroup = krad_mixer->portgroup[p];
	
		if ((portgroup != NULL) && (portgroup->active)) {

			if (portgroup->io_type == JACK) {
				for (c = 0; c < mixgroup->channels; c++) {
					portgroup->samples[c] = jack_port_get_buffer (portgroup->ports[c], nframes);
				}
			}
		
			if (portgroup->direction == INPUT) {
				apply_volume (portgroup, nframes);
			}

			compute_peaks(portgroup, nframes);
		}
	}

	// Clear Mixgroups
	for (m = 0; m < MIXGROUP_MAX; m++) {
	
		mixgroup = krad_mixer->mixgroup[m];
	
		if ((mixgroup != NULL) && (mixgroup->active)) {
			for (c = 0; c < mixgroup->channels; c++) {
				for (s = 0; s < nframes; s++) {
					portgroup->samples[c][s] = 0.0f;
				}
			}
		}
	}

		
	// hardlimit all outputs
	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (portgroup->direction == OUTPUT) {
				for (c = 0; c < mixgroup->channels; c++) {			
					hardlimit (portgroup->samples[c], nframes);
				}
			}
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


void krad_mixer_jack_shutdown (void *arg) {

	printf("Jack called shutdown..\n");
	//krad_mixer_want_shutdown();

}

void krad_mixer_jack_finish(void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	jack_client_close (krad_mixer->jack_client);
	
}

void krad_mixer_mixgroup_destroy (krad_mixer_t *krad_mixer, mixgroup_t *mixgroup) {
	
	int c;
	int p;
	
	for (p = 0; p < PORTGROUP_MAX; p++) {
		if ((krad_mixer->portgroup[p]->active == 1) && (krad_mixer->portgroup[p]->mixgroup == mixgroup)) {
			krad_mixer_portgroup_destroy (krad_mixer, krad_mixer->portgroup[p]);
		}
	}

	mixgroup->active = 2;
	
	while (mixgroup->active != 0) {
		usleep(30000);
	}
	
	for (c = 0; c < mixgroup->channels; c++) {
		free (mixgroup->samples[c]);
	}
	
	mixgroup->active = 0;
}


mixgroup_t *krad_mixer_mixgroup_create (krad_mixer_t *krad_mixer, const char *name, int channels) {

	int m;
	int c;
	mixgroup_t *mixgroup;
	
	for(m = 0; m < MIXGROUP_MAX; m++) {
		if (krad_mixer->portgroup[m]->active == 0) {
			mixgroup = krad_mixer->mixgroup[m];
			break;
		}
	}

	mixgroup->krad_mixer = krad_mixer;

	strcpy (mixgroup->name, name);
	mixgroup->channels = channels;
	
	
	for (c = 0; c < mixgroup->channels; c++) {
		mixgroup->samples[c] = calloc (1, 8192 * 4);
	}
	
	mixgroup->active = 1;
	
	return mixgroup;
	
}



void krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, const char *name, int direction, int channels, mixgroup_t *mixgroup, portgroup_io_t io_type) {

	int pg;
	int c;
	portgroup_t *portgroup;
	char portname[256];
	int port_direction;
	
	for(pg = 0; pg < PORTGROUP_MAX; pg++) {
		if (krad_mixer->portgroup[pg]->active == 0) {
			portgroup = krad_mixer->portgroup[pg];
			break;
		}
	}
	
	strcpy(portname, name);

	portgroup->krad_mixer = krad_mixer;

	strcpy (portgroup->name, name);
	portgroup->channels = channels;
	portgroup->io_type = io_type;
	portgroup->mixgroup = mixgroup;
	portgroup->direction = direction;

	if (portgroup->direction == INPUT) {		
		port_direction = JackPortIsInput;
	} else {
		port_direction = JackPortIsOutput;
	}

	for (c = 0; c < portgroup->channels; c++) {
	
		portgroup->volume[pg] = 90;

		portgroup->volume_actual[pg] = (float)(portgroup->volume[pg]/100.0f);
		portgroup->volume_actual[pg] *= portgroup->volume_actual[pg];
		portgroup->new_volume_actual[pg] = portgroup->volume_actual[pg];
	}

	strcpy (portname, name);
	

	strcat (portname, "_left");

	portgroup->ports[0] = jack_port_register (krad_mixer->jack_client, portname, JACK_DEFAULT_AUDIO_TYPE, port_direction, 0);
	
	if (portgroup->ports[0] == NULL) {
		fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", portname);
		return;
	}
	
	strcpy (portname, name);
	strcat (portname, "_right");

	portgroup->ports[1] = jack_port_register (krad_mixer->jack_client, portname, JACK_DEFAULT_AUDIO_TYPE, port_direction, 0);
	if (portgroup->ports[1] == NULL) {
		fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", portname);
		return;
	}



	portgroup->active = 1;

}


void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, portgroup_t *portgroup) {


	int c;

	portgroup->active = 2;
	
	while (portgroup->active != 0) {
		usleep(30000);
	}

	printf("Krad Mixer: Removing %d channel Portgroup %s\n", portgroup->channels, portgroup->name);

	for (c = 0; c < portgroup->channels; c++) {

		printf("Krad Mixer: Disconnecting %s Port %d\n", portgroup->name, c);
		jack_port_disconnect(krad_mixer->jack_client, portgroup->ports[c]);
		printf("Krad Mixer: Unregistering %s Port %d\n", portgroup->name, c);
		jack_port_unregister(krad_mixer->jack_client, portgroup->ports[c]);
	
	}

}

int krad_mixer_destroy_portgroup_by_name(krad_mixer_t *krad_mixer, char *name) {

	int pg;
	portgroup_t *portgroup;

	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
		portgroup = krad_mixer->portgroup[pg];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (strncmp(name, portgroup->name, strlen(name)) == 0) {	
				krad_mixer_portgroup_destroy (krad_mixer, portgroup);
				return 1;
			}
		}
	}
		
	return 0;
}


void listcontrols (krad_mixer_t *krad_mixer, char *data) {

	int pos;
	int p;
	portgroup_t *portgroup;

	pos = 0;

	pos += sprintf(data + pos, "krad_mixer_state:");

	for (p = 0; p < PORTGROUP_MAX; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if ((portgroup->direction == INPUT)) { 
				pos += sprintf(data + pos, "/mainmixer/%s/volume_left|%f|%f|%f*", portgroup->name, 0.0f, 100.0f, portgroup->volume[0]);
				pos += sprintf(data + pos, "/mainmixer/%s/volume_right|%f|%f|%f*", portgroup->name, 0.0f, 100.0f, portgroup->volume[1]);
			}
		}
	}

}


int setcontrol (krad_mixer_t *krad_mixer, char *data) {
/*
	int pg;
	portgroup_t *portgroup;

	temp = NULL;
	strtok_r(data, "/", &temp);

	unit  = strtok_r(NULL, "/", &temp);

	subunit = strtok_r(NULL, "/", &temp);

	control = strtok_r(NULL, "/", &temp);

	float_value = atof(strtok_r(NULL, "/", &temp));

	//printf("Unit: %s Subunit: %s Control: %s Value: %f\n", unit, subunit, control, float_value);
	
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if ((portgroup->active) && (strcmp(subunit, portgroup->name)) == 0) {
			
			//printf("I'm on %s\n",  portgroup->name);
			
			if ((strncmp(control, "volume", 6) == 0) || (strncmp(control, "volume_left", 11) == 0)) {
				portgroup->volume[0] = float_value;
				volume_temp = (float_value/100.0f);
				volume_temp *= volume_temp;
				//printf("Left Old Value: %f New Value %f\n", portgroup->volume_actual[0], volume_temp);
				portgroup->new_volume_actual[0] = volume_temp;
				//printf("Set Left Value: %f\n", portgroup->new_volume_actual[0]);
			
			
			}
			if ((strncmp(control, "volume", 6) == 0) || (strncmp(control, "volume_right", 12) == 0)) {
				portgroup->volume[1] = float_value;
				volume_temp = (float_value/100.0f);
				volume_temp *= volume_temp;
				//printf("Right Old Value: %f New Value %f\n", portgroup->volume_actual[1], volume_temp);
				portgroup->new_volume_actual[1] = volume_temp;
				//printf("Set Right Value: %f\n", portgroup->new_volume_actual[0]);
			
			}
			
			if (strncmp(control, "monitor", 7) == 0) {
			
				portgroup->monitor = float_value;
			
			}
			
			if (strncmp(control, "djeq_on", 7) == 0) {
			
				portgroup->djeq_on = float_value;
			
			}
			
			if (strncmp(control, "fastlimiter_release", 19) == 0) {	
				portgroup->fastlimiter->new_release = float_value;
			}
			
			if (strncmp(control, "fastlimiter_limit", 17) == 0) {	
				portgroup->fastlimiter->new_limit = float_value;
			}
			
			
			if (strncmp(control, "djeq_low", 8) == 0) {	
				portgroup->djeq->low = float_value;
			}
		
			if (strncmp(control, "djeq_mid", 8) == 0) {	
				portgroup->djeq->mid = float_value;
			}
			
			if (strncmp(control, "djeq_high", 9) == 0) {	
				portgroup->djeq->high = float_value;
			}
			
			if (strcmp(control, "digilogue_drive") == 0) {	
				krad_mixer->digilogue_left->drive = value;
				krad_mixer->digilogue_right->drive = value;
			}
			
			if (strcmp(control, "digilogue_blend") == 0) {	
				krad_mixer->digilogue_left->blend = value;
				krad_mixer->digilogue_right->blend = value;
			}
			
			if (strcmp(control, "highpass") == 0) {	
				krad_mixer->pass_left->high_freq = value;
				krad_mixer->pass_right->high_freq = value;
			}
			
			if (strcmp(control, "lowpass") == 0) {	
				krad_mixer->pass_left->low_freq = value;
				krad_mixer->pass_right->low_freq = value;
			}
			
			
			if (strncmp(control, "music_1-2_crossfader", 20) == 0) {	
				krad_mixer->music_1_2_crossfade = float_value;
				
				krad_mixer->new_music1_fade_value[0] = get_fade_out_amount(krad_mixer->music_1_2_crossfade, fade_temp, fade_temp_pos);
				krad_mixer->new_music1_fade_value[1] = krad_mixer->new_music1_fade_value[0];

				krad_mixer->new_music2_fade_value[0] = get_fade_in_amount(krad_mixer->music_1_2_crossfade, fade_temp, fade_temp_pos);
				krad_mixer->new_music2_fade_value[1] = krad_mixer->new_music2_fade_value[0];
				
				// temp ghetto way
				if (krad_mixer->music_1_2_crossfade >= 0.0f) {
					krad_mixer->active_input = 2;
				} else {
					krad_mixer->active_input = 1;
				}
			}
			
			if (strncmp(control, "music-aux_crossfader", 20) == 0) {
				krad_mixer->music_aux_crossfade = float_value;
				
				krad_mixer->new_music_fade_value[0] = get_fade_out_amount(krad_mixer->music_aux_crossfade, fade_temp, fade_temp_pos);
				krad_mixer->new_music_fade_value[1] = krad_mixer->new_music_fade_value[0];
				krad_mixer->new_aux_fade_value[0] = get_fade_in_amount(krad_mixer->music_aux_crossfade, fade_temp, fade_temp_pos);
				krad_mixer->new_aux_fade_value[1] = krad_mixer->new_aux_fade_value[0];
				
				
			}
			
			return 1;
		}
	
	}
	}
*/
	return 0;

}


void krad_mixer_destroy (krad_mixer_t *krad_mixer) {


	krad_mixer->shutdown = 1;

	printf("krad_mixer Shutting Down\n");
	//jack_client_close (krad_mixer->jack_client);
	//printf("krad_mixer Jack Client Closed\n");
	//pthread_cancel(krad_mixer->levels_thread);
	//pthread_cancel(krad_mixer->active_input_thread);
}

krad_mixer_t *krad_mixer_create (char *callsign) {

	int p;
	int m;
	mixgroup_t *mixgroup;
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
	
	for (m = 0; m < MIXGROUP_MAX; m++) {
		krad_mixer->mixgroup[m] = calloc (1, sizeof (mixgroup_t));
	}
	
	strcpy (krad_mixer->callsign, callsign);
	
	strcat (jack_client_name_string, "krad_mixer_");
	strcat (jack_client_name_string, callsign);
	krad_mixer->client_name = jack_client_name_string;

/*	
	krad_mixer->server_name = NULL;
	krad_mixer->options = JackNoStartServer;
	
	krad_mixer->userdata = krad_mixer;

	// Connect up to the JACK server 

	krad_mixer->jack_client = jack_client_open (krad_mixer->client_name, krad_mixer->options, &krad_mixer->status, krad_mixer->server_name);
	if (krad_mixer->jack_client == NULL) {
			fprintf(stderr, "jack_client_open() failed, "
			"status = 0x%2.0x\n", krad_mixer->status);
		if (krad_mixer->status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	
	if (krad_mixer->status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}

	if (krad_mixer->status & JackNameNotUnique) {
		krad_mixer->client_name = jack_get_client_name(krad_mixer->jack_client);
		fprintf(stderr, "unique name `%s' assigned\n", krad_mixer->client_name);
	}

	// Set up Callbacks

	jack_set_process_callback (krad_mixer->jack_client, jack_callback, krad_mixer->userdata);
	jack_on_shutdown (krad_mixer->jack_client, jack_shutdown, krad_mixer->userdata);

	jack_set_xrun_callback (krad_mixer->jack_client, xrun_callback, krad_mixer->userdata);

	// Activate

	if (jack_activate (krad_mixer->jack_client)) {
		printf("cannot activate client\n");
		exit (1);
	}
	*/
	
	
	/*
	
	krad_mixer_create_portgroup (krad_mixer, "main", OUTPUT, STEREO, MAINMIX, 0);
	krad_mixer_create_portgroup (krad_mixer, "music1", INPUT, STEREO, MUSIC, 0);
	krad_mixer_create_portgroup (krad_mixer, "music2", INPUT, STEREO, MUSIC, 0);
	
	sprintf(portname1, "%s:main_left", krad_mixer->client_name);
	sprintf(portname2, "kradlink_%s:flac_encoder_left", krad_mixer->callsign);
	connect_port(krad_mixer->jack_client, portname1, portname2);
	
	sprintf(portname1, "%s:main_right", krad_mixer->client_name);
	sprintf(portname2, "kradlink_%s:flac_encoder_right", krad_mixer->callsign);
	connect_port(krad_mixer->jack_client, portname1, portname2);
		
	
	//
	//create_portgroup(client, "aux1", INPUT, STEREO, AUX, 0);
	//create_portgroup(client, "submix1", OUTPUT, STEREO, SUBMIX, 1);
	//create_portgroup(client, "submix2", OUTPUT, STEREO, SUBMIX, 2);
	//create_portgroup(client, "mic1", INPUT, MONO, MIC, 1);
	//create_portgroup(client, "mic2", INPUT, MONO, MIC, 2);
	//

	
	*/

	//pthread_create(&krad_mixer->levels_thread, NULL, levels_thread, (void *)krad_mixer);
	//pthread_create(&krad_mixer->active_input_thread, NULL, active_input_thread, (void *)krad_mixer);

	mixgroup = krad_mixer_mixgroup_create (krad_mixer, "MainMix", 2);

	krad_mixer_portgroup_create (krad_mixer, "Music1", INPUT, 2, mixgroup, JACK);
	krad_mixer_portgroup_create (krad_mixer, "Music2", INPUT, 2, mixgroup, JACK);
	krad_mixer_portgroup_create (krad_mixer, "Main", OUTPUT, 2, mixgroup, JACK);	
	
	return krad_mixer;
	
}


int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc ) {

	uint32_t command;
	uint64_t ebml_data_size;
//	uint64_t number;
	
	uint64_t element;
	
	uint64_t response;
//	uint64_t broadcast;
	
	//int i;
	
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
			//number = krad_ipc_server_read_number ( krad_radio_station->ipc, ebml_data_size );
			//krad_radio_station->test_value = number;
			//printf("SET CONTROL to %d!\n", krad_radio_station->test_value);
			//krad_ipc_server_broadcast_number ( krad_radio_station->ipc, EBML_ID_KRAD_RADIO_CONTROL, krad_radio_station->test_value);
			//*output_len = 666;
			return 2;
			break;	
		case EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS:
			printf("get PORTGROUPS list\n");
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_MIXER_PORTGROUP_LIST, &element);
			
			//while (krad_tags_get_next_tag ( krad_radio_station->krad_tags, &i, &tag_name, &tag_value)) {
			//	krad_ipc_server_response_add_tag ( krad_radio_station->ipc, tag_name, tag_value);
			//	printf("got PORTGROUP %d: %s to %s\n", i, tag_name, tag_value);
			//}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element);
			krad_ipc_server_response_finish ( krad_ipc, response);
			return 1;
			break;
	}
	
	
	


/*

	//printf("Krad Mixer Daemon Handler got %zu byte message: '%s' \n", strlen(data), data);
	
	//reset things from last command
	temp = NULL;
	strcpy(path, "");
	strcpy(path2, "");
	strcpy(path3, "");
	strcpy(path4, "");
	strcpy(path5, "");
	strcpy(datacpy, data);
	cmd = strtok_r(datacpy, ",", &temp);
	
	
	//if (strncmp(cmd, "broadcasts", 10) == 0) {
	if (strncmp(cmd, "getpeak", 7) == 0) {
		krad_ipc_server_set_client_broadcasts(krad_mixer->ipc, client, 2);
		printf("client asked for broadcasts\n");
		return 0; // to not broadcast
	}
	
	if (strncmp(cmd, "listcontrols", 12) == 0) {
		printf("Listing Controls\n");
		listcontrols(client, data);
		return 0; // to not broadcast
	}

	if (strncmp(cmd, "xruns", 5) == 0) {
		printf("Number of xruns: %d\n", krad_mixer->xruns);
		sprintf(data, "Number of xruns: %d\n", krad_mixer->xruns);
		return 0; // to not broadcast
	}
	
	if (strncmp(cmd, "get_active_input", 16) == 0) {
		printf("Active Input: %d\n", krad_mixer->active_input);
		sprintf(data, "Active Input: %d\n", krad_mixer->active_input);
		return 0; // to not broadcast
	}


	if (strncmp(cmd, "setcontrol", 9) == 0) {
		if (setcontrol(client, datacpy)) {
			sprintf(data, "krad_mixer:setcontrol/mainmixer/%s/%s/%f", subunit, control, float_value);
			//printf("data: %s\n", data);
			return 1; // to broadcast
		} else {
			sprintf(data, "krad_mixer:setcontrol/failed");
			//printf("data: %s\n", data);
			return 0;
		}
	}

	if (strncmp(cmd, "create_portgroup", 16) == 0) {
		strcat(path, strtok_r(NULL, ",", &temp));
		//strcat(path2, strtok_r(NULL, ",", &temp));
		//strcat(path3, strtok_r(NULL, ",", &temp));
		//strcat(path4, strtok_r(NULL, ",", &temp));
		
		if (strncmp(path, "sampler", 7) == 0) {
			sprintf(path5, "sampler");
			create_portgroup(client, path5, INPUT, STEREO, AUX, 0);
		}
		
		if (strncmp(path, "synth", 5) == 0) {
			sprintf(path5, "synth");
			create_portgroup(client, path5, INPUT, STEREO, AUX, 0);
		}
		
		sprintf(data, "krad_mixer:portgroup/add/%s", path5);
		return 2;
	}

	if (strncmp(cmd, "create_advanced_portgroup", 25) == 0) {
		strcat(path, strtok_r(NULL, ",", &temp));
		strcat(path2, strtok_r(NULL, ",", &temp));
		strcat(path3, strtok_r(NULL, ",", &temp));
		//strcat(path4, strtok_r(NULL, ",", &temp));


		if (strncmp(path3, "MIC", 3) == 0) {
		
			krad_mixer->portgroup_count++;
		
			sprintf(path5, "submix_%s_out_for_%s", path2, path);
			create_portgroup(client, path5, OUTPUT, STEREO, SUBMIX, krad_mixer->portgroup_count);
			sprintf(path5, "mic_%s_in_from_%s", path2, path);
			create_portgroup(client, path5, INPUT, MONO, MIC, krad_mixer->portgroup_count);
		
		}
		
		if (strncmp(path3, "STEREOMIC", 9) == 0) {
		
			krad_mixer->portgroup_count++;

			sprintf(path5, "submix_%s_out_for_%s", path2, path);
			create_portgroup(client, path5, OUTPUT, STEREO, SUBMIX, krad_mixer->portgroup_count);
			sprintf(path5, "stereomic_%s_in_from_%s", path2, path);
			create_portgroup(client, path5, INPUT, STEREO, STEREOMIC, krad_mixer->portgroup_count);
		
		}
		
		if (strncmp(path3, "AUX", 3) == 0) {
			sprintf(path5, "aux_%s_in_from_%s", path2, path);
			create_portgroup(client, path5, INPUT, STEREO, AUX, 0);
		}

		sprintf(data, "krad_mixer:portgroup/add/%s", path5);
		return 2;
	}
	
	if (strncmp(cmd, "destroy_advanced_portgroup", 26) == 0) {
		strcat(path, strtok_r(NULL, ",", &temp));
		strcat(path2, strtok_r(NULL, ",", &temp));
		strcat(path3, strtok_r(NULL, ",", &temp));
		//strcat(path4, strtok_r(NULL, ",", &temp));


		if (strncmp(path3, "MIC", 3) == 0) {
		
			sprintf(path5, "submix_%s_out_for_%s", path2, path);
			destroy_portgroup_by_name(client, path5);
			sprintf(path5, "mic_%s_in_from_%s", path2, path);
			destroy_portgroup_by_name(client, path5);
		
		}
		
		if (strncmp(path3, "STEREOMIC", 9) == 0) {

			sprintf(path5, "submix_%s_out_for_%s", path2, path);
			destroy_portgroup_by_name(client, path5);
			sprintf(path5, "stereomic_%s_in_from_%s", path2, path);
			destroy_portgroup_by_name(client, path5);
		
		}
		
		if (strncmp(path3, "AUX", 3) == 0) {
			sprintf(path5, "aux_%s_in_from_%s", path2, path);
			destroy_portgroup_by_name(client, path5);
		}

		sprintf(data, "krad_mixer:portgroup/remove/%s", path5);
		return 2;
	}
	
	if (strncmp(cmd, "destroy_portgroup", 17) == 0) {
		strcat(path, strtok_r(NULL, ",", &temp));
		
		destroy_portgroup_by_name(client, path);
		sprintf(data, "krad_mixer:portgroup/remove/%s", path);
		return 2;

	}


	if (strncmp(cmd, "kill", 4) == 0) {
		printf("Killed by a client..\n");
		krad_mixer_want_shutdown();
		sprintf(data, "krad_mixer shutting down now!");
		return 1; // to broadcast
	}
*/

	return 0;

}






