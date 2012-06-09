#include "krad_theora.h"

#define TEST_COUNT 1

int main (int argc, char *argv[]) {

	krad_theora_encoder_t *krad_theora_encoder;
	krad_theora_decoder_t *krad_theora_decoder;

	int width;
	int height;
	int quality;
	int count;
	int test_count;
	
	width = 1280;
	height = 720;
	count = 0;
	quality = 33;
	
	if (argc == 2) {
		test_count = atoi(argv[1]);
	} else {
		test_count = TEST_COUNT;
	}

	while (count < test_count) {

		//krad_theora_decoder_t *krad_theora_decoder_create(unsigned char *header1, int header1len, unsigned char *header2, int header2len, unsigned char *header3, int header3len);
		//krad_theora_decoder = krad_theora_decoder_create();
		//krad_theora_decoder_destroy(krad_theora_decoder);

		krad_theora_encoder = krad_theora_encoder_create(width, height, quality);
		krad_theora_encoder_destroy(krad_theora_encoder);
	
		count++;
	
	}
	
	printf("it worked %d times\n", test_count);
	
	return 0;

}
