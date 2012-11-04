#ifndef KRAD_EBML_H
#define KRAD_EBML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
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

#include "krad_transmitter.h"
#include "krad_system.h"
#include "krad_codec_header.h"

#ifndef KRADEBML_VERSION
#define KRADEBML_VERSION "10"
#endif

#define VOID_START_SIZE 16382
#define VOID_START_SIZE_SIZE 3

#define CLUSTER_RECORDING_START_SIZE 32000

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

#define EBML_ID_VOID			   0xEC

#define EBML_ID_EBMLVERSION        0x4286
#define EBML_ID_EBMLREADVERSION    0x42F7
#define EBML_ID_EBMLMAXIDLENGTH    0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH  0x42F3
#define EBML_ID_DOCTYPE            0x4282
#define EBML_ID_DOCTYPEVERSION     0x4287
#define EBML_ID_DOCTYPEREADVERSION 0x4285
#define EBML_ID_HEADER			   0x1A45DFA3

#define EBML_ID_SEEKHEAD		   0x114D9B74
#define EBML_ID_SEEK 			   0x4DBB
#define EBML_ID_SEEK_ID 		   0x53AB
#define EBML_ID_SEEK_POSITION	   0x53AC



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
#define EBML_ID_AUDIOOUTPUTSAMPLERATE	0x78B5
#define EBML_ID_AUDIOBITDEPTH			0x6264

#define EBML_ID_3D						0x53B8

#define EBML_ID_TIMECODESCALE			0x2AD7B1
#define EBML_ID_DURATION				0x4489
#define EBML_ID_DEFAULTDURATION			0x23E383

#define EBML_ID_TAGS					0x1254C367
#define EBML_ID_TAG						0x7373
#define EBML_ID_TAG_TARGETS				0x63C0
#define EBML_ID_TAG_TARGETTYPEVALUE		0x68CA
#define EBML_ID_TAG_TARGETTYPE			0x63CA
#define EBML_ID_TAG_SIMPLE				0x67C8
#define EBML_ID_TAG_NAME				0x45A3
#define EBML_ID_TAG_STRING				0x4487
#define EBML_ID_TAG_BINARY				0x4485

#define EBML_ID_CUES					0x1C53BB6B
#define EBML_ID_CUEPOINT 				0xBB
#define EBML_ID_CUETIME					0xB3
#define EBML_ID_CUETRACKPOSITIONS		0xB7
#define EBML_ID_CUETRACK				0xF7
#define EBML_ID_CUECLUSTERPOSITION		0xF1

#define KRAD_EBML_MAX_TRACKS 10

#define KRADEBML_WRITE_BUFFER_SIZE 8192 * 1024 * 2

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

/* from nestegg */

enum ebml_type_enum {
  TYPE_UNKNOWN,
  TYPE_MASTER,
  TYPE_UINT,
  TYPE_FLOAT,
  TYPE_INT,
  TYPE_STRING,
  TYPE_BINARY
};

struct ebml_binary {
  unsigned char * data;
  size_t length;
};

struct ebml_type {
  union ebml_value {
    uint64_t u;
    double f;
    int64_t i;
    char * s;
    struct ebml_binary b;
  } v;
  enum ebml_type_enum type;
};

struct ebml_header {
  struct ebml_type ebml_version;
  struct ebml_type ebml_read_version;
  struct ebml_type ebml_max_id_length;
  struct ebml_type ebml_max_size_length;
  struct ebml_type doctype;
  struct ebml_type doctype_version;
  struct ebml_type doctype_read_version;
};

/* end from nestegg */


typedef enum {
	KRAD_EBML_IO_READONLY,
	KRAD_EBML_IO_WRITEONLY,
	KRAD_EBML_IO_READWRITE,
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
	
	int width;
	int height;
	
	
	unsigned char *codec_data;
	int codec_data_size;
	
	unsigned char *header[3];
	int header_len[3];
	int headers;
	int changed;
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
	
	int firstwritedone;
	krad_transmission_t *krad_transmission;	
	
	unsigned char *buffer_io_buffer;
	int buffer_io_read_pos;
	int buffer_io_read_len;	

	unsigned char *write_buffer;
	uint64_t write_buffer_pos;

};

