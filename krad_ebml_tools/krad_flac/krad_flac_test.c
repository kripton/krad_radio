#include "krad_flac.h"
#include "krad_tone.h"

#define TEST_COUNT 5
#define TEST_COUNT2 335
#define TEST_COUNT3 1

#define DECODING_TEST_COUNT1 1


void encoding_tests() {

	krad_flac_t *krad_flac;
	krad_tone_t *krad_tone;
	
	int count;

	int bit_depth;
	int sample_rate;
	int channels;
	
	int fd;
	unsigned char *buffer;
	int bytes;
	float *audio;
	
	count = 0;

	channels = 2;
	bit_depth = 24;
	sample_rate = 44100;

	// test create and destroy

	for (count = 0; count < TEST_COUNT; count++) {

		krad_flac = krad_flac_encoder_create(channels, sample_rate, bit_depth);
		printf("flac encoder created\n");

		krad_flac_encode_info(krad_flac);
		krad_flac_encode_test(krad_flac);

		krad_flac_encode_info(krad_flac);

		krad_flac_encoder_destroy(krad_flac);
		printf("flac encoder destroyed\n\n\n");
	
	}
	

	// Test Tone File

	krad_tone = krad_tone_create(sample_rate);
	krad_tone_add_preset(krad_tone, "dialtone");

	buffer = calloc(1, 8192 * 8);
	audio = calloc(1, 8192 * channels * 4);

	fd = open("/home/oneman/kode/testfile2.flac", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	krad_flac = krad_flac_encoder_create(1, sample_rate, bit_depth);
	bytes = krad_flac_encoder_read_min_header(krad_flac, buffer);
	write(fd, buffer, bytes);
	krad_flac_encode_info(krad_flac);
	
	for (count = 0; count < TEST_COUNT2; count++) {
		krad_tone_run(krad_tone, audio, 4096);
		bytes = krad_flac_encode(krad_flac, audio, 4096, buffer);
		write(fd, buffer, bytes);
		krad_flac_encode_info(krad_flac);
	}
	
	krad_flac_encoder_destroy(krad_flac);
	krad_tone_destroy(krad_tone);
	close(fd);
	free(audio);
	free(buffer);	
	
	
	// silent w/ seekback to update flac header

	buffer = calloc(1, 8192 * 8);
	audio = calloc(1, 8192 * channels * 4);

	fd = open("/home/oneman/kode/testfile3.flac", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	krad_flac = krad_flac_encoder_create(1, sample_rate, bit_depth);
	bytes = krad_flac_encoder_read_header(krad_flac, buffer);
	write(fd, buffer, bytes);
	krad_flac_encode_info(krad_flac);
	
	for (count = 0; count < TEST_COUNT2; count++) {
		bytes = krad_flac_encode(krad_flac, audio, 4096, buffer);
		write(fd, buffer, bytes);
		//krad_flac_encode_info(krad_flac);
	}
	
	
	bytes = krad_flac_encoder_finish(krad_flac, buffer);
	printf("got %d flac finiishing bytes\n", bytes);
	write(fd, buffer, bytes);
	
	
	bytes = krad_flac_encoder_read_header(krad_flac, buffer);
	lseek(fd, 0, SEEK_SET);
	write(fd, buffer, bytes);
	
	
	
	krad_flac_encoder_destroy(krad_flac);
	close(fd);
	free(audio);
	free(buffer);
	
	// Test Tone File w/ seekback to update flac header

	krad_tone = krad_tone_create(sample_rate);
	krad_tone_add_preset(krad_tone, "dialtone");

	buffer = calloc(1, 8192 * 8);
	audio = calloc(1, 8192 * channels * 4);

	fd = open("/home/oneman/kode/testfile4.flac", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	krad_flac = krad_flac_encoder_create(1, sample_rate, bit_depth);
	bytes = krad_flac_encoder_read_header(krad_flac, buffer);
	write(fd, buffer, bytes);
	krad_flac_encode_info(krad_flac);
	
	for (count = 0; count < TEST_COUNT2; count++) {
		krad_tone_run(krad_tone, audio, 4096);
		bytes = krad_flac_encode(krad_flac, audio, 4096, buffer);
		write(fd, buffer, bytes);
		//krad_flac_encode_info(krad_flac);
	}
	
	
	bytes = krad_flac_encoder_finish(krad_flac, buffer);
	printf("got %d flac finiishing bytes\n", bytes);
	write(fd, buffer, bytes);
	
	
	bytes = krad_flac_encoder_read_header(krad_flac, buffer);
	lseek(fd, 0, SEEK_SET);
	write(fd, buffer, bytes);
	
	
	
	krad_flac_encoder_destroy(krad_flac);
	krad_tone_destroy(krad_tone);
	close(fd);
	free(audio);
	free(buffer);
	
	
	
	printf("***************************\n");
	
	
	// silent2 w/ seekback to update flac header

	buffer = calloc(1, 8192 * 8);
	audio = calloc(1, 8192 * channels * 4);

	fd = open("/home/oneman/kode/testfile5.flac", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	krad_flac = krad_flac_encoder_create(2, sample_rate, bit_depth);
	bytes = krad_flac_encoder_read_min_header(krad_flac, buffer);
	printf("got %d header bytes\n", bytes);
	write(fd, buffer, bytes);
	krad_flac_encode_info(krad_flac);
	
	for (count = 0; count < 555; count++) {
		bytes = krad_flac_encode(krad_flac, audio, 4096, buffer);
		printf("got %d flac bytes\n", bytes);
		write(fd, buffer, bytes);
		krad_flac_encode_info(krad_flac);
	}
	
	
	bytes = krad_flac_encoder_finish(krad_flac, buffer);
	printf("got %d flac finiishing bytes\n", bytes);
	write(fd, buffer, bytes);
	
	
	
	bytes = krad_flac_encoder_read_min_header(krad_flac, buffer);
		printf("got %d headerre bytes\n", bytes);
	lseek(fd, 0, SEEK_SET);
	write(fd, buffer, bytes);
	
	
	
	krad_flac_encoder_destroy(krad_flac);
	close(fd);
	free(audio);
	free(buffer);
	

}


void decoding_tests() {

	krad_flac_t *krad_flac;

	int count;
	int fd;

	unsigned char *buffer;
	//int bytes;
	float *audio;

	for (count = 0; count < DECODING_TEST_COUNT1; count++) {
	
		krad_flac = krad_flac_decoder_create();

		buffer = calloc(1, 8192 * 8);
		audio = calloc(1, 8192 * 4 * 4);

		fd = open("/home/oneman/kode/testfile5.flac", O_RDONLY);
		
		read(fd, buffer, 42);
		krad_flac_decode(krad_flac, buffer, 42, audio);
		

		krad_flac_decoder_destroy(krad_flac);
	
		close(fd);
		free(audio);
		free(buffer);
	}


}


int main (int argc, char *argv[]) {


	encoding_tests();

	decoding_tests();

	return 0;

}
