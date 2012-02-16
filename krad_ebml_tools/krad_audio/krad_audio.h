#ifndef KRAD_AUDIO_H
#define KRAD_AUDIO_H

#define RINGBUFFER_SIZE 3000000

typedef enum {
	JACK = 999,
	PULSE,
	ALSA,
	TONE,
	NOAUDIO,
} krad_audio_api_t;

typedef enum {
	KOUTPUT,
	KINPUT,
	KDUPLEX,
} krad_audio_direction_t;

#ifndef KRAD_CODEC_T
typedef enum {
	VORBIS = 6666,
	OPUS,
	FLAC,
	VP8,
	DIRAC,
	THEORA,
	NOCODEC,
} krad_codec_t;
#define KRAD_CODEC_T 1
#endif

#include <krad_alsa.h>
#include <krad_jack.h>
#include <krad_pulse.h>
#include <krad_ring.h>
#include <krad_tone.h>

#include <stddef.h>

typedef struct krad_audio_St krad_audio_t;

struct krad_audio_St {

	krad_audio_api_t audio_api;
	krad_audio_direction_t direction;
	char name[256];

	void *api;
	
	int sample_rate;

	krad_ringbuffer_t *input_ringbuffer[2];
	krad_ringbuffer_t *output_ringbuffer[2];

	float input_peak[8];
	float output_peak[8];
	
	void (*process_callback)(int, void *);
    void *userdata;
    
	pthread_t tone_generator_thread;
	int run_tone;
    
};

float read_peak(krad_audio_t *kradaudio, krad_audio_direction_t direction, int channel);
void compute_peak(krad_audio_t *kradaudio, krad_audio_direction_t direction, float *samples, int channel, int sample_count, int interleaved);

void kradaudio_set_process_callback(krad_audio_t *kradaudio, void kradaudio_process_callback(int, void *), void *userdata);
void kradaudio_destroy(krad_audio_t *kradaudio);
krad_audio_t *kradaudio_create(char *name, krad_audio_direction_t direction, krad_audio_api_t api);
void kradaudio_write(krad_audio_t *kradaudio,  int channel, char *data, int len);
void kradaudio_read(krad_audio_t *kradaudio,  int channel, char *data, int len);
size_t kradaudio_buffered_frames(krad_audio_t *kradaudio);
size_t kradaudio_buffered_input_frames(krad_audio_t *kradaudio);
#endif

typedef struct krad_audio_St krad_audio_t;
