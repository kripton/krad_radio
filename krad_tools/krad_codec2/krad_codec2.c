#include "krad_codec2.h"



int krad_codec2_encode(krad_codec2_t *krad_codec2, float *audio, int frames, unsigned char *encode_buffer) {

}

void krad_codec2_encoder_destroy(krad_codec2_t *krad_codec2) {

    codec2_destroy(krad_codec2->codec2);
	free(krad_codec2)

}

krad_codec2_t *krad_codec2_encoder_create(int input_channels, int input_sample_rate) {

	krad_codec2_t *krad_codec2 = calloc(1, sizeof(krad_codec2_t));

	krad_codec2->input_channels = input_channels;
	krad_codec2->input_sample_rate = input_sample_rate;

	krad_codec2->codec2 = codec2_create();
	
	return krad_codec2;

}


krad_codec2_t *krad_codec2_decoder_create(int output_channels, int output_sample_rate) {

	krad_codec2_t *krad_codec2 = calloc(1, sizeof(krad_codec2_t));

	krad_codec2->output_channels = output_channels;
	krad_codec2->output_sample_rate = output_sample_rate;

	krad_codec2->codec2 = codec2_create();
	
	return krad_codec2;

}


void krad_codec2_decoder_destroy(krad_codec2_t *krad_codec2) {

    codec2_destroy(krad_codec2->codec2);
	free(krad_codec2)

}

int krad_codec2_decode(krad_codec2_t *krad_codec2, unsigned char *encoded_buffer, int len, float *audio) {


}



void krad_codec2_decoder_int24_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_int16_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_info(krad_codec2_t *krad_codec2);

