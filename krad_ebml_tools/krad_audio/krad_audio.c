#include <krad_audio.h>

float iec_scale(float db) {
	
	float def;
	
	if (db < -70.0f) {
		def = 0.0f;
	} else if (db < -60.0f) {
		def = (db + 70.0f) * 0.25f;
	} else if (db < -50.0f) {
		def = (db + 60.0f) * 0.5f + 2.5f;
	} else if (db < -40.0f) {
		def = (db + 50.0f) * 0.75f + 7.5;
	} else if (db < -30.0f) {
		def = (db + 40.0f) * 1.5f + 15.0f;
	} else if (db < -20.0f) {
		def = (db + 30.0f) * 2.0f + 30.0f;
	} else if (db < 0.0f) {
		def = (db + 20.0f) * 2.5f + 50.0f;
	} else {
		def = 100.0f;
	}
	
	return def;
}

float read_peak(krad_audio_t *kradaudio, krad_audio_direction_t direction, int channel)
{
	float tmp, db, peak;
	
	if (direction == KOUTPUT) {
		tmp = kradaudio->output_peak[channel];
		kradaudio->output_peak[channel] = 0.0f;
	} else {
		tmp = kradaudio->input_peak[channel];
		kradaudio->input_peak[channel] = 0.0f;
	}
	
	db = 20.0f * log10f(tmp * 1.0f);
	peak = iec_scale(db);
	
	return peak;
}

void compute_peak(krad_audio_t *kradaudio, krad_audio_direction_t direction, float *samples, int channel, int sample_count, int interleaved) {

	int s;


	if (direction == KOUTPUT) {
		for(s = 0; s < sample_count; s = s + 1 + interleaved) {
			const float sample = fabs(samples[s]);
			if (sample > kradaudio->output_peak[channel]) {
				kradaudio->output_peak[channel] = sample;
			}
		}
	} else {
		for(s = 0; s < sample_count; s = s + 1 + interleaved) {
			const float sample = fabs(samples[s]);
			if (sample > kradaudio->input_peak[channel]) {
				kradaudio->input_peak[channel] = sample;
			}
		}
	}

//	printf("peak %f %f\n", kradaudio->output_peak[0], kradaudio->output_peak[1]);

}

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
		case TONE:
			krad_tone_destroy(kradaudio->api);
			break;
		case NOAUDIO:
			break;
	}
	
	krad_ringbuffer_free ( kradaudio->input_ringbuffer[0] );
	krad_ringbuffer_free ( kradaudio->input_ringbuffer[1] );

	krad_ringbuffer_free ( kradaudio->output_ringbuffer[0] );
	krad_ringbuffer_free ( kradaudio->output_ringbuffer[1] );
	
	free(kradaudio);

}


krad_audio_t *kradaudio_create(char *name, krad_audio_direction_t direction, krad_audio_api_t api) {

	krad_audio_t *kradaudio;
	
	if ((kradaudio = calloc (1, sizeof (krad_audio_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
	kradaudio->output_ringbuffer[0] = krad_ringbuffer_create (RINGBUFFER_SIZE);
	kradaudio->output_ringbuffer[1] = krad_ringbuffer_create (RINGBUFFER_SIZE);

	kradaudio->input_ringbuffer[0] = krad_ringbuffer_create (RINGBUFFER_SIZE);
	kradaudio->input_ringbuffer[1] = krad_ringbuffer_create (RINGBUFFER_SIZE);

	strncpy(kradaudio->name, name, 128);
	kradaudio->name[127] = '\0';
	
	kradaudio->audio_api = api;
	kradaudio->direction = direction;

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
		case TONE:
			kradaudio->sample_rate = 48000;
			kradaudio->api = krad_tone_create(kradaudio->sample_rate);
			krad_tone_add_preset(kradaudio->api, "dialtone");
			break;
		case NOAUDIO:
			break;
	}
	
	return kradaudio;

}


///		krad_tone_run(krad_tone, audio, 4096);

void kradaudio_set_process_callback(krad_audio_t *kradaudio, void kradaudio_process_callback(int, void *), void *userdata) {

	kradaudio->process_callback = kradaudio_process_callback;
	kradaudio->userdata = userdata;

}


void kradaudio_write(krad_audio_t *kradaudio, int channel, char *data, int len) {

	krad_ringbuffer_write (kradaudio->output_ringbuffer[channel], data, len );

}



void kradaudio_read(krad_audio_t *kradaudio, int channel, char *data, int len) {

	krad_ringbuffer_read (kradaudio->input_ringbuffer[channel], data, len );

}

size_t kradaudio_buffered_input_frames(krad_audio_t *kradaudio) {

	return (krad_ringbuffer_read_space(kradaudio->input_ringbuffer[1]) / 4);

}

size_t kradaudio_buffered_frames(krad_audio_t *kradaudio) {

	return (krad_ringbuffer_read_space(kradaudio->output_ringbuffer[1]) / 4);

}
