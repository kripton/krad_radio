#include "hardlimiter.h"
#include "sidechain_comp.h"
#include "krad_ipc_server.h"
#include "FLAC/stream_encoder.h"
#include "FLAC/stream_decoder.h"

#include "krad_system.h"
#include "krad_ring.h"

#define MAX_SAMPLES 48
#define PORTGROUP_MAX 8
#define SAMPLE_RINGBUF_SIZE 8000000

// #include "kiss_fft.h"

typedef enum {
	OUTPUT,
	INPUT,
} portgroup_direction_t;

typedef enum {
	NIL,
	MONO,
	STEREO,
	THREE,
	QUAD,
	FIVE,
	SIX,
} portgroup_channel_t;

typedef enum {
	MIC,
	MUSIC,
	AUX,
	MAINMIX,
	SUBMIX,
	SYNTH,
	SAMPLER,
} portgroup_type_t;

typedef struct portgroup_St portgroup_t;
typedef struct krad_sampler_St krad_sampler_t;
typedef struct krad_sample_St krad_sample_t;
typedef struct kraddsp_St kraddsp_t;

struct krad_sample_St {

	char name[128];
	jack_ringbuffer_t *ringbuffer[2];
	int record;
	int play;
	int loop;
	int active;
	int seek;
	int write_seek;
	int length;
	int saving;
	int saved;
	int loading;
	pthread_t encoder_thread;
	pthread_t decoder_thread;
	FLAC__StreamEncoder *flac_encoder;
	FLAC__StreamDecoder *flac_decoder;
	char filename[256];
	krad_sampler_t *sampler;
	
	int fade_frame_duration;
	int current_sample[2];
	int fade; // 0 not, 1 in, 2 out
	float current_fade_amount[2];
	int lastsign[2];
	
	int startoffset;
	int stopoffset;
	int crossfade_frame_duration;
	int crossfade_sample_count;
	int crossfade_sample_count2;
	int crossfaded_samples_left;
	jack_default_audio_sample_t *crossfade_samples1[2];
	jack_default_audio_sample_t *crossfade_samples2[2];
	jack_default_audio_sample_t *crossfaded_samples[2];
	jack_default_audio_sample_t *samples[2];
};

struct krad_sampler_St {

	portgroup_t *input_portgroup;
	portgroup_t *output_portgroup;
	kraddsp_t *kraddsp;
	krad_sample_t *sample[MAX_SAMPLES];

};

struct portgroup_St {
	char name[256];
	portgroup_direction_t direction;
	portgroup_type_t type;
	portgroup_channel_t channels;
	int group;
	jack_port_t *input_port;
	jack_port_t *input_ports[8];
	jack_port_t *output_ports[8];
	float volume[8];
	float volume_actual[8];
	float new_volume_actual[8];
	int last_sign[8];
	
	int last_signx[8];
	
	float peak[8];
	jack_default_audio_sample_t *samples[8];
	
	int active;
	
	krad_synth_t *synth;
	krad_sampler_t *sampler;
	
	int monitor;
	portgroup_t *next;
};



struct kraddsp_St {

	krad_ipc_server_t *ipc;
	
	const char **ports;
	char client_name[256];
	char station_name[256];
	const char *server_name;
	jack_options_t options;
	jack_status_t status;
	jack_client_t *jack_client;

	int xruns;
	int sample_rate;
		
    void *userdata;
	portgroup_t *portgroup[PORTGROUP_MAX];
	
};



void destroy_portgroup (kraddsp_t *kraddsp, portgroup_t *portgroup);
portgroup_t *create_portgroup(kraddsp_t *kraddsp, const char *name, int direction, int channels, int type, int group);

int kraddsp_handler(char *data, void *pointer);
int setcontrol(kraddsp_client_t *client, char *data, float value);
void compute_peak(portgroup_t *portgroup, int channel, int sample_count);
void compute_peaks(portgroup_t *portgroup, int sample_count);
void listcontrols(kraddsp_client_t *client, char *data);

