#ifndef KRAD_CODEC_HEADER_H
#define KRAD_CODEC_HEADER_H

#ifndef KRAD_CODEC_T
typedef enum {
	VORBIS = 6666,
	OPUS,
	FLAC,
	VP8,
	THEORA,
	MJPEG,
	PNG,
	CODEC2,
	SKELETON,
	Y4M,
	DAALA,
	H264,
	NOCODEC,
} krad_codec_t;
#define KRAD_CODEC_T 1
#endif

typedef struct krad_codec_header_St krad_codec_header_t;

struct krad_codec_header_St {

	unsigned char *header_combined;
	int header_combined_size;
	
	unsigned char *header[10];
	int header_size[10];
	int header_count;
};

#endif