struct krad_ebml_St {
	
	struct ebml_header *header;
	
	int record_cluster_info;
	krad_ebml_io_t io_adapter;

	uint64_t io_position;

	int ebml_level;
	uint32_t cluster_count;
	int cluster_recording_space;
	int current_cluster;
	krad_ebml_cluster_t *clusters;
	uint64_t current_cluster_timecode;
	short last_block_timecode;
	uint32_t block_count;
	uint32_t largest_cluster;
	uint32_t smallest_cluster;
	
	int track_count;
	int current_track;
	krad_ebml_track_t *tracks;
	
	int header_read;
	
	char bsbuffer[8192 * 8];
	int stream;
	
	
	krad_transmission_t *krad_transmission;	
	
	uint64_t current_timecode;
	
	int width;
	int height;
	
	
	int read_laced_frames;
	uint64_t frame_sizes[256];
	int current_laced_frame;
	
	//writing
	uint64_t segment;
	uint64_t segment_size;
	uint64_t void_space;
	uint64_t segment_info_position;
	uint64_t tracks_info_position;
	uint64_t cluster_start_position;
	uint64_t cues_position;	
	float segment_duration;
	uint64_t segment_timecode;
	uint64_t tracks_info;
	uint64_t cluster;	
	int fps_numerator;
	int fps_denominator;
	uint64_t total_video_frames;
	uint64_t cluster_timecode;
	
	uint64_t total_audio_frames;
	double audio_sample_rate;
	int audio_channels;
	int audio_frames_since_cluster;
	
	int new_tags;
	char tags[512];
	int tags_position;
	
	int tracks_size;
	int tracks_pos;
	
	//fwd kludge	
	int read_copy;
	unsigned char *read_copy_buffer;
	uint64_t read_copy_pos;
	
};

/* writing functions */

char *krad_ebml_codec_to_ebml_codec_id (krad_codec_t codec);

void krad_ebml_sync (krad_ebml_t *krad_ebml);
int krad_ebml_add_video_track(krad_ebml_t *krad_ebml, krad_codec_t codec, int fps_numerator, int fps_denominator, int width, int height);
void krad_ebml_header (krad_ebml_t *krad_ebml, char *doctype, char *appversion);
void krad_ebml_header_advanced (krad_ebml_t *krad_ebml, char *doctype, int doctype_version, int doctype_read_version);
int krad_ebml_add_video_track_with_private_data (krad_ebml_t *krad_ebml, krad_codec_t codec, int fps_numerator, int fps_denominator, int width, int height, unsigned char *private_data, int private_data_size);
int krad_ebml_add_subtitle_track(krad_ebml_t *krad_ebml, char *codec_id);
int krad_ebml_add_audio_track(krad_ebml_t *krad_ebml, krad_codec_t codec, int sample_rate, int channels, unsigned char *private_data, int private_data_size);
void krad_ebml_add_video(krad_ebml_t *krad_ebml, int track_num, unsigned char *buffer, int buffer_len, int keyframe);
void krad_ebml_add_audio(krad_ebml_t *krad_ebml, int track_num, unsigned char *buffer, int buffer_len, int frames);
void krad_ebml_cluster(krad_ebml_t *krad_ebml, int64_t timecode);

void krad_ebml_start_segment(krad_ebml_t *krad_ebml, char *appversion);
void krad_ebml_start_file_segment(krad_ebml_t *krad_ebml);
void krad_ebml_finish_file_segment(krad_ebml_t *krad_ebml);
void krad_ebml_write_element (krad_ebml_t *krad_ebml, uint32_t element);
void krad_ebml_write_int8 (krad_ebml_t *krad_ebml, uint64_t element, int8_t number);
void krad_ebml_write_int32 (krad_ebml_t *krad_ebml, uint64_t element, int32_t number);
void krad_ebml_write_int64 (krad_ebml_t *krad_ebml, uint64_t element, int64_t number);
void krad_ebml_write_string (krad_ebml_t *krad_ebml, uint64_t element, char *string);
void krad_ebml_write_data_size (krad_ebml_t *krad_ebml, uint64_t data_size);
void krad_ebml_write_reversed (krad_ebml_t *krad_ebml, void *buffer, uint32_t len);
void krad_ebml_write_float (krad_ebml_t *krad_ebml, uint64_t element, float number);

