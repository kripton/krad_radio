#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define SCHRO_ENABLE_UNSTABLE_API

#include <schroedinger/schro.h>
#include <schroedinger/schrodebug.h>
#include <schroedinger/schroparams.h>
#include <schroedinger/schrovideoformat.h>

#include "krad_system.h"


/*
#include <schroedinger/schroframe.h>
#include <schroedinger/schrodecoder.h>
#include <schroedinger/schroencoder.h>
*/


typedef struct krad_dirac_St krad_dirac_t;

struct krad_dirac_St {

	int width;
	int height;

	SchroEncoder *encoder;
	SchroDecoder *decoder;

	SchroVideoFormat *format;

	SchroBuffer *buffer;
	SchroFrame *frame;
	SchroFrame *frame2;
	
	int fps_numerator;
	int fps_denominator;
	int bitrate;
	
	
	unsigned char *fullpacket;
	
	int size;
	uint8_t *picture;
	int go;
	
	int verbose;

};


int krad_dirac_encoder_write (krad_dirac_t *dirac, unsigned char **packet);
void krad_dirac_encoder_frame_free (SchroFrame *frame, void *priv);
void krad_dirac_encoder_destroy (krad_dirac_t *dirac);
krad_dirac_t *krad_dirac_encoder_create (int width, int height, int fps_numerator, int fps_denominator, int bitrate);


void krad_dirac_packet_type (unsigned char p);
krad_dirac_t *krad_dirac_decoder_create ();
void krad_dirac_decoder_destroy (krad_dirac_t *dirac);
void krad_dirac_decoder_buffer_free (SchroBuffer *buf, void *priv);
void krad_dirac_decode (krad_dirac_t *dirac, unsigned char *buffer, int len);
