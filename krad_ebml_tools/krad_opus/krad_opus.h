#include "opus_header.h"

#include <speex/speex_resampler.h>
#include <opus.h>
#include <opus_multistream.h>
#include <ogg/ogg.h>

#define OPUS_MAX_FRAME_SIZE (960*6)
#define OPUS_MAX_FRAME_BYTES 61295


#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#define RINGBUFFER_SIZE 3000000
#define DEFAULT_CHANNEL_COUNT 2
#define MAX_CHANNELS 8

typedef struct kradopus_St krad_opus_t;

struct kradopus_St {


   int opus_decoder_error;
   OpusMSDecoder *decoder;


	OpusMSEncoder *st;
	OpusHeader *opus_header;
	SpeexResamplerState *resampler;	
	float *resampled_samples[MAX_CHANNELS];
	float *samples[MAX_CHANNELS];
	float *read_samples[MAX_CHANNELS];
	float *interleaved_samples;
	float interleaved_resampled_samples[960 * 2];
	jack_ringbuffer_t *resampled_ringbuf[MAX_CHANNELS];

	jack_ringbuffer_t *ringbuf[MAX_CHANNELS];
	int ret;
	int bitrate;
	int channels;
	
	int err;
	int extra_samples;

	opus_int32 frame_size;

	unsigned char mapping[256];
	opus_int32 lookahead;

	char *st_string;
	float input_sample_rate;
	float output_sample_rate;
	float speed;
	int mode;

	int id;
	int num_bytes;
	
	unsigned char bits[OPUS_MAX_FRAME_BYTES];	
	
	unsigned char header_data[100];
	int header_data_size;
	
};

void kradopus_decoder_set_speed(krad_opus_t *kradopus, float speed);
float kradopus_decoder_get_speed(krad_opus_t *kradopus);


krad_opus_t *kradopus_decoder_create(unsigned char *header_data, int header_length, float output_sample_rate);
void kradopus_decoder_destroy(krad_opus_t *kradopus);
int kradopus_read_audio(krad_opus_t *kradopus, int channel, char *buffer, int buffer_length);
int kradopus_write_opus(krad_opus_t *kradopus,  unsigned char *buffer, int length);

int kradopus_read_opus(krad_opus_t *kradopus, unsigned char *buffer);
int kradopus_write_audio(krad_opus_t *kradopus, int channel, char *buffer, int buffer_length);
void kradopus_encoder_destroy(krad_opus_t *kradopus);
krad_opus_t *kradopus_encoder_create(float input_sample_rate, int channels, int bitrate, int mode);
