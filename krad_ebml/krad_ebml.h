#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>



#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <limits.h>

#ifndef KRADEBML_VERSION
#define KRADEBML_VERSION "2.1"
#endif

#define EBML_LENGTH_1 0x80 // 10000000
#define EBML_LENGTH_2 0x40 // 01000000
#define EBML_LENGTH_3 0x20 // 00100000
#define EBML_LENGTH_4 0x10 // 00010000
#define EBML_LENGTH_5 0x08 // 00001000
#define EBML_LENGTH_6 0x04 // 00000100
#define EBML_LENGTH_7 0x02 // 00000010
#define EBML_LENGTH_8 0x01 // 00000001

#define EBML_DATA_SIZE_UNKNOWN 0x01FFFFFFFFFFFFFFLLU
#define EBML_DATA_SIZE_UNKNOWN_LENGTH 8

#define EBML_ID_EBMLVERSION        0x4286
#define EBML_ID_EBMLREADVERSION    0x42F7
#define EBML_ID_EBMLMAXIDLENGTH    0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH  0x42F3
#define EBML_ID_DOCTYPE            0x4282
#define EBML_ID_DOCTYPEVERSION     0x4287
#define EBML_ID_DOCTYPEREADVERSION 0x4285

#define EBML_ID_HEADER					0x1A45DFA3
#define EBML_ID_CLUSTER					0x1F43B675
#define EBML_ID_TRACK_UID				0x73C5
#define EBML_ID_TRACK_TYPE				0x83
#define EBML_ID_LANGUAGE				0x22b59C
#define EBML_ID_SEGMENT					0x18538067
#define EBML_ID_SEGMENT_TITLE			0x7BA9
#define EBML_ID_SEGMENT_INFO			0x1549A966
#define EBML_ID_SEGMENT_TRACKS			0x1654AE6B
#define EBML_ID_TRACK					0xAE
#define EBML_ID_CODECDATA				0x63A2
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

#define EBML_ID_TIMECODESCALE			0x2AD7B1


#define EBML_ID_TAGS					0x1254C367
#define EBML_ID_TAG						0x7373
#define EBML_ID_TAG_TARGETS				0x63C0
#define EBML_ID_TAG_TARGETTYPEVALUE		0x68CA
#define EBML_ID_TAG_TARGETTYPE			0x63CA
#define EBML_ID_TAG_SIMPLE				0x67C8
#define EBML_ID_TAG_NAME				0x45A3
#define EBML_ID_TAG_STRING				0x4487
#define EBML_ID_TAG_BINARY				0x4485



#define KRADEBML_WRITE_BUFFER_SIZE 8192 * 1024 * 2

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

typedef struct kradx_base64_St kradx_base64_t;

struct kradx_base64_St {
	int len;
	int chunk;
	char *out;
	char *result;	
};

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
	//krad_EBML_IO_READWRITE,
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
	
	unsigned char *xiph_header[3];
	int xiph_header_len[3];
	
};

struct krad_ebml_io_St {

	int seekable;
	krad_ebml_io_mode_t mode;
	char *uri;
	int (* write)(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length);
	int (* read)(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length);
	int64_t (* seek)(krad_ebml_io_t *krad_ebml_io, int64_t offset, int whence);
	int64_t (* tell)(krad_ebml_io_t *krad_ebml_io);
	int32_t (* open)(krad_ebml_io_t *krad_ebml_io);
	int32_t (* close)(krad_ebml_io_t *krad_ebml_io);
	
	int ptr;
	char *host;
	char *mount;
	char *password;
	int port;
	int sd;
	

	unsigned char *write_buffer;
	uint64_t write_buffer_pos;

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
	
	char bsbuffer[8192 * 8];
	int stream;
	
	
	int64_t current_timecode;
	
	int width;
	int height;
	
	
	int read_laced_frames;
	uint64_t frame_sizes[256];
	int current_laced_frame;
	
