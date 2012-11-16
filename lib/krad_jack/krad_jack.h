#ifndef KRAD_JACK_H
#define KRAD_JACK_H

typedef struct krad_jack_St krad_jack_t;

#include "krad_audio.h"
#include <jack/jack.h>

typedef struct krad_jack_portgroup_St krad_jack_portgroup_t;

struct krad_jack_portgroup_St {

	krad_jack_t *krad_jack;

	char name[256];
	jack_port_t *ports[8];
	float *samples[8];
	int direction;
	int channels;

};

struct krad_jack_St {

	krad_audio_t *krad_audio;

	int active;
	
	int set_thread_name_process;

	int xruns;
	char server_name[256];
	const char *name;
	jack_options_t options;
	jack_status_t status;
	jack_client_t *client;
	
	int sample_rate;
	
	char *stay_connected[256];
	char *stay_connected_to[256];
	
	pthread_mutex_t connections_lock;	
    
};

//jack_port_t *input_ports[2];
//jack_port_t *output_ports[2];

//void jack_connect_to_ports (krad_audio_t *krad_audio, krad_audio_portgroup_direction_t direction, char *ports);
//void krad_jack_connect_port(jack_client_t *client, char *port_one, char *port_two);


int krad_jack_detect ();
int krad_jack_detect_for_jack_server_name (char *server_name);

void krad_jack_portgroup_samples_callback (int frames, void *userdata, float **samples);
void krad_jack_portgroup_plug (krad_jack_portgroup_t *portgroup, char *remote_name);
void krad_jack_portgroup_unplug (krad_jack_portgroup_t *portgroup, char *remote_name);
void krad_jack_portgroup_destroy (krad_jack_portgroup_t *portgroup);
krad_jack_portgroup_t *krad_jack_portgroup_create (krad_jack_t *krad_jack, char *name, int direction, int channels);

void krad_jack_destroy (krad_jack_t *jack);
krad_jack_t *krad_jack_create (krad_audio_t *krad_audio);
krad_jack_t *krad_jack_create_for_jack_server_name (krad_audio_t *krad_audio, char *server_name);

int krad_jack_process (jack_nframes_t nframes, void *arg);
void krad_jack_shutdown (void *arg);
int krad_jack_xrun (void *arg);

#endif
