#ifndef KRAD_CODEC_HEADER_H
#define KRAD_CODEC_HEADER_H

typedef struct krad_codec_header_St krad_codec_header_t;

struct krad_codec_header_St {

	unsigned char *header_combined;
	int header_combined_size;
	
	unsigned char *header[10];
	int header_size[10];
	int header_count;
};

#endif
