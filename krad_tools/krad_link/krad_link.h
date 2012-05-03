typedef struct krad_link_St krad_link_t;
typedef struct krad_linker_St krad_linker_t;

#include "krad_radio.h"


#ifndef KRAD_LINK_H
#define KRAD_LINK_H

#include <sys/prctl.h>
#include <libswscale/swscale.h>

#define APPVERSION "Krad Link 2.42"
#define KRAD_LINKER_MAX_LINKS 42
#define DEFAULT_VPX_BITRATE 200 * 8
#define HELP -1337
#define DEFAULT_CAPTURE_BUFFER_FRAMES 15
#define DEFAULT_ENCODING_BUFFER_FRAMES 15
#define DEFAULT_VORBIS_QUALITY 0.7
#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720
#define DEFAULT_FPS 15
#define KRAD_LINK_DEFAULT_TCP_PORT 80
#define KRAD_LINK_DEFAULT_UDP_PORT 42666

struct krad_linker_St {
	krad_link_t *krad_link[KRAD_LINKER_MAX_LINKS];
	krad_radio_t *krad_radio;
};

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
} krad_video_source_t;

struct krad_link_St {


	krad_radio_t *krad_radio;

	char sysname[64];
	krad_tags_t *krad_tags;

	krad_link_av_mode_t av_mode;

	krad_decklink_t *krad_decklink;
	krad_x11_t *krad_x11;
	kradgui_t *krad_gui;
	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_vpx_decoder_t *krad_vpx_decoder;
	krad_theora_encoder_t *krad_theora_encoder;
	krad_theora_decoder_t *krad_theora_decoder;
	krad_dirac_t *krad_dirac;	
	krad_vorbis_t *krad_vorbis;
	krad_flac_t *krad_flac;
	krad_opus_t *krad_opus;
	krad_ogg_t *krad_ogg;
	krad_ebml_t *krad_ebml;
	krad_container_t *krad_container;
	krad_v4l2_t *krad_v4l2;
	
	krad_link_operation_mode_t operation_mode;
	krad_link_interface_mode_t interface_mode;
	krad_video_source_t video_source;
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
	krad_codec_t last_audio_codec;
	krad_codec_t last_video_codec;
	
	krad_link_transport_mode_t transport_mode;
	
	char device[512];
	char alsa_capture_device[512];
	char alsa_playback_device[512];
	char jack_ports[512];
	
	char output[512];
	char input[512];
	char host[512];
	int tcp_port;
	char mount[512];
	char password[512];

	
	int capture_width;
	int capture_height;
	int capture_fps;
	int composite_width;
	int composite_height;
	int composite_fps;
	int display_width;
	int display_height;
	int encoding_width;
	int encoding_height;
	int encoding_fps;

	int mjpeg_mode;

	int capture_buffer_frames;
	int encoding_buffer_frames;
	int decoding_buffer_frames;
	
	unsigned char *current_encoding_frame;
	unsigned char *current_frame;
	
	unsigned int captured_frames;
	
	int encoding;
	int capturing;
	
	float vorbis_quality;
	int capture_audio;
	krad_ringbuffer_t *audio_input_ringbuffer[KRAD_MIXER_MAX_CHANNELS];
	krad_ringbuffer_t *audio_output_ringbuffer[KRAD_MIXER_MAX_CHANNELS];
	float *samples[KRAD_MIXER_MAX_CHANNELS];
	int audio_encoder_ready;
	int audio_frames_captured;
	int audio_frames_encoded;
		
	int video_frames_encoded;	
	int composited_frame_byte_size;

	krad_ringbuffer_t *decoded_audio_ringbuffer;
	krad_ringbuffer_t *encoded_audio_ringbuffer;
	krad_ringbuffer_t *encoded_video_ringbuffer;

	krad_ringbuffer_t *captured_frames_buffer;
	krad_ringbuffer_t *composited_frames_buffer;
	krad_ringbuffer_t *decoded_frames_buffer;
	
	struct SwsContext *decoded_frame_converter;
	struct SwsContext *captured_frame_converter;
	struct SwsContext *encoding_frame_converter;
	struct SwsContext *display_frame_converter;
	
	int video_track;
	int audio_track;

	pthread_t main_thread;

	pthread_t video_capture_thread;
	pthread_t video_encoding_thread;
	pthread_t audio_encoding_thread;

	pthread_t video_decoding_thread;
	pthread_t audio_decoding_thread;

	pthread_t stream_output_thread;
	pthread_t stream_input_thread;

	pthread_t udp_output_thread;
	pthread_t udp_input_thread;

	pthread_t signal_thread;
	sigset_t set;
	int destroy;

	int audio_channels;

	int render_meters;

	int new_capture_frame;
	int link_started;

	float temp_peak;
	float kick;

	// for playback
	uint64_t current_frame_timecode;

	// for internal signal generation
	struct timespec next_frame_time;
	struct timespec sleep_time;
	uint64_t next_frame_time_ms;

	int input_ready;
	int verbose;
	
    vpx_codec_enc_cfg_t vpx_encoder_config;

	char bug[512];
	int bug_x;
	int bug_y;

	struct stat file_stat;
		
	int udp_mode;
	
	int udp_recv_port;
	int udp_send_port;
	
	krad_rebuilder_t *krad_rebuilder;
	krad_slicer_t *krad_slicer;
	
	krad_codec2_t *krad_codec2_decoder;
	krad_codec2_t *krad_codec2_encoder;
};

krad_linker_t *krad_linker_create ();
void krad_linker_destroy (krad_linker_t *krad_linker);
int krad_linker_handler ( krad_linker_t *krad_linker, krad_ipc_server_t *krad_ipc );


void krad_link_shutdown();


void krad_link_audio_samples_callback (int frames, void *userdata, float **samples);
void dbg (char* format, ...);
void krad_link_destroy (krad_link_t *krad_link);
krad_link_t *krad_link_create();
void krad_link_run (krad_link_t *krad_link);
#endif
