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
typedef struct crossfade_group_St crossfade_group_t;

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

struct crossfade_group_St {

	portgroup_t *portgroup[2];
	float fade;

};

struct portgroup_St {

	char sysname[256];
	
	portgroup_direction_t direction;
	portgroup_io_t io_type;
	channels_t channels;
	mixbus_t *mixbus;
	crossfade_group_t *crossfade_group;
	
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
	
	char sysname[64];
	
	const char *jack_client_name;
	const char *jack_server_name;
	jack_options_t jack_options;
	jack_status_t jack_status;
	jack_client_t *jack_client;
	int jack_xruns;
	int jack_sample_rate;

	int shutdown;
    
	portgroup_t *portgroup[PORTGROUP_MAX];
	crossfade_group_t *crossfade_group;

};

int krad_mixer_process (krad_mixer_t *krad_mixer, uint32_t frames);
krad_mixer_t *krad_mixer_create (char *sysname);
void krad_mixer_destroy (krad_mixer_t *krad_mixer);

int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc );

portgroup_t *krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, char *sysname, int direction, int channels, mixbus_t *mixbus, portgroup_io_t io_type);
void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, portgroup_t *portgroup);
portgroup_t *krad_mixer_get_portgroup_from_sysname (krad_mixer_t *krad_mixer, char *sysname);

void krad_mixer_crossfade_group_create (krad_mixer_t *krad_mixer, portgroup_t *portgroup1, portgroup_t *portgroup2);
void krad_mixer_crossfade_group_destroy (krad_mixer_t *krad_mixer, crossfade_group_t *crossfade_group);
void crossfade_group_set_crossfade (crossfade_group_t *crossfade_group, float value);

void portgroup_update_volume (portgroup_t *portgroup);
void portgroup_set_channel_volume (portgroup_t *portgroup, int channel, float value);
void portgroup_set_volume (portgroup_t *portgroup, float value);
void portgroup_set_crossfade (portgroup_t *portgroup, float value);

char *krad_mixer_channel_number_to_string (int channel);
void compute_peak (portgroup_t *portgroup, int channel, uint32_t nframes);
void compute_peaks (portgroup_t *portgroup, uint32_t nframes);
float read_stereo_peak (portgroup_t *portgroup);
int krad_mixer_jack_xrun_callback (void *arg);
