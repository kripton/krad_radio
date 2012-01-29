#include <krad_ebml.h>

#include "krad_opus.h"
#include "krad_audio.h"

typedef enum {
	STOPPED,
	PAUSED,
	PLAYING,
} krad_ebml_player_state_t;

typedef enum {
	STOP,
	PAUSE,
	PLAY,
} krad_ebml_player_command_t;

typedef struct krad_ebml_player_St krad_ebml_player_t;

struct krad_ebml_player_St {

	int stop;

	krad_audio_t *audio;
	kradebml_t *ebml;
	krad_opus_t *opus;
	int len;
	int callback_len;
	unsigned char buffer[8192];
	float *samples[2];
	float *callback_samples[2];
	
	char input[1024];
	pthread_t player_thread;
	krad_ebml_player_state_t state;
	krad_ebml_player_command_t command;
	
	float playback_time_ms;
	float speed;
};


void krad_ebml_player_speed_up(krad_ebml_player_t *krad_ebml_player);
void krad_ebml_player_speed_down(krad_ebml_player_t *krad_ebml_player);


void krad_ebml_player_play_file(krad_ebml_player_t *krad_ebml_player, char *filename);

void krad_ebml_player_set_speed(krad_ebml_player_t *krad_ebml_player, float speed);
float krad_ebml_player_get_speed(krad_ebml_player_t *krad_ebml_player);

void *krad_ebml_player_thread(void *arg);
void krad_ebml_player_play_file(krad_ebml_player_t *krad_ebml_player, char *filename);
void krad_ebml_player_play_file_blocking(krad_ebml_player_t *krad_ebml_player, char *filename);

float krad_ebml_player_get_playback_time_ms(krad_ebml_player_t *krad_ebml_player);
int krad_ebml_player_get_state(krad_ebml_player_t *krad_ebml_player);
void krad_ebml_player_stop(krad_ebml_player_t *krad_ebml_player);
void krad_ebml_player_stop_wait(krad_ebml_player_t *krad_ebml_player);
void krad_ebml_player_wait(krad_ebml_player_t *krad_ebml_player);

krad_ebml_player_t *krad_ebml_player_create();
void krad_ebml_player_destroy(krad_ebml_player_t *krad_ebml_player);



char *krad_ebml_player_get_input_name (krad_ebml_player_t *krad_ebml_player);
char *krad_ebml_player_get_input_type (krad_ebml_player_t *krad_ebml_player);
