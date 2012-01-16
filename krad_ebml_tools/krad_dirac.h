#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <schroedinger/schro.h>
#include <schroedinger/schrodebug.h>

#include <testsuite/common.h>

//#include <krad_audio.h>



typedef struct krad_dirac_St krad_dirac_t;

struct krad_dirac_St {

	int width;
	int height;

	SchroEncoder *encoder;
	SchroDecoder *decoder;

	SchroBuffer *buffer;
	SchroFrame *frame;
	SchroFrame *frame2;
	SchroVideoFormat *format;
	int size;
	uint8_t *picture;
	int n_frames;
	int go;
	
	int verbose;
	FILE *file;


};

void krad_dirac_encode_test (krad_dirac_t *dirac);
int krad_dirac_encode(krad_dirac_t *dirac, void *frame, void *output, int *framenum, int *took);
void krad_dirac_encoder_frame_free(SchroFrame *frame, void *priv);
void krad_dirac_encoder_destroy(krad_dirac_t *dirac);
krad_dirac_t *krad_dirac_encoder_create();


void krad_dirac_packet_type(unsigned char p);


krad_dirac_t *krad_dirac_decoder_create();
void krad_dirac_decoder_destroy(krad_dirac_t *dirac);
void krad_dirac_decoder_buffer_free (SchroBuffer *buf, void *priv);
void krad_dirac_decode_test(krad_dirac_t *dirac);

void krad_dirac_decode(krad_dirac_t *dirac, unsigned char *buffer, int len);

void krad_dirac_test_video_open (krad_dirac_t *dirac, char *filename);
