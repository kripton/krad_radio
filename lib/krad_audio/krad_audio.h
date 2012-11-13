#ifndef KRAD_AUDIO_API
#define KRAD_AUDIO_API
typedef enum {
	NOAUDIO = 999,	
	JACK,
	ALSA,
	DECKLINKAUDIO,
	TONE,
} krad_audio_api_t;
#endif

#ifndef KRAD_AUDIO_H
#define KRAD_AUDIO_H

typedef struct krad_audio_St krad_audio_t;
typedef struct krad_audio_portgroup_St krad_audio_portgroup_t;

#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE 2000000
#endif

#ifndef __MACH__
#include "krad_alsa.h"
#endif
#include "krad_jack.h"
#include "krad_mixer.h"
#include "krad_mixer_common.h"

#include "krad_radio.h"

#include <stddef.h>

typedef enum {
	KOUTPUT,
	KINPUT,
	KDUPLEX,
} krad_audio_portgroup_direction_t;

struct krad_audio_portgroup_St {

	krad_audio_t *krad_audio;
	krad_audio_api_t audio_api;
	krad_audio_portgroup_direction_t direction;

	void *api_portgroup;

	char name[256];

	int channels;
	int active;
	
	int sample_rate;

	//krad_ringbuffer_t *input_ringbuffer[KRAD_MIXER_MAX_CHANNELS];
	//krad_ringbuffer_t *output_ringbuffer[KRAD_MIXER_MAX_CHANNELS];
	
	//pthread_t tone_generator_thread;
	//int run_tone;
	
};

struct krad_audio_St {

	krad_mixer_t *krad_mixer;
	
	krad_alsa_t *krad_alsa;
	krad_jack_t *krad_jack;
	
	krad_audio_portgroup_t *portgroup[KRAD_MIXER_MAX_PORTGROUPS];	
	
	int destroy;
	
	//void (*process_callback)(int, void *);
    
};

void krad_audio_portgroup_samples_callback (int frames, void *userdata, float **samples);

krad_audio_portgroup_t *krad_audio_portgroup_create (krad_audio_t *krad_audio, char *name, krad_audio_portgroup_direction_t direction, int channels, krad_audio_api_t api);
void krad_audio_portgroup_destroy (krad_audio_portgroup_t *portgroup);

//void krad_audio_set_process_callback (krad_audio_t *krad_audio, void krad_audio_process_callback(int, void *), void *userdata);
void krad_audio_destroy (krad_audio_t *krad_audio);
krad_audio_t *krad_audio_create (krad_mixer_t *krad_mixer);

/*
void krad_audio_write (krad_audio_t *krad_audio,  int channel, char *data, int len);
void krad_audio_read (krad_audio_t *krad_audio,  int channel, char *data, int len);
size_t krad_audio_buffered_frames (krad_audio_t *krad_audio);
size_t krad_audio_buffered_input_frames (krad_audio_t *krad_audio);
*/
#endif
