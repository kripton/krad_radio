#include "krad_jack.h"

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

		portgroup->ports[c] = jack_port_register (portgroup->krad_jack->client,
												  portname,
												  JACK_DEFAULT_AUDIO_TYPE,
												  port_direction,
												  0);

		if (portgroup->ports[c] == NULL) {
			printke ("Krad Jack could not reg port, prolly a dupe reg: %s\n", portname);
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
	
	printke ("Krad Jack %s xrun number %d!\n", krad_jack->name, krad_jack->xruns);

	return 0;

}

int krad_jack_process (jack_nframes_t nframes, void *arg) {

	krad_jack_t *krad_jack = (krad_jack_t *)arg;

	return krad_mixer_process (nframes, krad_jack->krad_audio->krad_mixer);

	return 0;

}

void krad_jack_shutdown (void *arg) {

	//jack_t *jack = (jack_t *)arg;

}

void krad_jack_destroy (krad_jack_t *krad_jack) {

	jack_client_close (krad_jack->client);
	
	krad_mixer_unset_pusher (krad_jack->krad_audio->krad_mixer);
	
	free (krad_jack);

}

int krad_jack_detect () {
	return krad_jack_detect_for_jack_server_name (NULL);
}

int krad_jack_detect_for_jack_server_name (char *server_name) {

	jack_client_t *client;
	jack_options_t options;
	jack_status_t status;
	char name[128];

	sprintf(name, "krad_jack_detect_%d", rand()); 

	if (server_name != NULL) {
		options = JackNoStartServer | JackServerName;
	} else {
		options = JackNoStartServer;
	}

	client = jack_client_open (name, options, &status, server_name);

	if (client == NULL) {
		return 0;
	} else {
		jack_client_close (client);
		return 1;
	}

}

krad_jack_t *krad_jack_create (krad_audio_t *krad_audio) {

	return krad_jack_create_for_jack_server_name (krad_audio, NULL);

}

krad_jack_t *krad_jack_create_for_jack_server_name (krad_audio_t *krad_audio, char *server_name) {

	krad_jack_t *krad_jack;

	if ((krad_jack = calloc (1, sizeof (krad_jack_t))) == NULL) {
		failfast ("Krad Jack memory alloc failure\n");
	}

	krad_jack->krad_audio = krad_audio;

	if (krad_mixer_has_pusher(krad_jack->krad_audio->krad_mixer)) {
		printke ("Krad Jackoh no already have a pusher...");
		return NULL;
	}

	krad_jack->name = krad_jack->krad_audio->krad_mixer->name;
	
	if (server_name != NULL) {
		krad_jack->options = JackNoStartServer | JackServerName;	
		krad_jack->server_name[sizeof(krad_jack->server_name) - 1] = '\0';
		snprintf(krad_jack->server_name, sizeof(krad_jack->server_name) - 2, "%s", server_name);
	} else {
		krad_jack->options = JackNoStartServer;	
		krad_jack->server_name[0] = '\0';
	}

	krad_jack->client = jack_client_open (krad_jack->name, krad_jack->options, &krad_jack->status, krad_jack->server_name);
	if (krad_jack->client == NULL) {
		failfast ("jack_client_open() failed, status = 0x%2.0x\n", krad_jack->status);
		if (krad_jack->status & JackServerFailed) {
			failfast ("Unable to connect to JACK server\n");
		}
	}
	
	if (krad_jack->status & JackNameNotUnique) {
		krad_jack->name = jack_get_client_name(krad_jack->client);
		printke ("Krad Jack unique name `%s' assigned\n", krad_jack->name);
	}

	krad_mixer_set_pusher (krad_jack->krad_audio->krad_mixer, JACK);

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
		failfast ("cannot activate client\n");
	}

	krad_jack->active = 1;

	return krad_jack;

}

