#include "krad_audio.h"

void krad_audio_portgroup_samples_callback (int frames, void *userdata, float **samples) {

	krad_audio_portgroup_t *portgroup = (krad_audio_portgroup_t *)userdata;

	switch (portgroup->audio_api) {
	
		case JACK:
			krad_jack_portgroup_samples_callback ( frames, portgroup->api_portgroup, samples);
			break;
		case ALSA:
			break;
		case TONE:
			break;
		case NOAUDIO:
			break;
		case DECKLINKAUDIO:
			break;
	}
}


krad_audio_portgroup_t *krad_audio_portgroup_create (krad_audio_t *krad_audio, char *name, krad_audio_portgroup_direction_t direction,
													 int channels, krad_audio_api_t api) {


	krad_audio_portgroup_t *portgroup;
	int p;
	
	portgroup = NULL;
	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_audio->portgroup[p]->active == 0) {
			portgroup = krad_audio->portgroup[p];
			break;
		}
	}

	portgroup->krad_audio = krad_audio;
	portgroup->audio_api = api;
	portgroup->direction = direction;
	portgroup->channels = channels;
	
	strncpy (portgroup->name, name, 256);

	switch (portgroup->audio_api) {
	
		case JACK:
			if (krad_audio->krad_jack == NULL) {
				krad_audio->krad_jack = krad_jack_create (krad_audio);
        //restore thread name
        krad_system_set_thread_name ("kr_ipc_server");				
			}
			portgroup->api_portgroup = krad_jack_portgroup_create (krad_audio->krad_jack, portgroup->name, 
																   portgroup->direction, portgroup->channels);
			break;
		case ALSA:
			break;
		case TONE:		
			break;
		case NOAUDIO:
			break;
		case DECKLINKAUDIO:
			portgroup->sample_rate = 48000;
			break;
	}

	portgroup->active = 1;

	return portgroup;

}

void krad_audio_portgroup_destroy (krad_audio_portgroup_t *portgroup) {

	int p;
	int j;
	
	j = 0;
	portgroup->active = 2;

	switch (portgroup->audio_api) {
	
		case JACK:
			krad_jack_portgroup_destroy (portgroup->api_portgroup);
			// if there is no jack portgroups, disconnect from jack
			for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
				if ((portgroup->krad_audio->portgroup[p]->active != 0) &&
					(portgroup->krad_audio->portgroup[p] != portgroup) &&
					(portgroup->krad_audio->portgroup[p]->audio_api == JACK)) {
					j++;
					break;
				}
			}
			
			if ((j == 0) && (portgroup->krad_audio->krad_jack != NULL)) {
				krad_jack_destroy (portgroup->krad_audio->krad_jack);
				portgroup->krad_audio->krad_jack = NULL;
			}
				
			break;
		case ALSA:
			break;
		case TONE:		
			break;
		case NOAUDIO:
			break;
		case DECKLINKAUDIO:
			break;
	}

	portgroup->active = 0;

}


void krad_audio_destroy (krad_audio_t *krad_audio) {
	
	int p;

	krad_audio->destroy = 1;
	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_audio->portgroup[p]->active != 0) {
			krad_audio_portgroup_destroy ( krad_audio->portgroup[p] );
		}
	}	

	free (krad_audio);

}


krad_audio_t *krad_audio_create (krad_mixer_t *krad_mixer) {

	krad_audio_t *krad_audio;
	int p;
	
	if ((krad_audio = calloc (1, sizeof (krad_audio_t))) == NULL) {
		failfast ("Krad Audio memory alloc fail\n");
	}

	krad_audio->krad_mixer = krad_mixer;
		
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		krad_audio->portgroup[p] = calloc (1, sizeof (krad_audio_portgroup_t));
	}
		
	return krad_audio;

}
