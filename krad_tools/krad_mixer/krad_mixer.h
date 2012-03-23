#include "hardlimiter.h"
#include "sidechain_comp.h"
#include "djeq.h"
#include "digilogue.h"
#include "pass.h"
#include "fastlimiter.h"

#include "krad_ipc_server.h"
#include "krad_tags.h"

#include <jack/jack.h>

#define PORTGROUP_MAX 20
#define MIXGROUP_MAX 20

typedef struct krad_mixer_St krad_mixer_t;
typedef struct portgroup_St portgroup_t;
typedef struct mixgroup_St mixgroup_t;

typedef enum {
	OUTPUT,
	INPUT,
} portgroup_direction_t;

typedef enum {
	JACK,
	INTERNAL,
} portgroup_io_t;

typedef enum {
	NIL,
	MONO,
	STEREO,
	THREE,
	QUAD,
	FIVE,
	SIX,
	SEVEN,
	EIGHT,
} channels_t;

struct mixgroup_St {

	char name[256];
	float *samples[8];
	channels_t channels;
	int active;
	krad_mixer_t *krad_mixer;
};

struct portgroup_St {

	char name[256];
	
	portgroup_direction_t direction;
	portgroup_io_t io_type;
	channels_t channels;
	mixgroup_t *mixgroup;
	
	jack_port_t *ports[8];
	
	float volume[8];
	float volume_actual[8];
	float new_volume_actual[8];
	int last_sign[8];

	float peak[8];
	float *samples[8];

	int active;
	
	krad_mixer_t *krad_mixer;
	krad_tags_t *krad_tags;

};


struct krad_mixer_St {
	
	char callsign[64];
	
	const char **ports;
	const char *client_name;
	const char *server_name;
	jack_options_t options;
	jack_status_t status;
	jack_client_t *jack_client;
	int shutdown;
	int xruns;
	int sample_rate;
    
	mixgroup_t *mixgroup[MIXGROUP_MAX];
	portgroup_t *portgroup[PORTGROUP_MAX];

	/*
	sc2_data_t *sc2_data;
	sc2_data_t *sc2_data2;

	float music_1_2_crossfade;
	float music_aux_crossfade;
	
	float new_music1_fade_value[8];
	float new_music2_fade_value[8];
	float new_music_fade_value[8];
	float new_aux_fade_value[8];
	
	float music1_fade_value[8];
	float music2_fade_value[8];
	
	float fade_temp;
	float fade_temp_pos;	
	
	jack_default_audio_sample_t micmix_left[1024];
	jack_default_audio_sample_t micmix_right[1024];
	
	int active_input;
	int last_active_input;
	int active_input_bytes;
	char active_input_data[2048];
	pthread_t active_input_thread;
	
	int level_bytes;
	char level_data[2048];
	float level_float_value;
	int level_pg;
	portgroup_t *level_portgroup;
	pthread_t levels_thread;
	*/
};


int krad_mixer_process (krad_mixer_t *krad_mixer, uint32_t frames);

krad_mixer_t *krad_mixer_create (char *callsign);
void krad_mixer_destroy (krad_mixer_t *krad_mixer);

int setcontrol(krad_mixer_t *client, char *data);
void listcontrols(krad_mixer_t *client, char *data);
int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc );



void krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, const char *name, int direction, int channels, mixgroup_t *mixgroup, portgroup_io_t io_type);
void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, portgroup_t *portgroup);

void compute_peak (portgroup_t *portgroup, int channel, int sample_count);
void compute_peaks (portgroup_t *portgroup, int sample_count);
float read_stereo_peak (portgroup_t *portgroup);
int krad_mixer_jack_xrun_callback (void *arg);
