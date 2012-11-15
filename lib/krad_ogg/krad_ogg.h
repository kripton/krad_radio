#ifndef KRAD_OGG_H
#define KRAD_OGG_H

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

#include <ogg/ogg.h>
#include <theora/theora.h>
#include <vorbis/vorbisenc.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#include "krad_io.h"
#include "krad_system.h"
#include "krad_codec_header.h"
#include "krad_transmitter.h"

typedef struct krad_ogg_St krad_ogg_t;
typedef struct krad_ogg_track_St krad_ogg_track_t;


#define KRAD_OGG_DEFAULT_MAX_PACKETS_PER_PAGE 7
#define KRAD_OGG_MAX_TRACKS 10

#define KRAD_OGG_NO_SERIAL -420

struct krad_ogg_track_St {

	krad_codec_t codec;
	
	int channels;
	float sample_rate;
	int bit_depth;
	float frame_rate;
	int keyframe_shift;
	
	int width;
	int height;
	
	int fps_numerator;
	int fps_denominator;
	
	int header_count;
	unsigned char *header[32];
	int header_len[32];
	
	ogg_stream_state stream_state;
	int serial;
	int last_serial;
	
	int ready;
	
	uint64_t last_granulepos;
	
	uint64_t packet_num;
	
	
	int max_packets_per_page;
	int packets_on_current_page;
	
	
	ogg_int64_t frames;
	ogg_int64_t frames_since_keyframe;
	
	
  int writing;
	
};

struct krad_ogg_St {

	int track_count;
	krad_ogg_track_t *tracks;
	ogg_sync_state sync_state;
	krad_io_t *krad_io;
	krad_transmission_t *krad_transmission;
	unsigned char *input_buffer;

	int output_aux_headers;
};


int krad_ogg_track_count (krad_ogg_t *krad_ogg);
krad_codec_t krad_ogg_track_codec (krad_ogg_t *krad_ogg, int track);
int krad_ogg_track_header_count (krad_ogg_t *krad_ogg, int track);
int krad_ogg_track_header_size (krad_ogg_t *krad_ogg, int track, int header);
int krad_ogg_read_track_header (krad_ogg_t *krad_ogg, unsigned char *buffer, int track, int header);
int krad_ogg_track_active (krad_ogg_t *krad_ogg, int track);
int krad_ogg_track_changed (krad_ogg_t *krad_ogg, int track);
int krad_ogg_read_packet (krad_ogg_t *krad_ogg, int *track, uint64_t *timecode, unsigned char *buffer);
int krad_ogg_write (krad_ogg_t *krad_ogg, unsigned char *buffer, int length);
void krad_ogg_process (krad_ogg_t *krad_ogg);
krad_ogg_t *krad_ogg_open_file (char *filename, krad_io_mode_t mode);
krad_ogg_t *krad_ogg_open_stream (char *host, int port, char *mount, char *password);
krad_ogg_t *krad_ogg_open_transmission (krad_transmission_t *krad_transmission);
krad_ogg_t *krad_ogg_create();
void krad_ogg_destroy(krad_ogg_t *krad_ogg);
void krad_ogg_set_max_packets_per_page (krad_ogg_t *krad_ogg, int max_packets);

int krad_ogg_add_video_track_with_private_data (krad_ogg_t *krad_ogg, krad_codec_t codec, int fps_numerator,
												int fps_denominator, int width, int height, unsigned char *header[],
												int header_size[], int header_count);

int krad_ogg_add_video_track (krad_ogg_t *krad_ogg, krad_codec_t codec, int fps_numerator, 
												   int fps_denominator, int width, int height);

int krad_ogg_output_aux_headers (krad_ogg_t *krad_ogg);

int krad_ogg_add_audio_track (krad_ogg_t *krad_ogg, krad_codec_t codec, int sample_rate, int channels, 
							  unsigned char *header[], int header_size[], int header_count);	

int krad_ogg_add_track (krad_ogg_t *krad_ogg, krad_codec_t codec, 
						unsigned char *header[], int header_size[], int header_count);


void krad_ogg_add_video (krad_ogg_t *krad_ogg, int track, unsigned char *buffer, int buffer_size, int keyframe);
void krad_ogg_add_audio (krad_ogg_t *krad_ogg, int track, unsigned char *buffer, int buffer_size, int frames);


#endif
