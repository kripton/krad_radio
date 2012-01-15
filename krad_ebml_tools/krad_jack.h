

#ifndef KRAD_JACK_H
#define KRAD_JACK_H

#include <krad_audio.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>

#define RINGBUFFER_SIZE 3000000

typedef struct krad_jack_St krad_jack_t;

struct krad_jack_St {

	krad_audio_t *kradaudio;

	int active;
	int xruns;
	const char *server_name;
	jack_port_t *input_ports[2];
	jack_port_t *output_ports[2];
	const char *client_name;
	jack_options_t options;
	jack_status_t status;
	jack_client_t *jack_client;
    
};


void kradjack_connect_port(jack_client_t *client, char *port_one, char *port_two);
void kradjack_destroy(krad_jack_t *jack);
krad_jack_t *kradjack_create(krad_audio_t *kradaudio);
int kradjack_process(jack_nframes_t nframes, void *arg);
void kradjack_shutdown(void *arg);
int kradjack_xrun(void *arg);

#endif

typedef struct krad_jack_St krad_jack_t;
