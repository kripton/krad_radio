#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include <samplerate.h>
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
#define KRAD_DEFAULT_OPUS_BITRATE 128000
#define DEFAULT_OPUS_FRAME_SIZE 960
#define KRAD_MIN_OPUS_FRAME_SIZE 120
#define MAX_OPUS_FRAME_SIZE 2880
#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE 2000000
#endif
#define DEFAULT_CHANNEL_COUNT 2
#define MAX_CHANNELS 8
// SRC_SINC_BEST_QUALITY SRC_SINC_MEDIUM_QUALITY SRC_SINC_FASTEST SRC_ZERO_ORDER_HOLD SRC_LINEAR
// #define KRAD_OPUS_SRC_QUALITY SRC_SINC_BEST_QUALITY
#define KRAD_OPUS_SRC_QUALITY SRC_SINC_MEDIUM_QUALITY


typedef struct krad_opus_St krad_opus_t;

struct krad_opus_St {

	OpusMSDecoder *decoder;
	OpusMSEncoder *encoder;
	OpusHeader *opus_header;
	krad_codec_header_t krad_codec_header;
	unsigned char *opustags_header;
	int opus_decoder_error;
		
	float *resampled_samples[MAX_CHANNELS];
	float *samples[MAX_CHANNELS];
	float *read_samples[MAX_CHANNELS];
	float *interleaved_samples;
	float interleaved_resampled_samples[MAX_OPUS_FRAME_SIZE * MAX_CHANNELS];
	krad_ringbuffer_t *resampled_ringbuf[MAX_CHANNELS];

	krad_ringbuffer_t *ringbuf[MAX_CHANNELS];
	int ret;
	int channels;
	
	int streams;
	int coupled_streams;
	
	int err;
	int extra_samples;

	unsigned char mapping[256];
	opus_int32 lookahead;


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
	
	unsigned char header_data[100];
	int header_data_size;
	SRC_STATE *src_resampler[MAX_CHANNELS];
	SRC_DATA src_data[MAX_CHANNELS];
	int src_error[MAX_CHANNELS];
};


/* Encoding */

int krad_opus_get_bitrate (krad_opus_t *krad_opus);
int krad_opus_get_complexity (krad_opus_t *krad_opus);
int krad_opus_get_frame_size (krad_opus_t *krad_opus);
int krad_opus_get_signal (krad_opus_t *krad_opus);
int krad_opus_get_bandwidth (krad_opus_t *krad_opus);

void krad_opus_set_complexity (krad_opus_t *krad_opus, int complexity);
void krad_opus_set_frame_size (krad_opus_t *krad_opus, int frame_size);
void krad_opus_set_bitrate (krad_opus_t *krad_opus, int bitrate);
void krad_opus_set_signal (krad_opus_t *krad_opus, int signal);
void krad_opus_set_bandwidth (krad_opus_t *krad_opus, int bandwidth);

int krad_opus_encoder_read (krad_opus_t *krad_opus, unsigned char *buffer, int *nframes);
int krad_opus_encoder_write (krad_opus_t *krad_opus, int channel, char *buffer, int buffer_length);
void krad_opus_encoder_destroy (krad_opus_t *krad_opus);
krad_opus_t *krad_opus_encoder_create (int channels, int input_sample_rate, int bitrate, int application);


/* Decoding */

krad_opus_t *krad_opus_decoder_create (unsigned char *header_data, int header_length, float output_sample_rate);
void krad_opus_decoder_destroy (krad_opus_t *krad_opus);
int krad_opus_decoder_read (krad_opus_t *krad_opus, int channel, char *buffer, int buffer_length);
int krad_opus_decoder_write (krad_opus_t *krad_opus, unsigned char *buffer, int length);
