#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include <vorbis/vorbisenc.h>

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_ring.h"
#include "krad_codec_header.h"

#define KRAD_DEFAULT_VORBIS_FRAME_SIZE 1024

#define MAX_CHANNELS 8
#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE 2000000
#endif

typedef struct krad_vorbis_St krad_vorbis_t;

struct krad_vorbis_St {

	int channels;
	float sample_rate;
	float quality;

	krad_ringbuffer_t *ringbuf[MAX_CHANNELS];

	uint64_t frames_encoded;

	ogg_sync_state oy;

	ogg_stream_state oggstate;
	vorbis_dsp_state vdsp;
	vorbis_block vblock;
	vorbis_info vinfo;
	vorbis_comment vc;
	int ret;
	int eos;
	int bytes;
	ogg_packet op;
	ogg_page og;
	int samples_since_flush;
	int last_flush_samples;
	float **buffer;
	krad_codec_header_t krad_codec_header;
	ogg_packet header_main;
	ogg_packet header_comments;
	ogg_packet header_codebooks;
	float samples[2048];
	unsigned char header[8192];
	int headerpos;
	int head_count;

};

int krad_vorbis_encoder_read (krad_vorbis_t *krad_vorbis, int *out_frames, unsigned char **buffer);
void krad_vorbis_encoder_prepare (krad_vorbis_t *krad_vorbis, int frames, float ***buffer);
int krad_vorbis_encoder_wrote (krad_vorbis_t *krad_vorbis, int frames);
int krad_vorbis_encoder_write (krad_vorbis_t *krad_vorbis, float **samples, int frames);
int krad_vorbis_encoder_finish (krad_vorbis_t *krad_vorbis);
void krad_vorbis_encoder_destroy (krad_vorbis_t *krad_vorbis);
krad_vorbis_t *krad_vorbis_encoder_create (int channels, int sample_rate, float quality);


void krad_vorbis_decoder_destroy(krad_vorbis_t *vorbis);
krad_vorbis_t *krad_vorbis_decoder_create(unsigned char *header1, int header1len, unsigned char *header2, int header2len, unsigned char *header3, int header3len);
void krad_vorbis_decoder_decode(krad_vorbis_t *vorbis, unsigned char *buffer, int bufferlen);
int krad_vorbis_decoder_read_audio(krad_vorbis_t *vorbis, int channel, char *buffer, int buffer_length);