	//writing
	uint64_t segment;
	uint64_t tracks_info;
	uint64_t cluster;	
	uint64_t video_frame_rate;
	uint64_t total_video_frames;
	uint64_t cluster_timecode;
	
	int total_audio_frames;
	float audio_sample_rate;
	int audio_channels;
	int audio_frames_since_cluster;
	
	int new_tags;
	char tags[512];
	int tags_position;
	
	int tracks_size;
	int tracks_pos;
	
};

int krad_ebml_read_element (krad_ebml_t *krad_ebml, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr);

void krad_ebml_sync (krad_ebml_t *krad_ebml);
int krad_ebml_add_video_track(krad_ebml_t *krad_ebml, char *codec_id, int frame_rate, int width, int height);
void krad_ebml_header (krad_ebml_t *krad_ebml, char *doctype, char *appversion);
int krad_ebml_add_video_track_with_private_data (krad_ebml_t *krad_ebml, char *codec_id, int frame_rate, int width, int height, unsigned char *private_data, int private_data_size);
int krad_ebml_add_subtitle_track(krad_ebml_t *krad_ebml, char *codec_id);
int krad_ebml_add_audio_track(krad_ebml_t *krad_ebml, char *codec_id, float sample_rate, int channels, unsigned char *private_data, int private_data_size);
void krad_ebml_add_video(krad_ebml_t *krad_ebml, int track_num, unsigned char *buffer, int buffer_len, int keyframe);
void krad_ebml_add_audio(krad_ebml_t *krad_ebml, int track_num, unsigned char *buffer, int buffer_len, int frames);
void krad_ebml_cluster(krad_ebml_t *krad_ebml, int64_t timecode);

void krad_ebml_start_segment(krad_ebml_t *krad_ebml, char *appversion);
void krad_ebml_write_element (krad_ebml_t *krad_ebml, uint32_t element);
void krad_ebml_write_uint32 (krad_ebml_t *krad_ebml, uint64_t element, uint32_t number);
void krad_ebml_write_string (krad_ebml_t *krad_ebml, uint64_t element, char *string);
void krad_ebml_write_data_size (krad_ebml_t *krad_ebml, uint64_t data_size);
void krad_ebml_write_reversed (krad_ebml_t *krad_ebml, void *buffer, uint32_t len);

krad_codec_t krad_ebml_get_track_codec (krad_ebml_t *krad_ebml, int tracknumber);
int krad_ebml_get_track_codec_data(krad_ebml_t *krad_ebml, int tracknumber, unsigned char *buffer);
int krad_ebml_get_track_count(krad_ebml_t *krad_ebml);
int krad_ebml_read_packet (krad_ebml_t *krad_ebml, int *tracknumber, unsigned char *buffer);
krad_ebml_t *krad_ebml_open_stream(char *host, int port, char *mount, char *password);
krad_ebml_t *krad_ebml_open_file(char *filename, krad_ebml_io_mode_t mode);
void krad_ebml_destroy(krad_ebml_t *krad_ebml);

void krad_ebml_write_tag (krad_ebml_t *krad_ebml, char *name, char *value);
int krad_ebml_write_sync(krad_ebml_t *krad_ebml);


void krad_ebml_start_element (krad_ebml_t *krad_ebml, uint32_t element, uint64_t *position);
void krad_ebml_finish_element (krad_ebml_t *krad_ebml, uint64_t element_position);

char *krad_ebml_version();

int64_t krad_ebml_tell(krad_ebml_t *krad_ebml);
int64_t krad_ebml_fileio_tell(krad_ebml_io_t *krad_ebml_io);
int krad_ebml_write(krad_ebml_t *krad_ebml, void *buffer, size_t length);
int krad_ebml_read(krad_ebml_t *krad_ebml, void *buffer, size_t length);
int krad_ebml_seek(krad_ebml_t *krad_ebml, int64_t offset, int whence);

