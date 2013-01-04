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

void krad_jack_check_connection (krad_jack_t *krad_jack, char *remote_port) {

	int p;
	int flags;
	jack_port_t *port;
	
	pthread_mutex_lock (&krad_jack->connections_lock);	
	
	if (strlen(remote_port)) {
		for (p = 0; p < 256; p++) {
			if ((krad_jack->stay_connected[p] != NULL) && (krad_jack->stay_connected_to[p] != NULL)) {		
				if ((strncmp(krad_jack->stay_connected_to[p], remote_port, strlen(remote_port))) == 0) {
				
					port = jack_port_by_name (krad_jack->client, remote_port);
					flags = jack_port_flags (port);
				
					if (flags == JackPortIsOutput) {		
						printk ("jack want to replug %s to %s", remote_port, krad_jack->stay_connected[p]);
						jack_connect (krad_jack->client, remote_port, krad_jack->stay_connected[p]);
					} else {
						printk ("jack want to replug %s to %s", krad_jack->stay_connected[p], remote_port);
						jack_connect (krad_jack->client, krad_jack->stay_connected[p], remote_port);
					}				
				}
			}
		}
	}
	
	pthread_mutex_unlock (&krad_jack->connections_lock);
	
}

void krad_jack_stay_connection (krad_jack_t *krad_jack, char *port, char *remote_port) {

	int p;
	
	pthread_mutex_lock (&krad_jack->connections_lock);	
	
	if (strlen(port) && strlen(remote_port)) {
		for (p = 0; p < 256; p++) {
			if (krad_jack->stay_connected[p] == NULL) {
				krad_jack->stay_connected[p] = strdup(port);
				krad_jack->stay_connected_to[p] = strdup(remote_port);
				break;
			}
		}
	}
	
	pthread_mutex_unlock (&krad_jack->connections_lock);		
	
}

void krad_jack_unstay_connection (krad_jack_t *krad_jack, char *port, char *remote_port) {

	int p;
	
	pthread_mutex_lock (&krad_jack->connections_lock);	
	
	for (p = 0; p < 256; p++) {
		if (krad_jack->stay_connected[p] != NULL) {
			if ((krad_jack->stay_connected[p] != NULL) && (krad_jack->stay_connected_to[p] != NULL)) {
				if (((strncmp(krad_jack->stay_connected[p], port, strlen(port))) == 0) &&
					 ((strlen(remote_port) == 0) || (strncmp(krad_jack->stay_connected_to[p], remote_port, strlen(remote_port)) == 0))) {
					free (krad_jack->stay_connected[p]);
					free (krad_jack->stay_connected_to[p]);
					break;
				}
			}
		}
	}
	
	pthread_mutex_unlock (&krad_jack->connections_lock);
	
}

void krad_jack_port_registration_callback (jack_port_id_t portid, int regged, void *arg) {

	krad_jack_t *krad_jack = (krad_jack_t *)arg;
	
	jack_port_t *port;

	port = jack_port_by_id (krad_jack->client, portid);
	
	if (regged == 1) {
		printk ("jack port %s registered", jack_port_name (port));
		if (jack_port_is_mine(krad_jack->client, port) == 0) {
			krad_jack_check_connection (krad_jack, (char *)jack_port_name(port));
		}
	} else {
		printk ("jack port %s unregistered", jack_port_name (port));
	}
}


void krad_jack_port_connection_callback (jack_port_id_t a, jack_port_id_t b, int connect, void *arg) {

	krad_jack_t *krad_jack = (krad_jack_t *)arg;
	
	jack_port_t *ports[2];
	
	ports[0] = jack_port_by_id (krad_jack->client, a);
	ports[1] = jack_port_by_id (krad_jack->client, b);	

	if (connect == 1) {
		printk ("jack port %s connected to %s ", jack_port_name (ports[0]), jack_port_name (ports[1]));
	} else {
		printk ("jack port %s disconnected from %s ", jack_port_name (ports[0]), jack_port_name (ports[1]));
	}

}

void krad_jack_portgroup_plug (krad_jack_portgroup_t *portgroup, char *remote_name) {

	const char **ports;
	int flags;
	int c;
		
	flags = 0;
	
	if (strlen(remote_name) == 0) {
		return;
	}

	ports = jack_get_ports (portgroup->krad_jack->client, remote_name, JACK_DEFAULT_AUDIO_TYPE, flags);

	if (ports) {
		for (c = 0; c < portgroup->channels; c++) {
			if (ports[c]) {
				krad_jack_stay_connection (portgroup->krad_jack, (char *)jack_port_name(portgroup->ports[c]), (char *)ports[c]);
				if (portgroup->direction == INPUT) {	
					printk ("jack want to plug %s to %s", ports[c], jack_port_name(portgroup->ports[c]));
					jack_connect (portgroup->krad_jack->client, ports[c], jack_port_name(portgroup->ports[c]));
				} else {
					printk ("jack want to plug %s to %s", jack_port_name(portgroup->ports[c]), ports[c]);
					jack_connect (portgroup->krad_jack->client, jack_port_name(portgroup->ports[c]), ports[c]);
				}			
			} else {
				return;
			}
		}
	}
}

void krad_jack_portgroup_unplug (krad_jack_portgroup_t *portgroup, char *remote_name) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		krad_jack_unstay_connection (portgroup->krad_jack, (char *)jack_port_name(portgroup->ports[c]), remote_name);
		jack_port_disconnect (portgroup->krad_jack->client, portgroup->ports[c]);
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
	
	if (krad_jack->set_thread_name_process == 0) {
    krad_system_set_thread_name ("kr_jack_mix");
    krad_jack->set_thread_name_process = 1;
  }

	return krad_mixer_process (nframes, krad_jack->krad_audio->krad_mixer);

	return 0;

}

void krad_jack_shutdown (void *arg) {

	krad_jack_t *krad_jack = (krad_jack_t *)arg;

  printke ("Krad Jack shutdown callback, oh dear!");

}

void krad_jack_destroy (krad_jack_t *krad_jack) {

	int p;
	jack_client_close (krad_jack->client);
	
	krad_mixer_unset_pusher (krad_jack->krad_audio->krad_mixer);
	
	pthread_mutex_destroy (&krad_jack->connections_lock);
	
	for (p = 0; p < 256; p++) {
		if (krad_jack->stay_connected[p] != NULL) {
			free (krad_jack->stay_connected[p]);
		}
		if (krad_jack->stay_connected_to[p] != NULL) {
			free (krad_jack->stay_connected_to[p]);
		}		
	}
	
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

  krad_system_set_thread_name ("kr_jack");

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

	pthread_mutex_init (&krad_jack->connections_lock, NULL);

	// Set up Callbacks

	jack_set_process_callback (krad_jack->client, krad_jack_process, krad_jack);
	jack_on_shutdown (krad_jack->client, krad_jack_shutdown, krad_jack);
  jack_set_xrun_callback (krad_jack->client, krad_jack_xrun, krad_jack);
	//jack_set_port_registration_callback ( krad_jack->client, krad_jack_port_registration_callback, krad_jack );
	//jack_set_port_connect_callback ( krad_jack->client, krad_jack_port_connection_callback, krad_jack );

	// Activate

	if (jack_activate (krad_jack->client)) {
		failfast ("cannot activate client\n");
	}

	krad_jack->active = 1;

	return krad_jack;

}

