#include "krad_jack.h"

/*
void jack_connect_to_ports (krad_audio_t *krad_audio, krad_audio_direction_t direction, char *ports) {

	//const char *ports;
	const char *src_port, *dst_port;;
	const char **remote_ports;
	int i, err;
	
	krad_jack_t *krad_jack = (krad_jack_t *)krad_audio->api;
	
	if (direction == KINPUT) {

		if ((strlen(ports) == 0) || ((strncmp(ports, "physical", 8) == 0))) {
			remote_ports = jack_get_ports (krad_jack->jack_client, NULL, NULL,
				                           JackPortIsOutput | JackPortIsPhysical);

		} else {
			remote_ports = jack_get_ports (krad_jack->jack_client, ports, NULL, JackPortIsOutput);
	
		}

		for (i = 0; i < 2 && remote_ports && remote_ports[i]; i++) {
			dst_port = jack_port_name (krad_jack->input_ports[i]);
			printf("Connecting port %d %s\n", i , dst_port);
			err = jack_connect (krad_jack->jack_client, remote_ports[i], dst_port);
			if (err < 0) {
				return;
			}
		}
	}

	if (direction == KOUTPUT) {

		if ((strlen(ports) == 0) || ((strncmp(ports, "physical", 8) == 0))) {
			remote_ports = jack_get_ports (krad_jack->jack_client, NULL, NULL,
				                           JackPortIsInput | JackPortIsPhysical);

		} else {
			remote_ports = jack_get_ports (krad_jack->jack_client, ports, NULL, JackPortIsInput);
	
		}

		for (i = 0; i < 2 && remote_ports && remote_ports[i]; i++) {
			src_port = jack_port_name (krad_jack->output_ports[i]);
			printf("Connecting port %d %s\n", i , src_port);
			err = jack_connect (krad_jack->jack_client, src_port, remote_ports[i]);
			if (err < 0) {
				return;
			}
		}
	}

}

void krad_jack_connect_port(jack_client_t *client, char *port_one, char *port_two) {
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
*/


void krad_jack_portgroup_samples_callback (int frames, void *userdata, float **samples) {

	krad_jack_portgroup_t *portgroup = (krad_jack_portgroup_t *)userdata;
	int c;
	float *temp;
	
	for (c = 0; c < portgroup->channels; c++) {
		if (portgroup->direction == INPUT) {
			temp = jack_port_get_buffer (portgroup->ports[c], frames);
			memcpy (portgroup->samples[c], temp, frames * 4);
			samples[c] = portgroup->samples[c];
		} else {
			samples[c] = jack_port_get_buffer (portgroup->ports[c], frames);
		}

	}
}

void krad_jack_portgroup_destroy (krad_jack_portgroup_t *portgroup) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {

		jack_port_unregister (portgroup->krad_jack->client, portgroup->ports[c]);	
	
		portgroup->ports[c] = NULL;
	
		if (portgroup->direction == INPUT) {		
			free (portgroup->samples[c]);
		}
	}
	
	free (portgroup);
}


krad_jack_portgroup_t *krad_jack_portgroup_create (krad_jack_t *krad_jack, char *name, int direction, int channels) {

	int c;
	int port_direction;
	char portname[256];
	krad_jack_portgroup_t *portgroup;
	
	portgroup = calloc (1, sizeof(krad_jack_portgroup_t));
	
	portgroup->krad_jack = krad_jack;
	portgroup->channels = channels;
	portgroup->direction = direction;
	strcpy ( portgroup->name, name );
		
	if (portgroup->direction == INPUT) {		
		port_direction = JackPortIsInput;
	} else {
		port_direction = JackPortIsOutput;
	}

	for (c = 0; c < portgroup->channels; c++) {

		strcpy ( portname, name );

		if (portgroup->channels > 1) {
			strcat ( portname, "_" );
			strcat ( portname, krad_mixer_channel_number_to_string ( c ) );
		}

		portgroup->ports[c] = jack_port_register (portgroup->krad_jack->client, portname, JACK_DEFAULT_AUDIO_TYPE, port_direction, 0);

		if (portgroup->ports[c] == NULL) {
			fprintf(stderr, "could not reg, prolly a dupe reg: %s\n", portname);
			return NULL;
		}
		
		if (portgroup->direction == INPUT) {		
			portgroup->samples[c] = calloc (1, 16384);
		}	
	}

	return portgroup;

}

int krad_jack_xrun (void *arg) {

	krad_jack_t *krad_jack = (krad_jack_t *)arg;

	krad_jack->xruns++;
	
	printf("%s xrun number %d!\n", krad_jack->name, krad_jack->xruns);

	return 0;

}

