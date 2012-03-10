#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>

#include <jack/jack.h>

#include "krad_mixer.h"

void connect_port(jack_client_t *client, char *port_one, char *port_two)
{
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

int xrun_callback(void *arg)
{

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;

	krad_mixer->xruns++;
	
	printf("%s xrun number %d!\n", krad_mixer->client_name, krad_mixer->xruns);


	return 0;

}


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

float get_fade_in_amount(float crossfade_value, float in_fade_amount, float in_fade_pos) {

	const float total_samples = 200.0f;
	
	in_fade_pos = (crossfade_value + 1.0f) * 100.0f;
	in_fade_amount = cos(3.14159*0.5*(in_fade_pos + 0.5f)/total_samples);
	in_fade_amount = in_fade_amount * in_fade_amount;
	in_fade_amount = 1.0f - in_fade_amount;
	
	return in_fade_amount;
	
}

float get_fade_out_amount(float crossfade_value, float out_fade_amount, float out_fade_pos) {

	const float total_samples = 200.0f;

	out_fade_pos = (crossfade_value + 1.0f) * 100.0f;
	out_fade_amount = cos(3.14159*0.5*(out_fade_pos + 0.5f)/total_samples);
	out_fade_amount = out_fade_amount * out_fade_amount;
		
	return out_fade_amount;
		
}

void apply_volume(portgroup_t *portgroup, int nframes) {

	int c, s, sign;
	
	int no_fade_change[8];

	if (portgroup->type == MIC) {
		
		for (s = 0; s < nframes; s++) {
			// Turn into stereo, right channel first so the left volume doensnt control both
			portgroup->samples[1][s] = portgroup->samples[0][s] * portgroup->volume_actual[1];
			portgroup->samples[0][s] = portgroup->samples[0][s] * portgroup->volume_actual[0];

		}
	
	
	} else {	// 2 channel
		for (c = 0; c < 2; c++) {
		
			no_fade_change[c] = 1;
			
			if (portgroup->type == MUSIC) {
				if (strncmp(portgroup->name, "music1", 6) == 0) {
					if (portgroup->krad_mixer->music1_fade_value[c] != portgroup->krad_mixer->new_music1_fade_value[c]) {
						no_fade_change[c] = 0;
					}
				}
				if (strncmp(portgroup->name, "music2", 6) == 0) {
					if (portgroup->krad_mixer->music2_fade_value[c] != portgroup->krad_mixer->new_music2_fade_value[c]) {
						no_fade_change[c] = 0;
					}
				}
					
				if (portgroup->music_fade_value[c] != portgroup->krad_mixer->new_music_fade_value[c]) {
					no_fade_change[c] = 0;
				}
				
			}
			
			if (portgroup->type == AUX) {
				if (portgroup->aux_fade_value[c] != portgroup->krad_mixer->new_aux_fade_value[c]) {
					no_fade_change[c] = 0;
				}
			}
		
		
			if ((portgroup->new_volume_actual[c] == portgroup->volume_actual[c]) && (no_fade_change[c])) {
				for (s = 0; s < nframes; s++) {
					portgroup->samples[c][s] = portgroup->samples[c][s] * portgroup->final_volume_actual[c];
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
							
							
							
							
							if (portgroup->type == MUSIC) {
							
								portgroup->music_fade_value[c] = portgroup->krad_mixer->new_music_fade_value[c];
							
								if (strncmp(portgroup->name, "music1", 6) == 0) {
									portgroup->krad_mixer->music1_fade_value[c] = portgroup->krad_mixer->new_music1_fade_value[c];
									portgroup->final_fade_actual[c] = portgroup->music_fade_value[c] * portgroup->krad_mixer->music1_fade_value[c];	
								}
								
								if (strncmp(portgroup->name, "music2", 6) == 0) {
									portgroup->krad_mixer->music2_fade_value[c] = portgroup->krad_mixer->new_music2_fade_value[c];
									portgroup->final_fade_actual[c] = portgroup->music_fade_value[c] * portgroup->krad_mixer->music2_fade_value[c];	
								}
							}
			
							if (portgroup->type == AUX) {
								portgroup->aux_fade_value[c] = portgroup->krad_mixer->new_aux_fade_value[c];
								portgroup->final_fade_actual[c] = portgroup->aux_fade_value[c];
							}	
							
							
							portgroup->final_volume_actual[c] = portgroup->volume_actual[c] * portgroup->final_fade_actual[c];
							
							
							portgroup->last_sign[c] = 0;
						}
					}
						
					portgroup->samples[c][s] = (portgroup->samples[c][s] * portgroup->final_volume_actual[c]);
		
				}

				if (portgroup->last_sign[c] != 0) {
					portgroup->last_sign[c] = sign;
				}
			}
		}
	}

}


float read_peak(portgroup_t *portgroup, int channel)
{
	float tmp = portgroup->peak[channel];
	portgroup->peak[channel] = 0.0f;

	return tmp;
}

