typedef struct krad_mixer_St krad_mixer_t;
typedef struct krad_mixer_portgroup_St krad_mixer_portgroup_t;
typedef struct krad_mixer_portgroup_St krad_mixer_mixbus_t;
typedef struct krad_mixer_crossfade_group_St krad_mixer_crossfade_group_t;

#define KRAD_MIXER_MAX_PORTGROUPS 20
#define KRAD_MIXER_MAX_CHANNELS 8

#include "krad_radio.h"

#ifndef KRAD_MIXER_H
#define KRAD_MIXER_H

#include "hardlimiter.h"



typedef enum {
	OUTPUT,
	INPUT,
	MIX,
} krad_mixer_portgroup_direction_t;

typedef enum {
	KRAD_AUDIO, /* i.e local audio i/o */
	KRAD_LINK, /* i.e. remote audio i/o */
	MIXBUS,	/* i.e. mixer internal i/o */
} krad_mixer_portgroup_io_t;

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

struct krad_mixer_crossfade_group_St {

	krad_mixer_portgroup_t *portgroup[2];
	float fade;

};

struct krad_mixer_portgroup_St {
	
	char sysname[256];
	krad_mixer_portgroup_direction_t direction;
	krad_mixer_portgroup_io_t io_type;
	void *io_ptr;
	channels_t channels;
	krad_mixer_mixbus_t *mixbus;
	krad_mixer_crossfade_group_t *crossfade_group;
	
	float volume[KRAD_MIXER_MAX_CHANNELS];
	float volume_actual[KRAD_MIXER_MAX_CHANNELS];
	float new_volume_actual[KRAD_MIXER_MAX_CHANNELS];
	int last_sign[KRAD_MIXER_MAX_CHANNELS];

	float peak[KRAD_MIXER_MAX_CHANNELS];
	float *samples[KRAD_MIXER_MAX_CHANNELS];

	int active;
	
	krad_mixer_t *krad_mixer;
	krad_tags_t *krad_tags;

};


struct krad_mixer_St {

	krad_radio_t *krad_radio;
	krad_audio_t *krad_audio;
    
	krad_mixer_mixbus_t *master_mix;
    
	krad_mixer_portgroup_t *portgroup[KRAD_MIXER_MAX_PORTGROUPS];
	krad_mixer_crossfade_group_t *crossfade_group;

};

int krad_mixer_process (uint32_t nframes, krad_mixer_t *krad_mixer);
krad_mixer_t *krad_mixer_create (krad_radio_t *krad_radio);
void krad_mixer_destroy (krad_mixer_t *krad_mixer);

int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc );

krad_mixer_portgroup_t *krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, char *sysname, int direction, int channels, 
													 krad_mixer_mixbus_t *mixbus, krad_mixer_portgroup_io_t io_type, void *io_ptr, krad_audio_api_t api);
void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup);
krad_mixer_portgroup_t *krad_mixer_get_portgroup_from_sysname (krad_mixer_t *krad_mixer, char *sysname);

void krad_mixer_crossfade_group_create (krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup1, krad_mixer_portgroup_t *portgroup2);
void krad_mixer_crossfade_group_destroy (krad_mixer_t *krad_mixer, krad_mixer_crossfade_group_t *crossfade_group);
void crossfade_group_set_crossfade (krad_mixer_crossfade_group_t *crossfade_group, float value);

void portgroup_update_volume (krad_mixer_portgroup_t *portgroup);
void portgroup_set_channel_volume (krad_mixer_portgroup_t *portgroup, int channel, float value);
void portgroup_set_volume (krad_mixer_portgroup_t *portgroup, float value);
void portgroup_set_crossfade (krad_mixer_portgroup_t *portgroup, float value);

char *krad_mixer_channel_number_to_string (int channel);
void krad_mixer_portgroup_compute_channel_peak (krad_mixer_portgroup_t *portgroup, int channel, uint32_t nframes);
void krad_mixer_portgroup_compute_peaks (krad_mixer_portgroup_t *portgroup, uint32_t nframes);
float krad_mixer_portgroup_read_peak (krad_mixer_portgroup_t *portgroup);
//int krad_mixer_jack_xrun_callback (void *arg);
#endif
