#include <krad_dirac.h>

#define TEST_COUNT1 1
#define TEST_COUNT2 1

void dirac_encode_test() {

	krad_dirac_t *krad_dirac;

	int count;
	
	for (count = 0; count < TEST_COUNT1; count++) {
		krad_dirac = krad_dirac_encoder_create(1280, 720);
		krad_dirac_encode (krad_dirac);
		krad_dirac_encoder_destroy(krad_dirac);
	}

}
	
void dirac_decode_test() {
	
	krad_dirac_t *krad_dirac;

	char *filename = "/home/oneman/Downloads/bar.drc";

	int count;
	
	for (count = 0; count < TEST_COUNT2; count++) {
		krad_dirac = krad_dirac_decoder_create();
		krad_dirac_decode(krad_dirac, filename);
		krad_dirac_decoder_destroy(krad_dirac);
	}

}

int main (int argc, char *argv[]) {

	int count;
	
	for (count = 0; count < TEST_COUNT2; count++) {
		//dirac_encode_test();
		dirac_decode_test();
	}

	return 0;

}