float read_stereo_peak(portgroup_t *portgroup)
{
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

void compute_peaks(portgroup_t *portgroup, int sample_count) {

	int i;
	
	for (i = 0; i < portgroup->channels; i++) {
		compute_peak(portgroup, i, sample_count);
	}
}

void compute_peak(portgroup_t *portgroup, int channel, int sample_count) {

	int i;
	

	for(i = 0; i < sample_count; i++) {
		const float s = fabs(portgroup->samples[channel][i]);
		if (s > portgroup->peak[channel]) {
			portgroup->peak[channel] = s;
		}
	}
}


int jack_callback (jack_nframes_t nframes, void *arg)
{

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	
	int x, j, ret, cycle, i, i2, pg, pg2, s;

	portgroup_t *portgroup = NULL;
	portgroup_t *portgroup2 = NULL;
	
	/*
	
	cur_sample = cur_sample + nframes;

	if (cur_sample == 4096) {
		kiss_fft( cfg , inf , outf );
		cur_sample = 0;
	}
	
	*/
	
	
	
	// clear mic mix
	for (i = 0; i < nframes; i++) {
		krad_mixer->micmix_left[i] = 0.0f;
		krad_mixer->micmix_right[i] = 0.0f;
	}

	// Gets input/output port buffers and hardlimits inputs
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
	
		if (portgroup->type == MIC) {
			portgroup->samples[0] = jack_port_get_buffer (portgroup->input_port, nframes);
			
			apply_volume(portgroup, nframes);
			//hardlimit(portgroup->samples[0], nframes);
			compute_peaks(portgroup, nframes);
			
		} else {
			if (portgroup->direction == INPUT) {
				portgroup->samples[0] = jack_port_get_buffer (portgroup->input_ports[0], nframes);
				portgroup->samples[1] = jack_port_get_buffer (portgroup->input_ports[1], nframes);
				
				apply_volume(portgroup, nframes);
				if (portgroup->djeq_on) {
					djeq_run(portgroup->djeq, portgroup->samples[0], portgroup->samples[1], portgroup->samples[0], portgroup->samples[1], nframes);
				}
				//hardlimit(portgroup->samples[0], nframes);
				//hardlimit(portgroup->samples[1], nframes);
				compute_peaks(portgroup, nframes);
				
				
			} else {
				portgroup->samples[0] = jack_port_get_buffer (portgroup->output_ports[0], nframes);
				portgroup->samples[1] = jack_port_get_buffer (portgroup->output_ports[1], nframes);
			}
		}
	
	}
	}

	// Clears Submix output ports
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->type == SUBMIX) {
			for (x = 0;x < nframes; x++) {
				portgroup->samples[0][x] = 0.0f;
				portgroup->samples[1][x] = 0.0f;
			}
		}	
	}
	}


	// creates mix of all mics, and puts all mics not in the same group as a submix into each submix
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
	
		if ((portgroup->type == MIC) || (portgroup->type == STEREOMIC)) {
	
			for (x = 0;x < nframes; x++) {
				krad_mixer->micmix_left[x] += portgroup->samples[0][x];
				krad_mixer->micmix_right[x] += portgroup->samples[1][x];
			}
			
			//for (portgroup2 = krad_mixer->portgroup; portgroup2 != NULL && portgroup2->active; portgroup2 = portgroup2->next) {
			for (pg2 = 0; pg2 < PORTGROUP_MAX; pg2++) {
			portgroup2 = krad_mixer->portgroup[pg2];
			if ((portgroup2 != NULL) && (portgroup2->active)) {
				if (portgroup2->type == SUBMIX) {
					if ((portgroup->group != portgroup2->group) || (portgroup2->monitor == 1)) {
						for (x = 0;x < nframes; x++) {
							portgroup2->samples[0][x] += portgroup->samples[0][x];
							portgroup2->samples[1][x] += portgroup->samples[1][x];  // mic is mono
						}
					}
				}	
			}
			}
		}

	}
	}	
		
	int gotmusic1 = 0;
	int gotmusic2 = 0;
	
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->type == MUSIC) {
			gotmusic1 = 1;
			break;
		}	
	}
	}
	
	if (gotmusic1) {
		for (pg2 = 0; pg2 < PORTGROUP_MAX; pg2++) {
		portgroup2 = krad_mixer->portgroup[pg2];
		if ((portgroup2 != NULL) && (portgroup2->active)) {
			if ((portgroup2->type == MUSIC) && (portgroup2 != portgroup)) {
				gotmusic2 = 1;
				break;
			}	
		}
		}
	
		if ((gotmusic1) && (gotmusic1)) {
	
			//add music portgroups together
			for (s = 0; s < nframes; s++) {
					portgroup->samples[0][s] = portgroup->samples[0][s] + portgroup2->samples[0][s];
					portgroup->samples[1][s] = portgroup->samples[1][s] + portgroup2->samples[1][s];
			}
		}
	}
	


	//add aux inputs, takes input from aux portgroup and adds it to the music ports

	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->type == AUX) {
			//for (portgroup2 = krad_mixer->portgroup; portgroup2 != NULL && portgroup2->active; portgroup2 = portgroup2->next) {
			for (pg2 = 0; pg2 < PORTGROUP_MAX; pg2++) {
			portgroup2 = krad_mixer->portgroup[pg2];
			if ((portgroup2 != NULL) && (portgroup2->active)) {
				if (portgroup2->type == MUSIC) {
					for (x = 0; x < nframes; x++) {
						portgroup2->samples[0][x] += portgroup->samples[0][x];
						portgroup2->samples[1][x] += portgroup->samples[1][x];
					}
					// since all of the music gets put into the first music portgroup
					break;
				}
			}
			}
		}
	}
	}
	
	
	//compress music from mics, takes input from music portgroup and puts it in the mainmix output ports

	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->type == MUSIC) {
			//for (portgroup2 = krad_mixer->portgroup; portgroup2 != NULL && portgroup2->active; portgroup2 = portgroup2->next) {
			for (pg2 = 0; pg2 < PORTGROUP_MAX; pg2++) {
			portgroup2 = krad_mixer->portgroup[pg2];
			if ((portgroup2 != NULL) && (portgroup2->active)) {
				if (portgroup2->type == MAINMIX) {
					sc2_run(krad_mixer->sc2_data, portgroup->samples[0], krad_mixer->micmix_left, portgroup2->samples[0], nframes);
					sc2_run(krad_mixer->sc2_data2, portgroup->samples[1], krad_mixer->micmix_right, portgroup2->samples[1], nframes);
				}
			}
			}
		// since all of the music gets put into the first music portgroup	
		break;
		}
	}
	}
	
	// add music to submixes
	//for (portgroup2 = krad_mixer->portgroup; portgroup2 != NULL && portgroup2->active; portgroup2 = portgroup2->next) {
	for (pg2 = 0; pg2 < PORTGROUP_MAX; pg2++) {
	portgroup2 = krad_mixer->portgroup[pg2];
	if ((portgroup2 != NULL) && (portgroup2->active)) {
		if (portgroup2->type == SUBMIX) {
			//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
			for (pg = 0; pg < PORTGROUP_MAX; pg++) {
			portgroup = krad_mixer->portgroup[pg];
			if ((portgroup != NULL) && (portgroup->active)) {
				// This actually adds the current mainmix to the submixes, which is the music after being compressed by all mics
				if (portgroup->type == MAINMIX) {
					for (x = 0; x < nframes; x++) {
						portgroup2->samples[0][x] += portgroup->samples[0][x];
						portgroup2->samples[1][x] += portgroup->samples[1][x];
					}
				}
			}
			}	
		}
	}
	}
	
	// add mics to mainmix

	//for (portgroup2 = krad_mixer->portgroup; portgroup2 != NULL && portgroup2->active; portgroup2 = portgroup2->next) {
	for (pg2 = 0; pg2 < PORTGROUP_MAX; pg2++) {
	portgroup2 = krad_mixer->portgroup[pg2];
	if ((portgroup2 != NULL) && (portgroup2->active)) {
		if (portgroup2->type == MAINMIX) {
			for (x = 0; x < nframes; x++) {
				portgroup2->samples[0][x] += krad_mixer->micmix_left[x];
 				portgroup2->samples[1][x] += krad_mixer->micmix_right[x];
			}
		}
	}
	}

	
	// effects test
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	//	if (portgroup->type == MAINMIX) {
	//		pass_run(krad_mixer->pass_left, portgroup->samples[0], portgroup->samples[0], nframes);
	//		pass_run(krad_mixer->pass_right, portgroup->samples[1], portgroup->samples[1], nframes);
	//		digilogue_run(krad_mixer->digilogue_left, portgroup->samples[0], portgroup->samples[0], nframes);
	//		digilogue_run(krad_mixer->digilogue_right, portgroup->samples[1], portgroup->samples[1], nframes);
	//		djeq_run(krad_mixer->djeq, portgroup->samples[0], portgroup->samples[1], portgroup->samples[0], portgroup->samples[1], nframes);
	//	}
	//}	


	// fastlimit mainmix
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if ((portgroup->direction == OUTPUT) && (portgroup->type == MAINMIX)) {
			fastlimiter_run(portgroup->fastlimiter, portgroup->samples[0], portgroup->samples[1], portgroup->samples[0], portgroup->samples[1], nframes);
		}
	}
	}
		
	// hardlimit all outputs
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->direction == OUTPUT) {
			hardlimit(portgroup->samples[0], nframes);
			hardlimit(portgroup->samples[1], nframes);
		}
	}
	}
	
	// deactivate ports that need to be deactivated
	//for (portgroup = krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = krad_mixer->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->active == 2) {
			portgroup->active = 0;
		}
	}
	}
		

	return 0;      

}


