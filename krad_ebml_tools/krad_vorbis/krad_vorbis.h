#include <vorbis/vorbisenc.h>
#include <krad_audio.h>

#define MAX_CHANNELS 8

typedef struct krad_vorbis_St krad_vorbis_t;

struct krad_vorbis_St {

	jack_ringbuffer_t *ringbuf[MAX_CHANNELS];

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
	ogg_packet header_main;
	ogg_packet header_comments;
	ogg_packet header_codebooks;
	float samples[2048];
	unsigned char header[8192];
	int headerpos;
	int demented;
	
	int in_blockout;

};

ogg_packet *krad_vorbis_encode(krad_vorbis_t *vorbis, int frames, jack_ringbuffer_t *ring0, jack_ringbuffer_t *ring1);
void krad_vorbis_encoder_destroy(krad_vorbis_t *vorbis);
krad_vorbis_t *krad_vorbis_encoder_create(int channels, int sample_rate, float quality);


void krad_vorbis_decoder_destroy(krad_vorbis_t *vorbis);
krad_vorbis_t *krad_vorbis_decoder_create(unsigned char *header1, int header1len, unsigned char *header2, int header2len, unsigned char *header3, int header3len);
void krad_vorbis_decoder_decode(krad_vorbis_t *vorbis, unsigned char *buffer, int bufferlen);
int krad_vorbis_decoder_read_audio(krad_vorbis_t *vorbis, int channel, char *buffer, int buffer_length);
