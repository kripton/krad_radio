#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include <samplerate.h>
//#include <speex/speex_resampler.h>
#include <opus.h>
#include <opus_multistream.h>
#include <ogg/ogg.h>

#include "krad_radio_version.h"
#include "krad_system.h"
#include "opus_header.h"
#include "krad_ring.h"
#include "krad_codec_header.h"

// where did this come from?
#define OPUS_MAX_FRAME_BYTES 61295
#define DEFAULT_OPUS_COMPLEXITY 10
#define DEFAULT_OPUS_BITRATE 128000
#define DEFAULT_OPUS_FRAME_SIZE 960
#define MIN_OPUS_FRAME_SIZE 120
#define MAX_OPUS_FRAME_SIZE 2880
#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE 2000000
#endif
#define DEFAULT_CHANNEL_COUNT 2
#define MAX_CHANNELS 8
// SRC_SINC_BEST_QUALITY SRC_SINC_MEDIUM_QUALITY SRC_SINC_FASTEST SRC_ZERO_ORDER_HOLD SRC_LINEAR
#define KRAD_OPUS_SRC_QUALITY SRC_SINC_BEST_QUALITY

typedef struct kradopus_St krad_opus_t;

struct kradopus_St {

	int opus_decoder_error;
	OpusMSDecoder *decoder;

	OpusMSEncoder *st;
	OpusHeader *opus_header;
	krad_codec_header_t krad_codec_header;
	unsigned char *opustags_header;
		
	float *resampled_samples[MAX_CHANNELS];
	float *samples[MAX_CHANNELS];
	float *read_samples[MAX_CHANNELS];
	float *interleaved_samples;
	float interleaved_resampled_samples[MAX_OPUS_FRAME_SIZE * MAX_CHANNELS];
	krad_ringbuffer_t *resampled_ringbuf[MAX_CHANNELS];

	krad_ringbuffer_t *ringbuf[MAX_CHANNELS];
	int ret;
	int channels;
	
	int err;
	int extra_samples;

	unsigned char mapping[256];
	opus_int32 lookahead;

	char *st_string;
	float input_sample_rate;
	float output_sample_rate;

	int application;

	opus_int32 frame_size;	
	int complexity;
	int bitrate;
	int signal;
	int bandwidth;
		
	opus_int32 new_frame_size;	
	int new_complexity;
	int new_bitrate;
	int new_signal;
	int new_bandwidth;

	int id;
	int num_bytes;
	
	unsigned char bits[OPUS_MAX_FRAME_BYTES];	
	
	unsigned char header_data[100];
	int header_data_size;
	
	SRC_STATE *src_resampler[MAX_CHANNELS];
	SRC_DATA src_data[MAX_CHANNELS];
	int src_error[MAX_CHANNELS];	
	
};


int kradopus_get_bitrate (krad_opus_t *kradopus);
int kradopus_get_complexity (krad_opus_t *kradopus);
int kradopus_get_frame_size (krad_opus_t *kradopus);
int kradopus_get_signal (krad_opus_t *kradopus);
int kradopus_get_bandwidth (krad_opus_t *kradopus);

void kradopus_set_complexity (krad_opus_t *kradopus, int complexity);
void kradopus_set_frame_size (krad_opus_t *kradopus, int frame_size);
void kradopus_set_bitrate (krad_opus_t *kradopus, int bitrate);
void kradopus_set_signal (krad_opus_t *kradopus, int signal);
void kradopus_set_bandwidth (krad_opus_t *kradopus, int bandwidth);

krad_opus_t *kradopus_decoder_create (unsigned char *header_data, int header_length, float output_sample_rate);
void kradopus_decoder_destroy (krad_opus_t *kradopus);
int kradopus_read_audio (krad_opus_t *kradopus, int channel, char *buffer, int buffer_length);
int kradopus_write_opus (krad_opus_t *kradopus,  unsigned char *buffer, int length);

int kradopus_read_opus (krad_opus_t *kradopus, unsigned char *buffer, int *nframes);
int kradopus_write_audio (krad_opus_t *kradopus, int channel, char *buffer, int buffer_length);
void kradopus_encoder_destroy (krad_opus_t *kradopus);
krad_opus_t *kradopus_encoder_create (float input_sample_rate, int channels, int bitrate, int application);
