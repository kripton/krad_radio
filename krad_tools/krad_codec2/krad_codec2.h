#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include "codec2.h"

#define KRAD_CODEC2_BYTES_PER_FRAME 7
#define KRAD_CODEC2_SAMPLES_PER_FRAME 160
#define KRAD_CODEC2_SAMPLE_RATE 8000

typedef struct krad_codec2_St krad_codec2_t;

struct krad_codec2_St {

	int input_channels;
	int input_sample_rate;
	int output_channels;
	int output_sample_rate;

	void *codec2;

};


int krad_codec2_encode(krad_codec2_t *codec2, float *audio, int frames, unsigned char *encode_buffer);
void krad_codec2_encoder_destroy(krad_codec2_t *codec2);
krad_codec2_t *krad_codec2_encoder_create(int channels, int input_sample_rate);

krad_codec2_t *krad_codec2_decoder_create(int output_sample_rate);
void krad_codec2_decoder_destroy(krad_codec2_t *codec2);
int krad_codec2_decode(krad_codec2_t *codec2, unsigned char *encoded_buffer, int len, float *audio);



void krad_codec2_decoder_int24_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_int16_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_info(krad_codec2_t *codec2);

