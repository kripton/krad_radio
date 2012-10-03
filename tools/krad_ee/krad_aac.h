#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include <aacplus.h>

#include "krad_system.h"
#include "krad_codec_header.h"

typedef struct krad_aac_St krad_aac_t;

struct krad_aac_St {

	int channels;
	int sample_rate;
	int bitrate;

	unsigned long input_samples;
	unsigned long max_output_bytes;

	aacplusEncHandle encoder;
	aacplusEncConfiguration *config;

	unsigned char *header;
	unsigned long header_len;
	krad_codec_header_t krad_codec_header;

};

void krad_aac_encoder_destroy (krad_aac_t *krad_aac);
int krad_aac_encode (krad_aac_t *krad_aac, float *audio, int frames, unsigned char *encode_buffer);
krad_aac_t *krad_aac_encoder_create (int channels, int sample_rate, int bitrate);
