#include <krad_dirac.h>
#include <fcntl.h>

#define TEST_COUNT1 450
#define TEST_COUNT2 1

void dirac_encode_test() {

	krad_dirac_t *krad_dirac;

	char filename[512];

	int count;
	unsigned char *buffer;
	unsigned char *framedata;
	int len;
	int took;
	int frame;
	
	int fd;


	strcpy(filename, "/home/oneman/Videos/");
	//strcat(filename, "rp-bvfi9-cs-lvl3-d13-noac-nocb-pcmp-cr422-10b.drc");
	strcat(filename, "test_dirac_encode1.drc");

	fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	buffer = calloc(1, 2000000);
	len = 0;
	took = 0;
	count = 0;
	krad_dirac = krad_dirac_encoder_create(1280, 720);
	
	while (count < TEST_COUNT1) {
	
	
		framedata = malloc(krad_dirac->size);
		memset (framedata, 64 + rand() % 64, krad_dirac->size);
	
		len = krad_dirac_encode (krad_dirac, framedata, buffer, &frame, &took);
		if (took == 1) {
			took = 0;
			count++;
		}
		
		if (len > 0) {
			krad_dirac_packet_type(buffer[4]);
			write(fd, buffer, len);
			printf("Encoded size is %d for frame %d\n", len, frame);
		}
		
	}
	
	printf("submitted %d frames\n", count);
	
	schro_encoder_end_of_stream (krad_dirac->encoder);
	
	while (frame != count - 1) {
		len = krad_dirac_encode (krad_dirac, NULL, buffer, &frame, &took);
		if (len > 0) {
			krad_dirac_packet_type(buffer[4]);
			write(fd, buffer, len);
			printf("Encoded size is %d for frame %d\n", len, frame);
		} else {
			usleep(50000);
		}
	}
	
	printf("finished!\n");
	
	krad_dirac_encoder_destroy(krad_dirac);

	close(fd);
	
	free(buffer);

}
/*
void dirac_decode_test() {
	
	krad_dirac_t *krad_dirac;

	char *filename = "/home/oneman/Downloads/bar.drc";

	int count;
	
	for (count = 0; count < TEST_COUNT2; count++) {
		krad_dirac = krad_dirac_decoder_create();
		krad_dirac_decode_test(krad_dirac, filename);
		krad_dirac_decoder_destroy(krad_dirac);
	}

}
*/
int main (int argc, char *argv[]) {

	int count;
	
	for (count = 0; count < TEST_COUNT2; count++) {
		dirac_encode_test();
		//dirac_decode_test();
	}

	return 0;

}
