#include <string.h>

#include <opus_defines.h>

typedef struct krad_link_rep_St krad_link_rep_t;

#ifndef KRAD_LINK_COMMON_H
#define KRAD_LINK_COMMON_H

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
	HEXON,
	DAALA,
	NOCODEC,
} krad_codec_t;
#define KRAD_CODEC_T 1
#endif

typedef enum {
	AUDIO_ONLY = 150,
	VIDEO_ONLY,
	AUDIO_AND_VIDEO,
} krad_link_av_mode_t;

typedef enum {
	CAPTURE = 200,
	RECEIVE,
	TRANSMIT,
	PLAYBACK,
	RECORD,
	DISPLAY,
	FAILURE,
} krad_link_operation_mode_t;

typedef enum {
	TCP = 250,
	UDP,
	FILESYSTEM,
	FAIL,
} krad_link_transport_mode_t;

typedef enum {
	TEST = 500,
	INFO,
	V4L2,
	DECKLINK,
	X11,
	NOVIDEO,
} krad_link_video_source_t;


int krad_opus_string_to_bandwidth (char *string);
int krad_opus_string_to_signal (char *string);
char *krad_opus_bandwidth_to_string (int bandwidth);
char *krad_opus_signal_to_string (int signal);

char *krad_link_transport_mode_to_string (krad_link_transport_mode_t transport_mode);
char *krad_link_video_source_to_string (krad_link_video_source_t video_source);
krad_link_transport_mode_t krad_link_string_to_transport_mode (char *string);
krad_link_video_source_t krad_link_string_to_video_source (char *string);
char *krad_link_operation_mode_to_string (krad_link_operation_mode_t operation_mode);
krad_link_operation_mode_t krad_link_string_to_operation_mode (char *string);
char *krad_link_av_mode_to_string (krad_link_av_mode_t av_mode);
krad_link_av_mode_t krad_link_string_to_av_mode (char *string);
char *krad_codec_to_string (krad_codec_t codec);
krad_codec_t krad_string_to_codec (char *string);

struct krad_link_rep_St {

//	krad_tags_t *krad_tags;
	krad_link_transport_mode_t transport_mode;
	krad_link_operation_mode_t operation_mode;
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	krad_link_av_mode_t av_mode;
	krad_link_video_source_t video_source;

	char filename[512];
	char host[512];
	int port;
	char mount[512];
	char password[512];
	int width;
	int height;
	float vorbis_quality;
	int audio_channels;

	int opus_bandwidth;
	int opus_signal;
	int opus_bitrate;
	int opus_complexity;
	int opus_frame_size;

};


#endif
