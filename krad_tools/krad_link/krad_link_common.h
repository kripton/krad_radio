#include <string.h>

typedef struct krad_link_rep_St krad_link_rep_t;

#ifndef KRAD_LINK_COMMON_H
#define KRAD_LINK_COMMON_H

#ifndef KRAD_CODEC_T
typedef enum {
	VORBIS = 6666,
	OPUS,
	FLAC,
	VP8,
	DIRAC,
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
	FAILURE,
} krad_link_operation_mode_t;

typedef enum {
	TCP = 250,
	UDP,
	FILESYSTEM,
	FAIL,
} krad_link_transport_mode_t;

typedef enum {
	WINDOW = 300,
	COMMAND,
	DAEMON,
} krad_link_interface_mode_t;

typedef enum {
	TEST = 500,
	V4L2,
	DECKLINK,
	X11,
	NOVIDEO,
} krad_link_video_source_t;

char *krad_link_transport_mode_to_string (krad_link_transport_mode_t transport_mode);
char *krad_link_video_source_to_string (krad_link_video_source_t video_source);
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

	char host[512];
	int tcp_port;
	char mount[512];
	char password[512];
	int width;
	int height;
	float vorbis_quality;
	int audio_channels;

};


#endif
