#include "krad_ebml_player.h"

krad_ebml_player_t *krad_ebml_player_create(krad_audio_api_t api) {

	krad_ebml_player_t *krad_ebml_player = calloc(1, sizeof(krad_ebml_player_t));
	
	krad_ebml_player->samples[0] = malloc(4 * 8192);
	krad_ebml_player->samples[1] = malloc(4 * 8192);

	krad_ebml_player->callback_samples[0] = malloc(4 * 8192);
	krad_ebml_player->callback_samples[1] = malloc(4 * 8192);

	krad_ebml_player->audio = kradaudio_create("Krad EBML Player", api);
	krad_ebml_player->ebml = kradebml_create();
	
	krad_ebml_player->speed = 100.0f;
	
	return krad_ebml_player;

}


void krad_ebml_player_destroy(krad_ebml_player_t *krad_ebml_player) {

	
	krad_ebml_player_stop(krad_ebml_player);
	
	
	kradaudio_destroy(krad_ebml_player->audio);
	
	free(krad_ebml_player->samples[0]);
	free(krad_ebml_player->samples[1]);
	
	free(krad_ebml_player->callback_samples[0]);
	free(krad_ebml_player->callback_samples[1]);
	
	kradebml_destroy(krad_ebml_player->ebml);

	free(krad_ebml_player);

}

char *krad_ebml_player_get_input_name (krad_ebml_player_t *krad_ebml_player) {

	return krad_ebml_player->ebml->input_name;

}

char *krad_ebml_player_get_input_type (krad_ebml_player_t *krad_ebml_player) {

	switch (krad_ebml_player->ebml->input_type) {
	
		case KRADEBML_FILE:
			return "File";
			break;
		case KRADEBML_STREAM:
			return "Stream";
			break;
		case KRADEBML_BUFFER:
			return "Buffer";
			break;
	}
	
	return "";

}

void krad_ebml_player_stop(krad_ebml_player_t *krad_ebml_player) {

	krad_ebml_player->command = STOP;

}

void krad_ebml_player_stop_wait(krad_ebml_player_t *krad_ebml_player) {

	krad_ebml_player->command = STOP;

	pthread_join(krad_ebml_player->player_thread, NULL);

}

void krad_ebml_player_wait(krad_ebml_player_t *krad_ebml_player) {
	
	pthread_join(krad_ebml_player->player_thread, NULL);

}

int krad_ebml_player_get_state(krad_ebml_player_t *krad_ebml_player) {

	return krad_ebml_player->state;

}

float krad_ebml_player_get_playback_time_ms(krad_ebml_player_t *krad_ebml_player) {

	//printf("playback ms is %f\n", krad_ebml_player->playback_time_ms);

	return krad_ebml_player->playback_time_ms;

}


void krad_ebml_player_speed_up(krad_ebml_player_t *krad_ebml_player) {

	krad_ebml_player->speed += 12.0;

}

void krad_ebml_player_speed_down(krad_ebml_player_t *krad_ebml_player) {

	krad_ebml_player->speed -= 12.0;

}

void krad_ebml_player_set_speed(krad_ebml_player_t *krad_ebml_player, float speed) {

	krad_ebml_player->speed = speed;

}

float krad_ebml_player_get_speed(krad_ebml_player_t *krad_ebml_player) {

	return krad_ebml_player->speed;

}


void *krad_ebml_player_thread(void *arg) {

	krad_ebml_player_t *krad_ebml_player = (krad_ebml_player_t *)arg;

	krad_ebml_player_play_file_blocking(krad_ebml_player, krad_ebml_player->input);

	return NULL;

}

void krad_ebml_player_play_file(krad_ebml_player_t *krad_ebml_player, char *filename) {

	strncpy(krad_ebml_player->input, filename, 1024);

	krad_ebml_player->command = PLAY;
	krad_ebml_player->state = PLAYING;

	pthread_create( &krad_ebml_player->player_thread, NULL, krad_ebml_player_thread, krad_ebml_player);

}

void krad_ebml_player_audio_callback(int frames, void *userdata) {

	krad_ebml_player_t *krad_ebml_player = (krad_ebml_player_t *)userdata;

	if (kradopus_decoder_get_speed(krad_ebml_player->opus) != krad_ebml_player->speed) {
		kradopus_decoder_set_speed(krad_ebml_player->opus, krad_ebml_player->speed);
	}

	//printf("wanted %d frames\n", frames);

	krad_ebml_player->callback_len = kradopus_read_audio(krad_ebml_player->opus, 1, (char *)krad_ebml_player->callback_samples[0], frames * 4);

	if (krad_ebml_player->callback_len) {
		kradaudio_write (krad_ebml_player->audio, 0, (char *)krad_ebml_player->callback_samples[0], krad_ebml_player->callback_len );
	}

	krad_ebml_player->callback_len = kradopus_read_audio(krad_ebml_player->opus, 2, (char *)krad_ebml_player->callback_samples[1], frames * 4);

	if (krad_ebml_player->callback_len) {
		kradaudio_write (krad_ebml_player->audio, 1, (char *)krad_ebml_player->callback_samples[1], krad_ebml_player->callback_len );
	}
	
	krad_ebml_player->playback_time_ms += ((frames / 44.1) * (krad_ebml_player->speed * 0.01));
	
	
}


void krad_ebml_player_play_file_blocking(krad_ebml_player_t *krad_ebml_player, char *filename) {

	krad_ebml_player->command = PLAY;
	krad_ebml_player->state = PLAYING;
	
	kradebml_open_input_file(krad_ebml_player->ebml, filename);
	//kradebml_open_input_stream(krad_ebml_player->ebml, "192.168.1.2", 9080, "/teststream.krado");
	
	kradebml_debug(krad_ebml_player->ebml);
	
	krad_ebml_player->len = kradebml_read_audio_header(krad_ebml_player->ebml, 1, krad_ebml_player->buffer);
	
	krad_ebml_player->opus = kradopus_decoder_create(krad_ebml_player->buffer, krad_ebml_player->len, 44100.0f);

	kradaudio_set_process_callback(krad_ebml_player->audio, krad_ebml_player_audio_callback, krad_ebml_player);
	
	while (((krad_ebml_player->len = kradebml_read_audio(krad_ebml_player->ebml, krad_ebml_player->buffer)) > 0) && (krad_ebml_player->command != STOP)) {
		
		kradopus_write_opus(krad_ebml_player->opus, krad_ebml_player->buffer, krad_ebml_player->len);

		while (jack_ringbuffer_read_space (krad_ebml_player->opus->ringbuf[DEFAULT_CHANNEL_COUNT - 1]) >= (RINGBUFFER_SIZE - 960 * 4 * 10)) {
			usleep(50000);
		}

	}
	
	while (krad_ebml_player->command != STOP) {
	 
		usleep(50000);
	 
	}
	
	kradopus_decoder_destroy(krad_ebml_player->opus);

	krad_ebml_player->state = STOPPED;

}



