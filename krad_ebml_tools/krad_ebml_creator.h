#include <krad_ebml.h>
#include <krad_opus.h>
#include <krad_vorbis.h>
#include <krad_audio.h>
#include <krad_vpx.h>
#include <krad_v4l2.h>

#define DEFAULT_DEVICE "/dev/video0"

typedef enum {
	STREAMING,
	STOPPED,
} krad_ebml_creator_state_t;

typedef enum {
	STREAM,
	STOP,
} krad_ebml_creator_command_t;

typedef struct krad_ebml_creator_St krad_ebml_creator_t;

struct krad_ebml_creator_St {

	krad_audio_t *audio;
	krad_ebml_t *ebml;

	krad_opus_t *opus;
	krad_vorbis_t *vorbis;

	int len;
	int callback_len;
	unsigned char buffer[8192];
	float *samples[2];
	float *callback_samples[2];
	
	char appname[512];
	char input[1024];

	krad_ebml_creator_command_t command;
	krad_ebml_creator_state_t state;
	
	int audiotrack;
	int videotrack;

	int video;

	pthread_t creator_thread;
};



void *krad_ebml_creator_thread(void *arg);
void krad_ebml_creator_stream_video(krad_ebml_creator_t *krad_ebml_creator, char *stream_url);
void krad_ebml_creator_stream(krad_ebml_creator_t *krad_ebml_creator, char *stream_url);
void krad_ebml_creator_stream_blocking(krad_ebml_creator_t *krad_ebml_creator, char *stream_url);

float krad_ebml_creator_get_stream_time_ms(krad_ebml_creator_t *krad_ebml_creator);
int krad_ebml_creator_get_state(krad_ebml_creator_t *krad_ebml_creator);
void krad_ebml_creator_stop(krad_ebml_creator_t *krad_ebml_creator);
void krad_ebml_creator_stop_wait(krad_ebml_creator_t *krad_ebml_creator);


krad_ebml_creator_t *krad_ebml_creator_create(char *appname, krad_audio_api_t api);
void krad_ebml_creator_destroy(krad_ebml_creator_t *krad_ebml_creator);



char *krad_ebml_creator_get_output_name (krad_ebml_creator_t *krad_ebml_creator);
char *krad_ebml_creator_get_output_type (krad_ebml_creator_t *krad_ebml_creator);
