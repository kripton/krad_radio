typedef struct krad_jack_St krad_jack_t;

#ifndef KRAD_JACK_H
#define KRAD_JACK_H


#include "krad_audio.h"
#include <jack/jack.h>


struct krad_jack_St {

	krad_audio_t *krad_audio;

	int active;

	int xruns;
	const char *server_name;
	const char *name;
	jack_options_t options;
	jack_status_t status;
	jack_client_t *client;
	
	int sample_rate;
	
    
};

//jack_port_t *input_ports[2];
//jack_port_t *output_ports[2];

//void jack_connect_to_ports (krad_audio_t *krad_audio, krad_audio_portgroup_direction_t direction, char *ports);
//void krad_jack_connect_port(jack_client_t *client, char *port_one, char *port_two);

void krad_jack_destroy (krad_jack_t *jack);
krad_jack_t *krad_jack_create (krad_audio_t *krad_audio);

int krad_jack_process (jack_nframes_t nframes, void *arg);
void krad_jack_shutdown (void *arg);
int krad_jack_xrun (void *arg);

#endif
