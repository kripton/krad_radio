#include <krad_jack.h>


void jack_connect_to_ports (krad_audio_t *krad_audio, krad_audio_direction_t direction, char *ports)
{

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

void kradjack_connect_port(jack_client_t *client, char *port_one, char *port_two)
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

int kradjack_xrun(void *arg)
{

	krad_jack_t *kradjack = (krad_jack_t *)arg;

	kradjack->xruns++;
	
	printf("%s xrun number %d!\n", kradjack->client_name, kradjack->xruns);

	return 0;

}

int kradjack_process(jack_nframes_t nframes, void *arg) {

	krad_jack_t *kradjack = (krad_jack_t *)arg;

	int c, s;

	jack_default_audio_sample_t *samples[2];

	if (kradjack->active) {

		samples[0] = jack_port_get_buffer (kradjack->input_ports[0], nframes);
		samples[1] = jack_port_get_buffer (kradjack->input_ports[1], nframes);
	
		if ((krad_ringbuffer_write_space(kradjack->kradaudio->input_ringbuffer[0]) > (nframes * 4)) && (krad_ringbuffer_write_space(kradjack->kradaudio->input_ringbuffer[1]) > (nframes * 4))) {

			krad_ringbuffer_write (kradjack->kradaudio->input_ringbuffer[0], (char *)samples[0], nframes * 4);
			krad_ringbuffer_write (kradjack->kradaudio->input_ringbuffer[1], (char *)samples[1], nframes * 4);

			for (c = 0; c < 2; c++) {
				compute_peak(kradjack->kradaudio, KINPUT, samples[c], c, nframes, 0);
			}

		}
		
		if (kradjack->kradaudio->process_callback != NULL) {
			kradjack->kradaudio->process_callback(nframes, kradjack->kradaudio->userdata);
		}
		
		samples[0] = jack_port_get_buffer (kradjack->output_ports[0], nframes);
		samples[1] = jack_port_get_buffer (kradjack->output_ports[1], nframes);
	
		if ((krad_ringbuffer_read_space(kradjack->kradaudio->output_ringbuffer[0]) > (nframes * 4)) && (krad_ringbuffer_read_space(kradjack->kradaudio->output_ringbuffer[1]) > (nframes * 4))) {

			krad_ringbuffer_read (kradjack->kradaudio->output_ringbuffer[0], (char *)samples[0], nframes * 4);
			krad_ringbuffer_read (kradjack->kradaudio->output_ringbuffer[1], (char *)samples[1], nframes * 4);

			for (c = 0; c < 2; c++) {
				compute_peak(kradjack->kradaudio, KOUTPUT, samples[c], c, nframes, 0);
			}

		} else {

			for (s = 0; s < nframes; s++) {
				samples[0][s] = 0;
				samples[1][s] = 0;
			}
	
		}
	
	}

	return 0;

}

void kradjack_shutdown(void *arg) {

	//jack_t *jack = (jack_t *)arg;
	

}

void kradjack_destroy(krad_jack_t *kradjack) {

	jack_client_close (kradjack->jack_client);
	
	free(kradjack);

}

krad_jack_t *kradjack_create(krad_audio_t *kradaudio) {

	krad_jack_t *kradjack;

	if ((kradjack = calloc (1, sizeof (krad_jack_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}

	kradjack->kradaudio = kradaudio;

	kradjack->client_name = kradjack->kradaudio->name;

	kradjack->server_name = NULL;
	kradjack->options = JackNoStartServer;

	// Connect up to the JACK server 

	kradjack->jack_client = jack_client_open (kradjack->client_name, kradjack->options, &kradjack->status, kradjack->server_name);
	if (kradjack->jack_client == NULL) {
			fprintf(stderr, "jack_client_open() failed, "
			"status = 0x%2.0x\n", kradjack->status);
		if (kradjack->status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	
	if (kradjack->status & JackNameNotUnique) {
		kradjack->client_name = jack_get_client_name(kradjack->jack_client);
		fprintf(stderr, "unique name `%s' assigned\n", kradjack->client_name);
	}


	kradaudio->sample_rate = jack_get_sample_rate ( kradjack->jack_client );

	// Set up Callbacks

	jack_set_process_callback (kradjack->jack_client, kradjack_process, kradjack);
	jack_on_shutdown (kradjack->jack_client, kradjack_shutdown, kradjack);
	jack_set_xrun_callback (kradjack->jack_client, kradjack_xrun, kradjack);

	// Activate

	if (jack_activate (kradjack->jack_client)) {
		printf("cannot activate client\n");
		exit (1);
	}

	kradjack->input_ports[0] = jack_port_register (kradjack->jack_client, "InputLeft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	kradjack->input_ports[1] = jack_port_register (kradjack->jack_client, "InputRight", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	
	kradjack->output_ports[0] = jack_port_register (kradjack->jack_client, "Left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	kradjack->output_ports[1] = jack_port_register (kradjack->jack_client, "Right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	/*
	char portname[128];
	
	sprintf(portname, "%s:Left", kradjack->client_name);
	kradjack_connect_port(kradjack->jack_client, portname, "firewire_pcm:001486af2e61ac6b_Unknown1_out");
	sprintf(portname, "%s:Right", kradjack->client_name);
	kradjack_connect_port(kradjack->jack_client, portname, "firewire_pcm:001486af2e61ac6b_Unknown2_out");

	sprintf(portname, "%s:InputLeft", kradjack->client_name);
	kradjack_connect_port(kradjack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown_in", "krad_opus_ebml:InputLeft");
	sprintf(portname, "%s:InputRight", kradjack->client_name);
	kradjack_connect_port(kradjack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "krad_opus_ebml:InputRight");
	*/

	kradjack->active = 1;

	return kradjack;

}