void krad_ebml_write_tag (krad_ebml_t *krad_ebml, char *name, char *value);
int krad_ebml_write_sync(krad_ebml_t *krad_ebml);
void krad_ebml_start_element (krad_ebml_t *krad_ebml, uint32_t element, uint64_t *position);
void krad_ebml_finish_element (krad_ebml_t *krad_ebml, uint64_t element_position);


/* reading functions */

int krad_ebml_check_doctype_header (struct ebml_header *ebml_head, char *doctype, int doctype_version, int doctype_read_version );
int krad_ebml_read_ebml_header (krad_ebml_t *krad_ebml, struct ebml_header *ebml_head);
int krad_ebml_check_ebml_header (struct ebml_header *ebml_head);
void krad_ebml_print_ebml_header (struct ebml_header *ebml_head);

int krad_ebml_track_count(krad_ebml_t *krad_ebml);
krad_codec_t krad_ebml_track_codec (krad_ebml_t *krad_ebml, int track);

int krad_ebml_track_header_count (krad_ebml_t *krad_ebml, int track);
int krad_ebml_track_header_size (krad_ebml_t *krad_ebml, int track, int header);
int krad_ebml_read_track_header (krad_ebml_t *krad_ebml, unsigned char *buffer, int track, int header);
int krad_ebml_track_active (krad_ebml_t *krad_ebml, int track);
int krad_ebml_track_changed (krad_ebml_t *krad_ebml, int track);

int krad_ebml_read_packet (krad_ebml_t *krad_ebml, int *track, uint64_t *timecode, unsigned char *buffer);


int krad_ebml_read_element_from_frag (unsigned char *ebml_frag, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr);

/* internal read func */
int krad_ebml_read_element (krad_ebml_t *krad_ebml, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr);


// fwd copy kludge
void krad_ebml_disable_read_copy ( krad_ebml_t *krad_ebml );
int krad_ebml_read_copy (krad_ebml_t *krad_ebml, void *buffer );
void krad_ebml_enable_read_copy ( krad_ebml_t *krad_ebml );
float krad_ebml_read_float (krad_ebml_t *krad_ebml, uint64_t ebml_data_size);
uint64_t krad_ebml_read_string (krad_ebml_t *krad_ebml, char *string, uint64_t ebml_data_size);
uint64_t krad_ebml_read_number_from_frag (unsigned char *ebml_frag, uint64_t ebml_data_size);
uint64_t krad_ebml_read_number (krad_ebml_t *krad_ebml, uint64_t ebml_data_size);
uint64_t krad_ebml_read_command (krad_ebml_t *krad_ebml, unsigned char *buffer);
krad_ebml_t *krad_ebml_open_buffer(krad_ebml_io_mode_t mode);
int krad_ebml_io_buffer_push (krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length);
int krad_ebml_io_buffer_read_space (krad_ebml_io_t *krad_ebml_io);
krad_ebml_t *krad_ebml_open_active_socket (int socket, krad_ebml_io_mode_t mode);
/* r/w functions */

krad_ebml_t *krad_ebml_open_stream(char *host, int port, char *mount, char *password);
krad_ebml_t *krad_ebml_open_file(char *filename, krad_ebml_io_mode_t mode);
krad_ebml_t *krad_ebml_open_transmission (krad_transmission_t *krad_transmission);

void krad_ebml_destroy(krad_ebml_t *krad_ebml);
char *krad_ebml_version();

int64_t krad_ebml_tell(krad_ebml_t *krad_ebml);
int krad_ebml_write(krad_ebml_t *krad_ebml, void *buffer, size_t length);
int krad_ebml_read(krad_ebml_t *krad_ebml, void *buffer, size_t length);
int krad_ebml_seek(krad_ebml_t *krad_ebml, int64_t offset, int whence);

int krad_ebml_fileio_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length);
int64_t krad_ebml_fileio_seek(krad_ebml_io_t *krad_ebml_io, int64_t offset, int whence);
int64_t krad_ebml_fileio_tell(krad_ebml_io_t *krad_ebml_io);
#endif
