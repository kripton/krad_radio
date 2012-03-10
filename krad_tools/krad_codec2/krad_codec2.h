#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include "codec2.h"
#include <speex/speex_resampler.h>

#define KRAD_CODEC2_BYTES_PER_FRAME 7
#define KRAD_CODEC2_SAMPLES_PER_FRAME 160
#define KRAD_CODEC2_SAMPLE_RATE 8000

#define KRAD_CODEC2_USE_SPEEX_RESAMPLER 1

#define KRAD_CODEC2_SLICE_SIZE_MAX 4096

typedef struct krad_codec2_St krad_codec2_t;

struct krad_codec2_St {

	int input_channels;
	int input_sample_rate;
	int output_channels;
	int output_sample_rate;

	void *codec2;

	int16_t *short_samples_in;
	int16_t *short_samples_out;
	
	float *float_samples_in;
	float *float_samples_out;
	
	SpeexResamplerState *speex_resampler;
	int speex_resampler_error;
	uint32_t samples_in;
	uint32_t samples_out;
	
};

void krad_speex_resampler_info();

void krad_codec2_float_to_int16 (int16_t *dst,  float *src, int len);
void krad_codec2_int16_to_float (float *out, int16_t *in, int len);
void krad_linear_resample_8_48 (short * output, short * input, int n);
void krad_linear_resample_48_8 (short * output, short * input, int n);
void krad_smash (float *samples, int len);


int krad_codec2_encode(krad_codec2_t *codec2, float *audio, int frames, unsigned char *buffer);
void krad_codec2_encoder_destroy(krad_codec2_t *codec2);
krad_codec2_t *krad_codec2_encoder_create(int channels, int input_sample_rate);

krad_codec2_t *krad_codec2_decoder_create(int output_channels, int output_sample_rate);
void krad_codec2_decoder_destroy(krad_codec2_t *codec2);
int krad_codec2_decode(krad_codec2_t *codec2, unsigned char *buffer, int len, float *audio);



void krad_codec2_decoder_int24_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_int16_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_info(krad_codec2_t *codec2);

