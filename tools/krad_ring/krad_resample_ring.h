#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include <samplerate.h>

#include "krad_system.h"
#include "krad_ring.h"

#define KRAD_RESAMPLE_RING_SRC_QUALITY SRC_SINC_MEDIUM_QUALITY

typedef struct krad_resample_ring_St krad_resample_ring_t;

struct krad_resample_ring_St {

	uint32_t size;
	int input_sample_rate;
	int output_sample_rate;

	krad_ringbuffer_t *krad_ringbuffer;

	float speed;

	SRC_STATE *src_resampler;
	SRC_DATA src_data;
	int src_error;

	unsigned char *outbuffer;
	unsigned char *inbuffer;
	int inbuffer_pos;
};



void krad_resample_ring_destroy (krad_resample_ring_t *krad_resample_ring);
krad_resample_ring_t *krad_resample_ring_create (uint32_t size, int input_sample_rate, int output_sample_rate);

void krad_resample_ring_set_input_sample_rate (krad_resample_ring_t *krad_resample_ring, int input_sample_rate);

uint32_t krad_resample_ring_read_space (krad_resample_ring_t *krad_resample_ring);
uint32_t krad_resample_ring_write_space (krad_resample_ring_t *krad_resample_ring);

uint32_t krad_resample_ring_read (krad_resample_ring_t *krad_resample_ring, unsigned char *dest, uint32_t cnt);
uint32_t krad_resample_ring_write (krad_resample_ring_t *krad_resample_ring, unsigned char *src, uint32_t cnt);