void jack_shutdown (void *arg)
{
	printf("Jack called shutdown..\n");
	krad_mixer_want_shutdown();

}

void jack_finish(void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	jack_client_close (krad_mixer->jack_client);
	
}




void create_portgroup(krad_mixer_client_t *client, const char *name, int direction, int channels, int type, int group) {

	
	for(client->pg = 0; client->pg < PORTGROUP_MAX; client->pg++) {
		if (client->krad_mixer->portgroup[client->pg]->active == 0) {
			client->portgroup = client->krad_mixer->portgroup[client->pg];
			if (client->pg < PORTGROUP_MAX - 1) { client->portgroup->next = client->krad_mixer->portgroup[client->pg + 1]; }
			break;
		}
	}
	
	strcpy(client->portname, name);

	client->portgroup->krad_mixer = client->krad_mixer;

	strcpy(client->portgroup->name, name);
	client->portgroup->channels = channels;
	client->portgroup->type = type;
	client->portgroup->group = group;
	client->portgroup->direction = direction;

	client->portgroup->monitor = 0;
	
	client->portgroup->djeq_on = 1;
	client->portgroup->djeq = djeq_create();
	client->portgroup->fastlimiter = fastlimiter_create();

	for(client->pg = 0; client->pg < 8; client->pg++) {
	
		// temp default volume
		client->portgroup->volume[client->pg] = 90;
	
	
		client->portgroup->volume_actual[client->pg] = (float)(client->portgroup->volume[client->pg]/100.0f);
		client->portgroup->volume_actual[client->pg] *= client->portgroup->volume_actual[client->pg];
		client->portgroup->new_volume_actual[client->pg] = client->portgroup->volume_actual[client->pg];
		client->portgroup->final_volume_actual[client->pg] = client->portgroup->volume_actual[client->pg];
		
		if (strncmp(client->portgroup->name, "music1", 6) == 0) {
			client->portgroup->final_fade_actual[client->pg] = 1.0f;
		}
		if (strncmp(client->portgroup->name, "music2", 6) == 0) {
			client->portgroup->final_fade_actual[client->pg] = 0.0f;
		}
		if (client->portgroup->type == AUX) {
			client->portgroup->final_fade_actual[client->pg] = 0.0f;
		}


	}

	strcpy(client->portname, name);


	if (client->portgroup->direction == INPUT) {


		

		if (client->portgroup->channels == MONO) {
			client->portgroup->input_port = jack_port_register (client->krad_mixer->jack_client, client->portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsInput, 0);

			client->portgroup->input_ports[0] = client->portgroup->input_port;

			if (client->portgroup->input_port == NULL) {
				fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", client->portname);
				return;
			}
			
			//Allocate second buffer for turning mono into stereo since jack will give us one per port
			//FIXME free this? we dun use mono yet btw..
			client->portgroup->samples[1] = malloc(2048 * sizeof(float));
			
		} else {
		
			strcat(client->portname, "_left");

			client->portgroup->input_ports[0] = jack_port_register (client->krad_mixer->jack_client, client->portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsInput, 0);
			if (client->portgroup->input_ports[0] == NULL) {
				fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", client->portname);
				return;
			}
			
			strcpy(client->portname, name);
			strcat(client->portname, "_right");

			client->portgroup->input_ports[1] = jack_port_register (client->krad_mixer->jack_client, client->portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsInput, 0);
			if (client->portgroup->input_ports[1] == NULL) {
				fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", client->portname);
				return;
			}
																					  
		
		}

	} else {
	
	
		strcat(client->portname, "_left");

		client->portgroup->output_ports[0] = jack_port_register (client->krad_mixer->jack_client, client->portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsOutput, 0);

		if (client->portgroup->output_ports[0] == NULL) {
			fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", client->portname);
			return;
		}

		strcpy(client->portname, name);
		strcat(client->portname, "_right");

		client->portgroup->output_ports[1] = jack_port_register (client->krad_mixer->jack_client, client->portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsOutput, 0);
																					  
		if (client->portgroup->output_ports[1] == NULL) {
			fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", client->portname);
			return;
		}


	}


	client->portgroup->active = 1;


}


