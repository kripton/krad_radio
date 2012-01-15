#include <krad_audio.h>


void kradaudio_destroy(krad_audio_t *kradaudio) {

	switch (kradaudio->audio_api) {
	
		case JACK:
			kradjack_destroy(kradaudio->api);
			break;
		case ALSA:
			kradalsa_destroy(kradaudio->api);
			break;
		case PULSE:
			kradpulse_destroy(kradaudio->api);
			break;
	}
	
	jack_ringbuffer_free ( kradaudio->input_ringbuffer[0] );
	jack_ringbuffer_free ( kradaudio->input_ringbuffer[1] );

	jack_ringbuffer_free ( kradaudio->output_ringbuffer[0] );
	jack_ringbuffer_free ( kradaudio->output_ringbuffer[1] );
	
	free(kradaudio);

}


krad_audio_t *kradaudio_create(char *name, krad_audio_api_t api) {

	krad_audio_t *kradaudio;
	
	if ((kradaudio = calloc (1, sizeof (krad_audio_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
	kradaudio->output_ringbuffer[0] = jack_ringbuffer_create (RINGBUFFER_SIZE);
	kradaudio->output_ringbuffer[1] = jack_ringbuffer_create (RINGBUFFER_SIZE);

	kradaudio->input_ringbuffer[0] = jack_ringbuffer_create (RINGBUFFER_SIZE);
	kradaudio->input_ringbuffer[1] = jack_ringbuffer_create (RINGBUFFER_SIZE);

	strncpy(kradaudio->name, name, 128);
	kradaudio->name[127] = '\0';
	
	kradaudio->audio_api = api;


	switch (kradaudio->audio_api) {
	
		case JACK:
			kradaudio->api = kradjack_create(kradaudio);
			break;
		case ALSA:
			kradaudio->api = kradalsa_create(kradaudio);
			break;
		case PULSE:
			kradaudio->api = kradpulse_create(kradaudio);
			break;
	}
	
	return kradaudio;

}


void kradaudio_set_process_callback(krad_audio_t *kradaudio, void kradaudio_process_callback(int, void *), void *userdata) {

	kradaudio->process_callback = kradaudio_process_callback;
	kradaudio->userdata = userdata;

}


void kradaudio_write(krad_audio_t *kradaudio, int channel, char *data, int len) {

	jack_ringbuffer_write (kradaudio->output_ringbuffer[channel], data, len );

}



void kradaudio_read(krad_audio_t *kradaudio, int channel, char *data, int len) {

	jack_ringbuffer_read (kradaudio->input_ringbuffer[channel], data, len );

}

size_t kradaudio_buffered_input_frames(krad_audio_t *kradaudio) {

	return (jack_ringbuffer_read_space(kradaudio->input_ringbuffer[1]) / 4);

}

size_t kradaudio_buffered_frames(krad_audio_t *kradaudio) {

	return (jack_ringbuffer_read_space(kradaudio->output_ringbuffer[1]) / 4);

}