int krad_jack_process (jack_nframes_t nframes, void *arg) {

	krad_jack_t *krad_jack = (krad_jack_t *)arg;



	return krad_mixer_process (nframes, krad_jack->krad_audio->krad_mixer);

/*
	int c, s;

	jack_default_audio_sample_t *samples[2];

	if (krad_jack->active) {
		
		if ((krad_jack->krad_audio->direction == KINPUT) || (krad_jack->krad_audio->direction == KDUPLEX)) {

			samples[0] = jack_port_get_buffer (krad_jack->input_ports[0], nframes);
			samples[1] = jack_port_get_buffer (krad_jack->input_ports[1], nframes);
	
			if ((krad_ringbuffer_write_space(krad_jack->krad_audio->input_ringbuffer[0]) > (nframes * 4)) && (krad_ringbuffer_write_space(krad_jack->krad_audio->input_ringbuffer[1]) > (nframes * 4))) {

				krad_ringbuffer_write (krad_jack->krad_audio->input_ringbuffer[0], (char *)samples[0], nframes * 4);
				krad_ringbuffer_write (krad_jack->krad_audio->input_ringbuffer[1], (char *)samples[1], nframes * 4);

				for (c = 0; c < 2; c++) {
					compute_peak(krad_jack->krad_audio, KINPUT, samples[c], c, nframes, 0);
				}

			}
		
		}
		
		if (krad_jack->krad_audio->process_callback != NULL) {
			krad_jack->krad_audio->process_callback(nframes, krad_jack->krad_audio->userdata);
		}
		
		if ((krad_jack->krad_audio->direction == KOUTPUT) || (krad_jack->krad_audio->direction == KDUPLEX)) {
		
			samples[0] = jack_port_get_buffer (krad_jack->output_ports[0], nframes);
			samples[1] = jack_port_get_buffer (krad_jack->output_ports[1], nframes);
	
			if ((krad_ringbuffer_read_space(krad_jack->krad_audio->output_ringbuffer[0]) > (nframes * 4)) && (krad_ringbuffer_read_space(krad_jack->krad_audio->output_ringbuffer[1]) > (nframes * 4))) {

				krad_ringbuffer_read (krad_jack->krad_audio->output_ringbuffer[0], (char *)samples[0], nframes * 4);
				krad_ringbuffer_read (krad_jack->krad_audio->output_ringbuffer[1], (char *)samples[1], nframes * 4);

				for (c = 0; c < 2; c++) {
					compute_peak(krad_jack->krad_audio, KOUTPUT, samples[c], c, nframes, 0);
				}

			} else {

				for (s = 0; s < nframes; s++) {
					samples[0][s] = 0;
					samples[1][s] = 0;
				}
	
			}
		}
	
	}
*/
	return 0;

}

void krad_jack_shutdown (void *arg) {

	//jack_t *jack = (jack_t *)arg;

}

void krad_jack_destroy (krad_jack_t *krad_jack) {

	jack_client_close (krad_jack->client);
	
	free (krad_jack);

}

krad_jack_t *krad_jack_create (krad_audio_t *krad_audio) {

	krad_jack_t *krad_jack;

	if ((krad_jack = calloc (1, sizeof (krad_jack_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}

	krad_jack->krad_audio = krad_audio;

	krad_jack->name = krad_jack->krad_audio->krad_mixer->name;

	krad_jack->server_name = NULL;
	krad_jack->options = JackNoStartServer;

	// Connect up to the JACK server 

	krad_jack->client = jack_client_open (krad_jack->name, krad_jack->options, &krad_jack->status, krad_jack->server_name);
	if (krad_jack->client == NULL) {
			fprintf(stderr, "jack_client_open() failed, "
			"status = 0x%2.0x\n", krad_jack->status);
		if (krad_jack->status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	
	if (krad_jack->status & JackNameNotUnique) {
		krad_jack->name = jack_get_client_name(krad_jack->client);
		fprintf(stderr, "unique name `%s' assigned\n", krad_jack->name);
	}


	krad_jack->sample_rate = jack_get_sample_rate ( krad_jack->client );

	if (krad_jack->sample_rate != krad_mixer_get_sample_rate (krad_jack->krad_audio->krad_mixer)) {
		krad_mixer_set_sample_rate (krad_jack->krad_audio->krad_mixer, krad_jack->sample_rate);
		printk ("Jack set Krad Mixer Sample Rate to %d\n", krad_jack->sample_rate);		
	}

	// Set up Callbacks

	jack_set_process_callback (krad_jack->client, krad_jack_process, krad_jack);
	jack_on_shutdown (krad_jack->client, krad_jack_shutdown, krad_jack);
	jack_set_xrun_callback (krad_jack->client, krad_jack_xrun, krad_jack);

	// Activate

	if (jack_activate (krad_jack->client)) {
		printf("cannot activate client\n");
		exit (1);
	}

	/*
	if ((krad_jack->krad_audio->direction == KINPUT) || (krad_jack->krad_audio->direction == KDUPLEX)) {
		krad_jack->input_ports[0] = jack_port_register (krad_jack->jack_client, "InputLeft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		krad_jack->input_ports[1] = jack_port_register (krad_jack->jack_client, "InputRight", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	}
	
	if ((krad_jack->krad_audio->direction == KOUTPUT) || (krad_jack->krad_audio->direction == KDUPLEX)) {	
		krad_jack->output_ports[0] = jack_port_register (krad_jack->jack_client, "Left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		krad_jack->output_ports[1] = jack_port_register (krad_jack->jack_client, "Right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	}
	
	
	char portname[128];
	
	sprintf(portname, "%s:Left", krad_jack->client_name);
	krad_jack_connect_port(krad_jack->jack_client, portname, "firewire_pcm:001486af2e61ac6b_Unknown1_out");
	sprintf(portname, "%s:Right", krad_jack->client_name);
	krad_jack_connect_port(krad_jack->jack_client, portname, "firewire_pcm:001486af2e61ac6b_Unknown2_out");

	sprintf(portname, "%s:InputLeft", krad_jack->client_name);
	krad_jack_connect_port(krad_jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown_in", "krad_opus_ebml:InputLeft");
	sprintf(portname, "%s:InputRight", krad_jack->client_name);
	krad_jack_connect_port(krad_jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "krad_opus_ebml:InputRight");
	*/

	krad_jack->active = 1;

	return krad_jack;

}

