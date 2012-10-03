#include "krad_aac.h"

void krad_aac_encoder_destroy (krad_aac_t *krad_aac) {
	aacplusEncClose (krad_aac->encoder);
	if (krad_aac->header != NULL) {
		free (krad_aac->header);
	}
	free (krad_aac);
}

krad_aac_t *krad_aac_encoder_create (int channels, int sample_rate, int bitrate) {

	krad_aac_t *krad_aac = calloc(1, sizeof(krad_aac_t));

	krad_aac->channels = channels;
	krad_aac->sample_rate = sample_rate;
	krad_aac->bitrate = bitrate;

	krad_aac->encoder = aacplusEncOpen(krad_aac->sample_rate,
								   krad_aac->channels,
								   &krad_aac->input_samples,
								   &krad_aac->max_output_bytes);

	krad_aac->config = aacplusEncGetCurrentConfiguration (krad_aac->encoder);
  
	krad_aac->config->bitRate = krad_aac->bitrate;
	krad_aac->config->bandWidth = 0;
	krad_aac->config->outputFormat = 0;
	krad_aac->config->nChannelsOut = krad_aac->channels;
	krad_aac->config->inputFormat = AACPLUS_INPUT_FLOAT;
	
	if (aacplusEncSetConfiguration (krad_aac->encoder, krad_aac->config)) {
		printke("Krad aac config failed");
	}

	aacplusEncGetDecoderSpecificInfo (krad_aac->encoder, &krad_aac->header, &krad_aac->header_len);

	krad_aac->krad_codec_header.header[0] = krad_aac->header;
	krad_aac->krad_codec_header.header_size[0] = krad_aac->header_len;
	krad_aac->krad_codec_header.header_count = 1;

	krad_aac->krad_codec_header.header_combined = krad_aac->header;
	krad_aac->krad_codec_header.header_combined_size = krad_aac->header_len;

	printk("Krad aac header len %d", krad_aac->header_len);

	return krad_aac;
}


int krad_aac_encode (krad_aac_t *krad_aac, float *audio, int frames, unsigned char *encode_buffer) {
	return aacplusEncEncode(krad_aac->encoder, (int32_t *) audio, frames, encode_buffer, krad_aac->max_output_bytes);
}

