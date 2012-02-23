#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

void rmemcpy(void *dst, void *src, int len) {

	unsigned char *a_dst;
	unsigned char *a_src;
	int count;
	
	count = 0;
	len -= 1;
	a_dst = dst;
	a_src = src;

	while (len > -1) {
		a_dst[len--] = a_src[count++];
	}
}

void inspect_flac_file (char *filename) {

	int fd;
	int ret;
	//char *filename;
	
	unsigned char bytes[4];
	unsigned char flac_header_magic_bytes[4];
	
	unsigned char channels_char;
	unsigned char sample_rate_char[3];
	unsigned char bit_depth_char;
	
	unsigned short int min_blocksize;
	unsigned short int max_blocksize;
	
	unsigned int channels;
	unsigned int sample_rate;
	unsigned int bit_depth;
	
	channels_char = 0;
	memset(sample_rate_char, 0, 3);
	bit_depth_char = 0;

	min_blocksize = 0;
	max_blocksize = 0;	
	channels = 0;
	sample_rate = 0;
	bit_depth = 0;
	
	memcpy (flac_header_magic_bytes, "fLaC", 4);

	fd = open ( filename, O_RDONLY );
	
	if (fd <= 0) {
		printf("Could not open file %s\n", filename);
		exit(1);
	}
	
	
	ret = read (fd, bytes, 4);
	
	if (ret != 4) {
		printf("Could not read file %s\n", filename);
		close (fd);
		exit(1);
	}
	
	if (memcmp(flac_header_magic_bytes, bytes, 4) == 0) {
	
	
		ret = lseek(fd, 8, SEEK_SET);
	
		if (ret != 8) {
			printf("Could not seek file %s\n", filename);
			close (fd);
			exit(1);
		}
	
		ret = read (fd, bytes, 2);

		if (ret != 2) {
			printf("Could not read file %s\n", filename);
			close (fd);
			exit(1);
		}
	
		rmemcpy(&min_blocksize, bytes, 2);	
	
		ret = read (fd, bytes, 2);

		if (ret != 2) {
			printf("Could not read file %s\n", filename);
			close (fd);
			exit(1);
		}
		
		rmemcpy(&max_blocksize, bytes, 2);	
	
		//printf("blocksize min: %u max: %u\n", min_blocksize, max_blocksize);
		
	
		ret = lseek(fd, 18, SEEK_SET);
	
		if (ret != 18) {
			printf("Could not seek file %s\n", filename);
			close (fd);
			exit(1);
		}
	
		ret = read (fd, bytes, 4);

		if (ret != 4) {
			printf("Could not read file %s\n", filename);
			close (fd);
			exit(1);
		}

		
		channels_char |= bytes[2] & 0x08;
		channels_char |= bytes[2] & 0x04;
		channels_char |= bytes[2] & 0x02;
		channels_char >>= 1;
		
		channels = channels_char;
		channels++;
		
		//printf("Channels bytes is %x\n", channels_char);
		//printf("Channels is %u\n", channels);
		
		bit_depth_char |= bytes[3] & 0x80;
		bit_depth_char |= bytes[3] & 0x40;
		bit_depth_char |= bytes[3] & 0x20;
		bit_depth_char |= bytes[3] & 0x10;
		bit_depth_char >>= 4;
		
		if (bytes[2] & 0x01) {
			bit_depth_char |= 0x10;
		}
		
		bit_depth = bit_depth_char;
		bit_depth++;
		
		//printf("bit_depth bytes is %x\n", bit_depth_char);
		//printf("bit depth is %u\n", bit_depth);
		
		sample_rate_char[0] |= bytes[2] & 0x80;
		sample_rate_char[0] |= bytes[2] & 0x40;
		sample_rate_char[0] |= bytes[2] & 0x20;
		sample_rate_char[0] |= bytes[2] & 0x10;

		sample_rate_char[2] = bytes[0];
		sample_rate_char[1] = bytes[1];
		
		memcpy((unsigned char *)&sample_rate, sample_rate_char, 3);
		
		sample_rate >>= 4;
		
		//printf("sample rate is %u\n", sample_rate);
		
		
		if ((min_blocksize == 0) || (max_blocksize == 0) || (min_blocksize != max_blocksize) || (sample_rate != 44100) || (bit_depth != 16)
			|| (channels != 2)) {
		
			printf("File: %s\t\tR: %u B: %u C: %u BS %u-%u\n", filename, sample_rate, bit_depth, channels, min_blocksize, max_blocksize);
		
		} 
	
	} else {
	
		if (memcmp("ID3", bytes, 3) == 0) {
			printf("File: %s ID3 Header\n", filename);
		} else {
			printf("File: %s No Flac Header\n", filename);
		}
		
		
		
	}
	
	ret = close (fd);

	if (ret != 0) {
		printf("Could not close file %s\n", filename);
		exit(1);
	}

}


int main (int argc, char *argv[]) {

	char *filename;
		
	filename = "/home/oneman/kode/test1/07 - Nine.flac";

	if (argc == 2) {
		filename = argv[1];
	}
	
	inspect_flac_file(filename);
	

	return 0;

}