void destroy_portgroup (krad_mixer_t *krad_mixer, portgroup_t *portgroup) {


	portgroup->active = 2;
	
	while (portgroup->active != 0) {
		usleep(50000);
	}

	if (portgroup->direction == INPUT) {

		if (portgroup->channels == MONO) {

			printf("Krad Mixer Removing Mono Input Portgroup %s\n", portgroup->name);

			printf("Krad Mixer Disconnecting Input Port\n");
			jack_port_disconnect(krad_mixer->jack_client, portgroup->input_port);
			printf("Krad Mixer Unregistering Input port\n");
			jack_port_unregister(krad_mixer->jack_client, portgroup->input_port);
	
		} else {
	
			printf("Krad Mixer Removing Stereo Input Portgroup %s\n", portgroup->name);

			printf("Krad Mixer Disconnecting Left Input Port\n");
			jack_port_disconnect(krad_mixer->jack_client, portgroup->input_ports[0]);
			printf("Krad Mixer Unregistering Left Input port\n");
			jack_port_unregister(krad_mixer->jack_client, portgroup->input_ports[0]);
			
			printf("Krad Mixer Disconnecting Right Input Port\n");
			jack_port_disconnect(krad_mixer->jack_client, portgroup->input_ports[1]);
			printf("Krad Mixer Unregistering Right Input port\n");
			jack_port_unregister(krad_mixer->jack_client, portgroup->input_ports[1]);
	
		}
	
	
	} else {
	
		printf("Krad Mixer Removing Output Portgroup %s\n", portgroup->name);
		
		printf("Krad Mixer Disconnecting Left Output Port\n");
		jack_port_disconnect(krad_mixer->jack_client, portgroup->output_ports[0]);
		printf("Krad Mixer Unregistering Left Output port\n");
		jack_port_unregister(krad_mixer->jack_client, portgroup->output_ports[0]);
			
		printf("Krad Mixer Disconnecting Right Output Port\n");
		jack_port_disconnect(krad_mixer->jack_client, portgroup->output_ports[1]);
		printf("Krad Mixer Unregistering Right Output port\n");
		jack_port_unregister(krad_mixer->jack_client, portgroup->output_ports[1]);	

	
	}
	
	fastlimiter_destroy(portgroup->fastlimiter);
	djeq_destroy(portgroup->djeq);

}

int destroy_portgroup_by_name(krad_mixer_client_t *client, char *name) {

	for (client->pg = 0; client->pg < PORTGROUP_MAX; client->pg++) {
		client->portgroup = client->krad_mixer->portgroup[client->pg];
		if ((client->portgroup != NULL) && (client->portgroup->active)) {
			if (strncmp(name, client->portgroup->name, strlen(name)) == 0) {	
				destroy_portgroup(client->krad_mixer, client->portgroup);
				return 1;
			}
		}
	}
		
	return 0;
		
}

