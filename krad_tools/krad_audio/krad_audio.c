#include <krad_audio.h>

void *tone_generator_thread (void *arg);



void krad_audio_destroy (krad_audio_t *krad_audio) {

	krad_audio->destroy = 1;
	/*
	switch (krad_audio->audio_api) {
	
		case JACK:
			kradjack_destroy(krad_audio->api);
			break;
		case ALSA:
			kradalsa_destroy(krad_audio->api);
			break;
		case PULSE:
			kradpulse_destroy(krad_audio->api);
			break;
		case TONE:
			krad_audio->run_tone = 0;
			pthread_join(krad_audio->tone_generator_thread, NULL);
			krad_tone_destroy(krad_audio->api);
			break;
		case NOAUDIO:
			break;
		case DECKLINKAUDIO:
			break;
	}
	
	if ((krad_audio->direction == KINPUT) || (krad_audio->direction == KDUPLEX)) {
		krad_ringbuffer_free ( krad_audio->input_ringbuffer[0] );
		krad_ringbuffer_free ( krad_audio->input_ringbuffer[1] );
	}
	
	if ((krad_audio->direction == KOUTPUT) || (krad_audio->direction == KDUPLEX)) {
		krad_ringbuffer_free ( krad_audio->output_ringbuffer[0] );
		krad_ringbuffer_free ( krad_audio->output_ringbuffer[1] );
	}
	*/
	free(krad_audio);

}


//krad_audio_t *krad_audio_create (char *name, krad_audio_direction_t direction, krad_audio_api_t api) {
krad_audio_t *krad_audio_create (krad_mixer_t *krad_mixer) {

	krad_audio_t *krad_audio;
	
	if ((krad_audio = calloc (1, sizeof (krad_audio_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}


	/*
	krad_audio->audio_api = api;
	krad_audio->direction = direction;
	
	if ((krad_audio->direction == KOUTPUT) || (krad_audio->direction == KDUPLEX)) {
	
		krad_audio->output_ringbuffer[0] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		krad_audio->output_ringbuffer[1] = krad_ringbuffer_create (RINGBUFFER_SIZE);
	
	}

	if ((krad_audio->direction == KINPUT) || (krad_audio->direction == KDUPLEX)) {
	
		krad_audio->input_ringbuffer[0] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		krad_audio->input_ringbuffer[1] = krad_ringbuffer_create (RINGBUFFER_SIZE);
	}

	strncpy(krad_audio->name, name, 128);
	krad_audio->name[127] = '\0';

	switch (krad_audio->audio_api) {
	
		case JACK:
			krad_audio->api = kradjack_create(krad_audio);
			break;
		case ALSA:
			krad_audio->api = kradalsa_create(krad_audio);
			break;
		case PULSE:
			krad_audio->api = kradpulse_create(krad_audio);
			break;
		case TONE:
			krad_audio->sample_rate = 48000;
			krad_audio->api = krad_tone_create(krad_audio->sample_rate);
			krad_tone_add_preset(krad_audio->api, "dialtone");
			krad_audio->run_tone = 1;
			pthread_create(&krad_audio->tone_generator_thread, NULL, tone_generator_thread, (void *)krad_audio);		
			break;
		case NOAUDIO:
			break;
		case DECKLINKAUDIO:
			krad_audio->sample_rate = 48000;
			break;
	}
	*/
	return krad_audio;

}

/*
		
struct timespec atimespec_diff (struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}
		
void *tone_generator_thread (void *arg) {

	krad_audio_t *krad_audio = (krad_audio_t *)arg;

	uint64_t count;
	int next_time_ms;
	float *tone_samples;
	int tone_period;

	struct timespec start_time;
	struct timespec current_time;
	struct timespec elapsed_time;
	struct timespec next_time;
	struct timespec sleep_time;

		krad_tone_clear(krad_audio->api);
		krad_tone_add_preset(krad_audio->api, preset);

	
	tone_period = 512;
	count = 0;
	tone_samples = malloc(4 * 8192 * 4);

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	while (krad_audio->run_tone) {

		if ((krad_ringbuffer_write_space(krad_audio->input_ringbuffer[0]) >= 4 * 8192 * 4) && 
			(krad_ringbuffer_write_space(krad_audio->input_ringbuffer[1]) >= 4 * 8192 * 4)) {
			
			krad_tone_run(krad_audio->api, tone_samples, 4 * 8192);
			krad_ringbuffer_write (krad_audio->input_ringbuffer[0], (char *)tone_samples, 4 * 8192 * 4 );
			krad_ringbuffer_write (krad_audio->input_ringbuffer[1], (char *)tone_samples, 4 * 8192 * 4 );
		}
	
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		elapsed_time = atimespec_diff(start_time, current_time);

		next_time_ms = count * (tone_period * (1000.0f / krad_audio->sample_rate));

		next_time.tv_sec = (next_time_ms - (next_time_ms % 1000)) / 1000;
		next_time.tv_nsec = (next_time_ms % 1000) * 1000000;

		sleep_time = atimespec_diff(elapsed_time, next_time);

		if ((sleep_time.tv_sec > -1) && (sleep_time.tv_nsec > 0))  {
			nanosleep (&sleep_time, NULL);
		}
			
		if (krad_audio->process_callback != NULL) {
			compute_peak(krad_audio, KINPUT, tone_samples, 0, tone_period, 0);
			compute_peak(krad_audio, KINPUT, tone_samples, 1, tone_period, 0);
			krad_audio->process_callback(tone_period, krad_audio->userdata);
		}
			
		//printf("sleeped for %zu ns \n", sleep_time.tv_nsec);
			
		count++;
				
	}	
			
	free (tone_samples);
				
	return NULL;
}

*/


/*
void krad_audio_set_process_callback (krad_audio_t *krad_audio, void krad_audio_process_callback(int, void *), void *userdata) {

	krad_audio->process_callback = krad_audio_process_callback;
	krad_audio->userdata = userdata;

}


void krad_audio_write (krad_audio_t *krad_audio, int channel, char *data, int len) {

	while (krad_ringbuffer_write_space(krad_audio->output_ringbuffer[channel]) < len) {
		if (krad_audio->destroy) {
			return;
		}
		usleep(10000);
	}

	krad_ringbuffer_write (krad_audio->output_ringbuffer[channel], data, len );

}



void krad_audio_read (krad_audio_t *krad_audio, int channel, char *data, int len) {

	krad_ringbuffer_read (krad_audio->input_ringbuffer[channel], data, len );

}

size_t krad_audio_buffered_input_frames (krad_audio_t *krad_audio) {

	return (krad_ringbuffer_read_space(krad_audio->input_ringbuffer[1]) / 4);

}

size_t krad_audio_buffered_frames (krad_audio_t *krad_audio) {

	return (krad_ringbuffer_read_space(krad_audio->output_ringbuffer[1]) / 4);

}

*/
