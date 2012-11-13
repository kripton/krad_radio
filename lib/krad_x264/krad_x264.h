#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>
#include <inttypes.h>

#include <x264.h>

#include "krad_system.h"
#include "krad_codec_header.h"

typedef struct krad_x264_encoder_St krad_x264_t;
typedef struct krad_x264_encoder_St krad_x264_encoder_t;


#ifndef KRAD_X264_H
#define KRAD_X264_H

struct krad_x264_encoder_St {

	int width;
	int height;
	int fps_numerator;
	int fps_denominator;
	
	int bitrate;
	
	int update_config;	
	
	int frames;

	int finish;

	x264_t *encoder;
	x264_param_t *params;
	x264_picture_t *picture;
	x264_nal_t *headers;
	
	unsigned char *header[10];
	int header_len[10];
	int header_count;

	unsigned char *header_combined;
	int header_combined_size;
	int header_combined_pos;
	int demented;
	
	krad_codec_header_t krad_codec_header;	
	
};

/* encoder */

int krad_x264_is_keyframe (unsigned char *buffer);

krad_x264_encoder_t *krad_x264_encoder_create (int width, int height,
											   int fps_numerator,
											   int fps_denominator,
											   int bitrate);

void krad_x264_encoder_destroy (krad_x264_encoder_t *krad_x264);
int krad_x264_encoder_write (krad_x264_encoder_t *krad_x264, unsigned char **packet, int *keyframe);

#endif
