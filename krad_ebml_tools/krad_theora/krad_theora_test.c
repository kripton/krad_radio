#include "krad_theora.h"

#define TEST_COUNT 500

int main (int argc, char *argv[]) {

//	krad_theora_encoder_t *krad_theora_encoder;
	krad_theora_decoder_t *krad_theora_decoder;

	int width;
	int height;
	int count;
	
	width = 640;
	height = 480;
	count = 0;

	while (count < TEST_COUNT) {

		krad_theora_decoder = krad_theora_decoder_create();
		krad_theora_decoder_destroy(krad_theora_decoder);

	//	krad_theora_encoder = krad_theora_encoder_create(width, height);
	//	krad_theora_encoder_destroy(krad_theora_encoder);
	
		count++;
	
	}
	
	printf("it worked\n");
	
	return 0;

}