krad_mixer_t *krad_mixer_setup(char *jack_name_suffix) {

	// Setup our vars

	char jack_client_name_string[48] = "";

	krad_mixer_t *krad_mixer;

	if ((krad_mixer = calloc (1, sizeof (krad_mixer_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
	//if ((krad_mixer->portgroup = calloc (PORTGROUP_MAX, sizeof (portgroup_t))) == NULL) {
	//		fprintf(stderr, "mem alloc fail\n");
	//			exit (1);
	//}
	
	int i;
	
	for (i = 0; i < PORTGROUP_MAX; i++) {
		krad_mixer->portgroup[i] = calloc (1, sizeof (portgroup_t));
	}
	
	strcpy(krad_mixer->station, jack_name_suffix);
	
	krad_mixer->sc2_data = sc2_init(krad_mixer->sc2_data);
	krad_mixer->sc2_data2 = sc2_init(krad_mixer->sc2_data2);
	
	//krad_mixer->djeq = djeq_create(krad_mixer->djeq);
	//krad_mixer->digilogue_left = digilogue_create(krad_mixer->digilogue_left);
	//krad_mixer->digilogue_right = digilogue_create(krad_mixer->digilogue_right);
	
	//krad_mixer->pass_left = pass_create(krad_mixer->pass_left);
	//krad_mixer->pass_right = pass_create(krad_mixer->pass_right);
	
	krad_mixer->music_1_2_crossfade = -1.0f;
	krad_mixer->music_aux_crossfade = -1.0f;
	
	krad_mixer->new_music_fade_value[0] = get_fade_out_amount(krad_mixer->music_aux_crossfade, krad_mixer->fade_temp, krad_mixer->fade_temp_pos);
	krad_mixer->new_music_fade_value[1] = krad_mixer->new_music_fade_value[0];
	krad_mixer->new_aux_fade_value[0] = get_fade_in_amount(krad_mixer->music_aux_crossfade, krad_mixer->fade_temp, krad_mixer->fade_temp_pos);
	krad_mixer->new_aux_fade_value[1] = krad_mixer->new_aux_fade_value[0];
	
	krad_mixer->new_music1_fade_value[0] = get_fade_out_amount(krad_mixer->music_1_2_crossfade, krad_mixer->fade_temp, krad_mixer->fade_temp_pos);
	krad_mixer->new_music1_fade_value[1] = krad_mixer->new_music1_fade_value[0];

	krad_mixer->new_music2_fade_value[0] = get_fade_in_amount(krad_mixer->music_1_2_crossfade, krad_mixer->fade_temp, krad_mixer->fade_temp_pos);
	krad_mixer->new_music2_fade_value[1] = krad_mixer->new_music2_fade_value[0];
	
	krad_mixer->active_input = 1;
	
	if (jack_name_suffix != NULL) {
		strcat(jack_client_name_string, "krad_mixer_");
		strcat(jack_client_name_string, jack_name_suffix);
		krad_mixer->client_name = jack_client_name_string;
	} else {
		krad_mixer->client_name = "krad_mixer";
	}
	
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
	

	pthread_create(&krad_mixer->levels_thread, NULL, levels_thread, (void *)krad_mixer);
	pthread_create(&krad_mixer->active_input_thread, NULL, active_input_thread, (void *)krad_mixer);
	
	return krad_mixer;
	
}



void example_session(krad_mixer_t *krad_mixer) {

	krad_mixer_client_t *client = krad_mixer_new_client(krad_mixer);

	// Set up port groups.

	create_portgroup(client, "main", OUTPUT, STEREO, MAINMIX, 0);
	create_portgroup(client, "music1", INPUT, STEREO, MUSIC, 0);
	create_portgroup(client, "music2", INPUT, STEREO, MUSIC, 0);
	
	char portname1[256];
	char portname2[256];
	
	sprintf(portname2, "%s:music1_left", krad_mixer->client_name);
	sprintf(portname1, "xmms2_%s_1:out_1", krad_mixer->station);
	connect_port(krad_mixer->jack_client, portname1, portname2);
	
	sprintf(portname2, "%s:music1_right", krad_mixer->client_name);
	sprintf(portname1, "xmms2_%s_1:out_2", krad_mixer->station);
	connect_port(krad_mixer->jack_client, portname1, portname2);
	
	sprintf(portname2, "%s:music2_left", krad_mixer->client_name);
	sprintf(portname1, "xmms2_%s_2:out_1", krad_mixer->station);
	connect_port(krad_mixer->jack_client, portname1, portname2);
	
	sprintf(portname2, "%s:music2_right", krad_mixer->client_name);
	sprintf(portname1, "xmms2_%s_2:out_2", krad_mixer->station);
	connect_port(krad_mixer->jack_client, portname1, portname2);

	sprintf(portname1, "%s:main_left", krad_mixer->client_name);
	sprintf(portname2, "kradlink_%s:flac_encoder_left", krad_mixer->station);
	connect_port(krad_mixer->jack_client, portname1, portname2);
	
	sprintf(portname1, "%s:main_right", krad_mixer->client_name);
	sprintf(portname2, "kradlink_%s:flac_encoder_right", krad_mixer->station);
	connect_port(krad_mixer->jack_client, portname1, portname2);
	
	/*
	create_portgroup(client, "aux1", INPUT, STEREO, AUX, 0);
	create_portgroup(client, "submix1", OUTPUT, STEREO, SUBMIX, 1);
	create_portgroup(client, "submix2", OUTPUT, STEREO, SUBMIX, 2);
	create_portgroup(client, "mic1", INPUT, MONO, MIC, 1);
	create_portgroup(client, "mic2", INPUT, MONO, MIC, 2);
	*/
	
	
	
	krad_mixer_close_client(client);
	
	
}

void listcontrols(krad_mixer_client_t *client, char *data) {

	client->pos = 0;

	client->pos += sprintf(data + client->pos, "krad_mixer_state:");

	//for (portgroup = client->krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (client->pg = 0; client->pg < PORTGROUP_MAX; client->pg++) {
	client->portgroup = client->krad_mixer->portgroup[client->pg];
	if ((client->portgroup != NULL) && (client->portgroup->active)) {
		if ((client->portgroup->type == MUSIC) || (client->portgroup->type == MIC) || (client->portgroup->type == STEREOMIC) || (client->portgroup->type == AUX)) { 
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/volume_left|%f|%f|%f*", client->portgroup->name, 0.0f, 100.0f, client->portgroup->volume[0]);
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/volume_right|%f|%f|%f*", client->portgroup->name, 0.0f, 100.0f, client->portgroup->volume[1]);
		}
		
		if ((client->portgroup->type == SUBMIX)) { 
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/monitor|%f|%f|%d*", client->portgroup->name, 0.0f, 1.0f, client->portgroup->monitor);
		}
		
		if ((client->portgroup->type == MAINMIX)) { 
		/*
			x += sprintf(data + x, "/mainmixer/%s/highpass|%f|%f|%f*", portgroup->name, 20.0f, 20000.0f, client->krad_mixer->pass_left->high_freq);
			x += sprintf(data + x, "/mainmixer/%s/lowpass|%f|%f|%f*", portgroup->name, 20.0f, 20000.0f, client->krad_mixer->pass_left->low_freq);
		*/
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/music_1-2_crossfader|%f|%f|%f*", client->portgroup->name, -1.0f, 1.0f, client->krad_mixer->music_1_2_crossfade);
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/music-aux_crossfader|%f|%f|%f*", client->portgroup->name, -1.0f, 1.0f, client->krad_mixer->music_aux_crossfade);
			
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/fastlimiter_limit|%f|%f|%f*", client->portgroup->name, -20.0f, 0.0f, client->portgroup->fastlimiter->limit);
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/fastlimiter_release|%f|%f|%f*", client->portgroup->name, 0.01f, 2.0f, client->portgroup->fastlimiter->release);
			
		}
		/*
		if ((portgroup->type == MUSIC)) {

			x += sprintf(data + x, "/mainmixer/%s/digilogue_drive|%f|%f|%f*", portgroup->name, 0.1f, 10.0f, client->krad_mixer->digilogue_left->drive);
			x += sprintf(data + x, "/mainmixer/%s/digilogue_blend|%f|%f|%f*", portgroup->name, -10.0f, 10.0f, client->krad_mixer->digilogue_left->blend);
			x += sprintf(data + x, "/mainmixer/%s/digilogue_on|%f|%f|%d*", portgroup->name, 0.0f, 1.0f, portgroup->digilogue_on);
		}
		*/
		if ((client->portgroup->direction == INPUT)) { 

			client->pos += sprintf(data + client->pos, "/mainmixer/%s/djeq_low|%f|%f|%f*", client->portgroup->name, -50.0f, 20.0f, client->portgroup->djeq->low);
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/djeq_mid|%f|%f|%f*", client->portgroup->name, -50.0f, 20.0f, client->portgroup->djeq->mid);
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/djeq_high|%f|%f|%f*", client->portgroup->name, -50.0f, 20.0f, client->portgroup->djeq->high);
			client->pos += sprintf(data + client->pos, "/mainmixer/%s/djeq_on|%f|%f|%d*", client->portgroup->name, 0.0f, 1.0f, client->portgroup->djeq_on);

		}

	}
	}

}


int setcontrol(krad_mixer_client_t *client, char *data) {

	client->temp = NULL;
	strtok_r(data, "/", &client->temp);

	client->unit  = strtok_r(NULL, "/", &client->temp);

	client->subunit = strtok_r(NULL, "/", &client->temp);

	client->control = strtok_r(NULL, "/", &client->temp);

	client->float_value = atof(strtok_r(NULL, "/", &client->temp));

	//printf("Unit: %s Subunit: %s Control: %s Value: %f\n", client->unit, client->subunit, client->control, client->float_value);
	
	//for (portgroup = client->krad_mixer->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (client->pg = 0; client->pg < PORTGROUP_MAX; client->pg++) {
	client->portgroup = client->krad_mixer->portgroup[client->pg];
	if ((client->portgroup != NULL) && (client->portgroup->active)) {
		if ((client->portgroup->active) && (strcmp(client->subunit, client->portgroup->name)) == 0) {
			
			//printf("I'm on %s\n",  client->portgroup->name);
			
			if ((strncmp(client->control, "volume", 6) == 0) || (strncmp(client->control, "volume_left", 11) == 0)) {
				client->portgroup->volume[0] = client->float_value;
				client->volume_temp = (client->float_value/100.0f);
				client->volume_temp *= client->volume_temp;
				//printf("Left Old Value: %f New Value %f\n", client->portgroup->volume_actual[0], client->volume_temp);
				client->portgroup->new_volume_actual[0] = client->volume_temp;
				//printf("Set Left Value: %f\n", client->portgroup->new_volume_actual[0]);
			
			
			}
			if ((strncmp(client->control, "volume", 6) == 0) || (strncmp(client->control, "volume_right", 12) == 0)) {
				client->portgroup->volume[1] = client->float_value;
				client->volume_temp = (client->float_value/100.0f);
				client->volume_temp *= client->volume_temp;
				//printf("Right Old Value: %f New Value %f\n", client->portgroup->volume_actual[1], client->volume_temp);
				client->portgroup->new_volume_actual[1] = client->volume_temp;
				//printf("Set Right Value: %f\n", client->portgroup->new_volume_actual[0]);
			
			}
			
			if (strncmp(client->control, "monitor", 7) == 0) {
			
				client->portgroup->monitor = client->float_value;
			
			}
			
			if (strncmp(client->control, "djeq_on", 7) == 0) {
			
				client->portgroup->djeq_on = client->float_value;
			
			}
			
			if (strncmp(client->control, "fastlimiter_release", 19) == 0) {	
				client->portgroup->fastlimiter->new_release = client->float_value;
			}
			
			if (strncmp(client->control, "fastlimiter_limit", 17) == 0) {	
				client->portgroup->fastlimiter->new_limit = client->float_value;
			}
			
			
			if (strncmp(client->control, "djeq_low", 8) == 0) {	
				client->portgroup->djeq->low = client->float_value;
			}
		
			if (strncmp(client->control, "djeq_mid", 8) == 0) {	
				client->portgroup->djeq->mid = client->float_value;
			}
			
			if (strncmp(client->control, "djeq_high", 9) == 0) {	
				client->portgroup->djeq->high = client->float_value;
			}
			/*
			if (strcmp(control, "digilogue_drive") == 0) {	
				client->krad_mixer->digilogue_left->drive = value;
				client->krad_mixer->digilogue_right->drive = value;
			}
			
			if (strcmp(control, "digilogue_blend") == 0) {	
				client->krad_mixer->digilogue_left->blend = value;
				client->krad_mixer->digilogue_right->blend = value;
			}
			
			if (strcmp(control, "highpass") == 0) {	
				client->krad_mixer->pass_left->high_freq = value;
				client->krad_mixer->pass_right->high_freq = value;
			}
			
			if (strcmp(control, "lowpass") == 0) {	
				client->krad_mixer->pass_left->low_freq = value;
				client->krad_mixer->pass_right->low_freq = value;
			}
			*/
			
			if (strncmp(client->control, "music_1-2_crossfader", 20) == 0) {	
				client->krad_mixer->music_1_2_crossfade = client->float_value;
				
				client->krad_mixer->new_music1_fade_value[0] = get_fade_out_amount(client->krad_mixer->music_1_2_crossfade, client->fade_temp, client->fade_temp_pos);
				client->krad_mixer->new_music1_fade_value[1] = client->krad_mixer->new_music1_fade_value[0];

				client->krad_mixer->new_music2_fade_value[0] = get_fade_in_amount(client->krad_mixer->music_1_2_crossfade, client->fade_temp, client->fade_temp_pos);
				client->krad_mixer->new_music2_fade_value[1] = client->krad_mixer->new_music2_fade_value[0];
				
				// temp ghetto way
				if (client->krad_mixer->music_1_2_crossfade >= 0.0f) {
					client->krad_mixer->active_input = 2;
				} else {
					client->krad_mixer->active_input = 1;
				}
			}
			
			if (strncmp(client->control, "music-aux_crossfader", 20) == 0) {
				client->krad_mixer->music_aux_crossfade = client->float_value;
				
				client->krad_mixer->new_music_fade_value[0] = get_fade_out_amount(client->krad_mixer->music_aux_crossfade, client->fade_temp, client->fade_temp_pos);
				client->krad_mixer->new_music_fade_value[1] = client->krad_mixer->new_music_fade_value[0];
				client->krad_mixer->new_aux_fade_value[0] = get_fade_in_amount(client->krad_mixer->music_aux_crossfade, client->fade_temp, client->fade_temp_pos);
				client->krad_mixer->new_aux_fade_value[1] = client->krad_mixer->new_aux_fade_value[0];
				
				
			}
			
			return 1;
		}
	
	}
	}

	return 0;

}




void krad_mixer_want_shutdown() {
	
	krad_ipc_server_want_shutdown();

}

void krad_mixer_shutdown(void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	krad_mixer->shutdown = 1;
	printf("krad_mixer Shutting Down\n");
	jack_client_close (krad_mixer->jack_client);
	printf("krad_mixer Jack Client Closed\n");
	pthread_cancel(krad_mixer->levels_thread);
	pthread_cancel(krad_mixer->active_input_thread);
}

int krad_mixer_handler(char *data, void *pointer) {

	krad_mixer_client_t *client = (krad_mixer_client_t *)pointer;

	//printf("Krad Mixer Daemon Handler got %zu byte message: '%s' \n", strlen(data), data);
	
	//reset things from last command
	client->temp = NULL;
	strcpy(client->path, "");
	strcpy(client->path2, "");
	strcpy(client->path3, "");
	strcpy(client->path4, "");
	strcpy(client->path5, "");
	strcpy(client->datacpy, data);
	client->cmd = strtok_r(client->datacpy, ",", &client->temp);
	
	
	//if (strncmp(client->cmd, "broadcasts", 10) == 0) {
	if (strncmp(client->cmd, "getpeak", 7) == 0) {
		krad_ipc_server_set_client_broadcasts(client->krad_mixer->ipc, client, 2);
		printf("client asked for broadcasts\n");
		return 0; // to not broadcast
	}
	
	if (strncmp(client->cmd, "listcontrols", 12) == 0) {
		printf("Listing Controls\n");
		listcontrols(client, data);
		return 0; // to not broadcast
	}

	if (strncmp(client->cmd, "xruns", 5) == 0) {
		printf("Number of xruns: %d\n", client->krad_mixer->xruns);
		sprintf(data, "Number of xruns: %d\n", client->krad_mixer->xruns);
		return 0; // to not broadcast
	}
	
	if (strncmp(client->cmd, "get_active_input", 16) == 0) {
		printf("Active Input: %d\n", client->krad_mixer->active_input);
		sprintf(data, "Active Input: %d\n", client->krad_mixer->active_input);
		return 0; // to not broadcast
	}


	if (strncmp(client->cmd, "setcontrol", 9) == 0) {
		if (setcontrol(client, client->datacpy)) {
			sprintf(data, "krad_mixer:setcontrol/mainmixer/%s/%s/%f", client->subunit, client->control, client->float_value);
			//printf("data: %s\n", data);
			return 1; // to broadcast
		} else {
			sprintf(data, "krad_mixer:setcontrol/failed");
			//printf("data: %s\n", data);
			return 0;
		}
	}

	if (strncmp(client->cmd, "create_portgroup", 16) == 0) {
		strcat(client->path, strtok_r(NULL, ",", &client->temp));
		//strcat(client->path2, strtok_r(NULL, ",", &client->temp));
		//strcat(client->path3, strtok_r(NULL, ",", &client->temp));
		//strcat(client->path4, strtok_r(NULL, ",", &client->temp));
		
		if (strncmp(client->path, "sampler", 7) == 0) {
			sprintf(client->path5, "sampler");
			create_portgroup(client, client->path5, INPUT, STEREO, AUX, 0);
		}
		
		if (strncmp(client->path, "synth", 5) == 0) {
			sprintf(client->path5, "synth");
			create_portgroup(client, client->path5, INPUT, STEREO, AUX, 0);
		}
		
		sprintf(data, "krad_mixer:portgroup/add/%s", client->path5);
		return 2;
	}

	if (strncmp(client->cmd, "create_advanced_portgroup", 25) == 0) {
		strcat(client->path, strtok_r(NULL, ",", &client->temp));
		strcat(client->path2, strtok_r(NULL, ",", &client->temp));
		strcat(client->path3, strtok_r(NULL, ",", &client->temp));
		//strcat(client->path4, strtok_r(NULL, ",", &client->temp));


		if (strncmp(client->path3, "MIC", 3) == 0) {
		
			client->krad_mixer->portgroup_count++;
		
			sprintf(client->path5, "submix_%s_out_for_%s", client->path2, client->path);
			create_portgroup(client, client->path5, OUTPUT, STEREO, SUBMIX, client->krad_mixer->portgroup_count);
			sprintf(client->path5, "mic_%s_in_from_%s", client->path2, client->path);
			create_portgroup(client, client->path5, INPUT, MONO, MIC, client->krad_mixer->portgroup_count);
		
		}
		
		if (strncmp(client->path3, "STEREOMIC", 9) == 0) {
		
			client->krad_mixer->portgroup_count++;

			sprintf(client->path5, "submix_%s_out_for_%s", client->path2, client->path);
			create_portgroup(client, client->path5, OUTPUT, STEREO, SUBMIX, client->krad_mixer->portgroup_count);
			sprintf(client->path5, "stereomic_%s_in_from_%s", client->path2, client->path);
			create_portgroup(client, client->path5, INPUT, STEREO, STEREOMIC, client->krad_mixer->portgroup_count);
		
		}
		
		if (strncmp(client->path3, "AUX", 3) == 0) {
			sprintf(client->path5, "aux_%s_in_from_%s", client->path2, client->path);
			create_portgroup(client, client->path5, INPUT, STEREO, AUX, 0);
		}

		sprintf(data, "krad_mixer:portgroup/add/%s", client->path5);
		return 2;
	}
	
	if (strncmp(client->cmd, "destroy_advanced_portgroup", 26) == 0) {
		strcat(client->path, strtok_r(NULL, ",", &client->temp));
		strcat(client->path2, strtok_r(NULL, ",", &client->temp));
		strcat(client->path3, strtok_r(NULL, ",", &client->temp));
		//strcat(client->path4, strtok_r(NULL, ",", &client->temp));


		if (strncmp(client->path3, "MIC", 3) == 0) {
		
			sprintf(client->path5, "submix_%s_out_for_%s", client->path2, client->path);
			destroy_portgroup_by_name(client, client->path5);
			sprintf(client->path5, "mic_%s_in_from_%s", client->path2, client->path);
			destroy_portgroup_by_name(client, client->path5);
		
		}
		
		if (strncmp(client->path3, "STEREOMIC", 9) == 0) {

			sprintf(client->path5, "submix_%s_out_for_%s", client->path2, client->path);
			destroy_portgroup_by_name(client, client->path5);
			sprintf(client->path5, "stereomic_%s_in_from_%s", client->path2, client->path);
			destroy_portgroup_by_name(client, client->path5);
		
		}
		
		if (strncmp(client->path3, "AUX", 3) == 0) {
			sprintf(client->path5, "aux_%s_in_from_%s", client->path2, client->path);
			destroy_portgroup_by_name(client, client->path5);
		}

		sprintf(data, "krad_mixer:portgroup/remove/%s", client->path5);
		return 2;
	}
	
	if (strncmp(client->cmd, "destroy_portgroup", 17) == 0) {
		strcat(client->path, strtok_r(NULL, ",", &client->temp));
		
		destroy_portgroup_by_name(client, client->path);
		sprintf(data, "krad_mixer:portgroup/remove/%s", client->path);
		return 2;

	}


	if (strncmp(client->cmd, "kill", 4) == 0) {
		printf("Killed by a client..\n");
		krad_mixer_want_shutdown();
		sprintf(data, "krad_mixer shutting down now!");
		return 1; // to broadcast
	}


}




void *krad_mixer_new_client (void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;
	
	krad_mixer_client_t *client;
	
	client = calloc(1, sizeof(krad_mixer_client_t));
	
	client->krad_mixer = krad_mixer;
	
	return client;
	
}

void krad_mixer_close_client (void *arg) {

	free(arg);

}



