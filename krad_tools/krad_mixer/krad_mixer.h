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
#define MAXCHANNELS 8

typedef struct krad_mixer_St krad_mixer_t;
typedef struct portgroup_St portgroup_t;
typedef struct portgroup_St mixbus_t;
typedef struct crossfadegroup_St crossfadegroup_t;

typedef enum {
	OUTPUT,
	INPUT,
	MIX,
} portgroup_direction_t;

typedef enum {
	JACK,
	KRAD_LINK,
	MIXBUS,
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

struct crossfadegroup_St {

	portgroup_t *portgroup[2];
	float fade;

};

struct portgroup_St {

	char name[256];
	
	portgroup_direction_t direction;
	portgroup_io_t io_type;
	channels_t channels;
	mixbus_t *mixbus;
	
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
    
	portgroup_t *portgroup[PORTGROUP_MAX];
	crossfadegroup_t *crossfadegroup[PORTGROUP_MAX];

	/*
	
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


char *krad_mixer_channel_number_to_string (int channel);

int krad_mixer_process (krad_mixer_t *krad_mixer, uint32_t frames);

krad_mixer_t *krad_mixer_create (char *callsign);
void krad_mixer_destroy (krad_mixer_t *krad_mixer);

int setcontrol(krad_mixer_t *client, char *data);
void listcontrols(krad_mixer_t *client, char *data);
int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc );



portgroup_t *krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, const char *name, int direction, int channels, mixbus_t *mixbus, portgroup_io_t io_type);
void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, portgroup_t *portgroup);

void compute_peak (portgroup_t *portgroup, int channel, int sample_count);
void compute_peaks (portgroup_t *portgroup, int sample_count);
float read_stereo_peak (portgroup_t *portgroup);
int krad_mixer_jack_xrun_callback (void *arg);
