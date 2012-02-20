#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define EBML_LENGTH_1 0x80 // 10000000
#define EBML_LENGTH_2 0x40 // 01000000
#define EBML_LENGTH_3 0x20 // 00100000
#define EBML_LENGTH_4 0x10 // 00010000
#define EBML_LENGTH_5 0x08 // 00001000
#define EBML_LENGTH_6 0x04 // 00000100
#define EBML_LENGTH_7 0x02 // 00000010
#define EBML_LENGTH_8 0x01 // 00000001

#define EBML_DATA_SIZE_UNKNOWN 0xFFFFFFFFFFFFFFFF

#define EBML_ID_EBMLVERSION        0x4286
#define EBML_ID_EBMLREADVERSION    0x42F7
#define EBML_ID_EBMLMAXIDLENGTH    0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH  0x42F3
#define EBML_ID_DOCTYPE            0x4282
#define EBML_ID_DOCTYPEVERSION     0x4287
#define EBML_ID_DOCTYPEREADVERSION 0x4285

#define EBML_ID_HEADER					0x1A45DFA3
#define EBML_ID_CLUSTER					0x1F43B675
#define EBML_ID_TRACK_UID				0x73c5
#define EBML_ID_TRACK_TYPE				0x83
#define EBML_ID_LANGUAGE				0x22b59c
#define EBML_ID_SEGMENT					0x18538067
#define EBML_ID_SEGMENT_TITLE			0x7BA9
#define EBML_ID_SEGMENT_INFO			0x1549A966
#define EBML_ID_SEGMENT_TRACKS			0x1654AE6B
#define EBML_ID_TAG						0x1254C367
#define EBML_ID_TRACK					0xAE
#define EBML_ID_CODECDATA				0x63a2
#define EBML_ID_CLUSTER_TIMECODE		0xE7
#define EBML_ID_SIMPLEBLOCK				0xA3
#define EBML_ID_BLOCKGROUP				0xA0
#define EBML_ID_DOCTYPE					0x4282
#define EBML_ID_MUXINGAPP 				0x4D80
#define EBML_ID_WRITINGAPP 				0x5741

#define EBML_ID_CODECID					0x86

#define EBML_ID_TRACKTYPE				0x83
#define EBML_ID_TRACKNUMBER				0xD7
#define EBML_ID_VIDEOWIDTH				0xB0
#define EBML_ID_VIDEOHEIGHT				0xBA

#define EBML_ID_VIDEOSETTINGS			0xE0
#define EBML_ID_AUDIOSETTINGS			0xE1

#define EBML_ID_AUDIOCHANNELS			0x9F
#define EBML_ID_AUDIOSAMPLERATE			0xB5
#define EBML_ID_AUDIOBITDEPTH			0x6264

#define EBML_ID_3D						0x53B8

#ifndef KRAD_CODEC_T
typedef enum {
	VORBIS = 6666,
	OPUS,
	FLAC,
	VP8,
	DIRAC,
	THEORA,
	NOCODEC,
} krad_codec_t;
#define KRAD_CODEC_T 1
#endif

typedef struct krad_ebml_St krad_ebml_t;
typedef struct krad_ebml_track_St krad_ebml_track_t;
typedef struct krad_ebml_io_St krad_ebml_io_t;
typedef struct krad_ebml_audiotrack_St krad_ebml_audiotrack_t;
typedef struct krad_ebml_videotrack_St krad_ebml_videotrack_t;
typedef struct krad_ebml_subtrack_St krad_ebml_subtrack_t;
typedef struct krad_ebml_cluster_St krad_ebml_cluster_t;

typedef enum {
	KRAD_EBML_IO_READONLY,
	KRAD_EBML_IO_WRITEONLY,
	//KRAD_EBML_IO_READWRITE,
} krad_ebml_io_mode_t;

struct krad_ebml_cluster_St {

	uint64_t position;
	uint64_t timecode;
	uint32_t size;

};

struct krad_ebml_track_St {

	krad_codec_t codec;
	
	int channels;
	int sample_rate;
	int bit_depth;
	unsigned char *codec_data;
	int codec_data_size;
};

struct krad_ebml_io_St {

	int seekable;
	krad_ebml_io_mode_t mode;
	char *uri;
	int (* write)(void *buffer, size_t length, krad_ebml_io_t *krad_ebml_io);
	int (* read)(void *buffer, size_t length, krad_ebml_io_t *krad_ebml_io);
	int64_t (* seek)(int64_t offset, int whence, krad_ebml_io_t *krad_ebml_io);
	int32_t (* open)(krad_ebml_io_t *krad_ebml_io);
	int32_t (* close)(krad_ebml_io_t *krad_ebml_io);
	
	int ptr;

};

struct krad_ebml_St {
	
	int record_cluster_info;
	krad_ebml_io_t io_adapter;

	uint64_t io_position;

	int ebml_level;
	uint32_t cluster_count;
	int cluster_recording_space;
	krad_ebml_cluster_t *clusters;
	int64_t current_cluster_timecode;
	short last_timecode;
	uint32_t block_count;
	uint32_t largest_cluster;
	uint32_t smallest_cluster;
	
	int track_count;
	int current_track;
	krad_ebml_track_t *tracks;
	
	int header_read;
	
};

krad_codec_t krad_ebml_get_track_codec (krad_ebml_t *krad_ebml, int tracknumber);
int krad_ebml_get_track_codec_data(krad_ebml_t *krad_ebml, int tracknumber, unsigned char *buffer);
int krad_ebml_get_track_count(krad_ebml_t *krad_ebml);
int krad_ebml_read_packet (krad_ebml_t *krad_ebml, int *tracknumber, unsigned char *buffer);

krad_ebml_t *krad_ebml_open_file(char *filename, krad_ebml_io_mode_t mode);
void krad_ebml_destroy(krad_ebml_t *krad_ebml);

int krad_ebml_write(krad_ebml_t *krad_ebml, void *buffer, size_t length);
int krad_ebml_read(krad_ebml_t *krad_ebml, void *buffer, size_t length);
int krad_ebml_seek(krad_ebml_t *krad_ebml, int64_t offset, int whence);

