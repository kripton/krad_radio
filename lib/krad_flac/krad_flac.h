#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include <FLAC/stream_encoder.h>
#include <FLAC/stream_decoder.h>

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_codec_header.h"

#define KRAD_FLAC_MINIMAL_HEADER_SIZE 42
#define KRAD_FLAC_STREAMINFO_BLOCK_SIZE 38
#define KRAD_FLAC_MARKER "fLaC"
#define KRAD_DEFAULT_FLAC_FRAME_SIZE 4096

typedef struct krad_flac_St krad_flac_t;

struct krad_flac_St {

	FLAC__StreamEncoder *encoder;
	FLAC__StreamDecoder *decoder;
	
	int sample_rate;
	int bit_depth;
	int channels;

	int test_fd;

	int finished;

	unsigned char min_header[42];
	unsigned char streaminfo_block[38];
	
	unsigned char header[256];
	int header_size;
	
	unsigned char *comment_header;
	
	int have_min_header;
	int streaminfo_rewrite;
	krad_codec_header_t krad_codec_header;

	unsigned long long total_frames_input;
	unsigned long long total_frames;
	unsigned long long total_bytes;
	
	int bytes;
	unsigned char *encode_buffer;
	int int32_samples[8192 * 4];
	
	unsigned char *decode_buffer;
	int decode_buffer_len;
	int decode_buffer_pos;
	
	int frames;
	
	float **output_buffer;
};

/* Encoding */

FLAC__StreamEncoderWriteStatus krad_flac_encoder_write_callback ( 
	const FLAC__StreamEncoder *encoder, const FLAC__byte fbuffer[], 
	unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);

FLAC__StreamEncoderSeekStatus krad_flac_encoder_seek_callback (const FLAC__StreamEncoder *encoder, 
															   FLAC__uint64 absolute_byte_offset, void *client_data);
FLAC__StreamEncoderTellStatus krad_flac_encoder_tell_callback (const FLAC__StreamEncoder *encoder, 
															   FLAC__uint64 *absolute_byte_offset, void *client_data);

int krad_flac_encode (krad_flac_t *flac, float *audio, int frames, unsigned char *encode_buffer);
int krad_flac_encoder_frames_remaining (krad_flac_t *flac);
void krad_flac_encode_info (krad_flac_t *flac);
int krad_flac_encoder_read_header (krad_flac_t *flac, unsigned char *buffer);
int krad_flac_encoder_read_min_header (krad_flac_t *flac, unsigned char *buffer);
int krad_flac_encoder_finish (krad_flac_t *flac, unsigned char *encode_buffer);
void krad_flac_encoder_destroy (krad_flac_t *flac);
krad_flac_t *krad_flac_encoder_create (int channels, int sample_rate, int bit_depth);


/* Decoding */

void krad_flac_decoder_callback_error (const FLAC__StreamDecoder *flacdecoder, 
									   FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC__StreamDecoderWriteStatus krad_flac_decoder_callback_write (const FLAC__StreamDecoder *flacdecoder,
																				   const FLAC__Frame *frame, 
																				   const FLAC__int32 * const fbuffer[],
																				   void *client_data);

FLAC__StreamDecoderReadStatus krad_flac_decoder_callback_read (const FLAC__StreamDecoder *flacdecoder,
															   FLAC__byte fbuffer[], size_t *bytes, void *client_data);

int krad_flac_decode (krad_flac_t *flac, unsigned char *encoded_buffer, int len, float **audio);
void krad_flac_decoder_int24_to_float_array (const int *in, float *out, int len);
void krad_flac_decoder_int16_to_float_array (const int *in, float *out, int len);
void krad_flac_decoder_info (krad_flac_t *flac);
krad_flac_t *krad_flac_decoder_create ();
void krad_flac_decoder_destroy (krad_flac_t *flac);
																				   
