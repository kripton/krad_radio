#include "krad_vpx.h"

#define TEST_COUNT 100

int main (int argc, char *argv[]) {

	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_vpx_decoder_t *krad_vpx_decoder;

	int width;
	int height;
	int count;
	int bitrate;
	
	width = 640;
	height = 480;
	count = 0;
	bitrate = 1000;

	while (count < TEST_COUNT) {

		krad_vpx_decoder = krad_vpx_decoder_create();
		krad_vpx_decoder_destroy(krad_vpx_decoder);

		krad_vpx_encoder = krad_vpx_encoder_create(width, height, bitrate);
		krad_vpx_encoder_destroy(krad_vpx_encoder);
	
		count++;
	
	}
	
	printf("it worked\n");
	
	return 0;

}
