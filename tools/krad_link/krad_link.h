typedef struct krad_link_St krad_link_t;
typedef struct krad_linker_St krad_linker_t;
typedef struct krad_linker_listen_client_St krad_linker_listen_client_t;

#include "krad_radio.h"


#ifndef KRAD_LINK_H
#define KRAD_LINK_H

#include <sys/prctl.h>
#include <libswscale/swscale.h>

#define DEFAULT_VPX_BITRATE 180 * 8
#define DEFAULT_DIRAC_BITRATE 15000000
#define DEFAULT_THEORA_QUALITY 42
#define DEFAULT_CAPTURE_BUFFER_FRAMES 120
#define DEFAULT_DECODING_BUFFER_FRAMES 120
#define DEFAULT_VORBIS_QUALITY 0.6
#define DEFAULT_CAPTURE_WIDTH 1280
#define DEFAULT_CAPTURE_HEIGHT 720
#define DEFAULT_ENCODER_WIDTH 1280
#define DEFAULT_ENCODER_HEIGHT 720
#define DEFAULT_COMPOSITOR_WIDTH 1280
#define DEFAULT_COMPOSITOR_HEIGHT 720
#define DEFAULT_COMPOSITOR_FPS 30
#define DEFAULT_COMPOSITOR_FPS_NUMERATOR DEFAULT_COMPOSITOR_FPS * 1000
#define DEFAULT_COMPOSITOR_FPS_DENOMINATOR 1 * 1000
#define DEFAULT_FPS 30
#define DEFAULT_FPS_NUMERATOR DEFAULT_FPS * 1000
#define DEFAULT_FPS_DENOMINATOR 1 * 1000
#define KRAD_LINK_DEFAULT_TCP_PORT 80
#define KRAD_LINK_DEFAULT_UDP_PORT 42666
#define KRAD_LINK_DEFAULT_VIDEO_CODEC VP8
#define KRAD_LINK_DEFAULT_AUDIO_CODEC VORBIS
#define KRAD_LINKER_MAX_LINKS 42
#define HELP -1337

struct krad_linker_St {
	krad_link_t *krad_link[KRAD_LINKER_MAX_LINKS];
	krad_radio_t *krad_radio;

	pthread_mutex_t change_lock;

	/* linker listener */	
	unsigned char *buffer;

	int port;
	int sd;
	struct sockaddr_in local_address;
	int listening;
	int stop_listening;
	pthread_t listening_thread;
	
	/* linker transmitter */	
	krad_transmitter_t *krad_transmitter;	
	
};

struct krad_linker_listen_client_St {

	krad_linker_t *krad_linker;
	pthread_t client_thread;

	char in_buffer[1024];
	char out_buffer[1024];
	char mount[256];
	char content_type[256];
	int got_mount;
	int got_content_type;

	int in_buffer_pos;
	int out_buffer_pos;
	
	int sd;
	int ret;
	int wrote;

};

struct krad_link_St {


	krad_radio_t *krad_radio;

	krad_linker_t *krad_linker;

	char sysname[64];
	krad_tags_t *krad_tags;

	krad_link_av_mode_t av_mode;

	krad_framepool_t *krad_framepool;

	krad_mixer_portgroup_t *krad_mixer_portgroup;
	krad_compositor_port_t *krad_compositor_port;

	krad_decklink_t *krad_decklink;
	krad_x11_t *krad_x11;
	kradgui_t *krad_gui;
	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_vpx_decoder_t *krad_vpx_decoder;
	krad_theora_encoder_t *krad_theora_encoder;
	krad_theora_decoder_t *krad_theora_decoder;
//	krad_codec2_t *krad_codec2_decoder;
//	krad_codec2_t *krad_codec2_encoder;	
	krad_dirac_t *krad_dirac;	
	krad_vorbis_t *krad_vorbis;
	krad_flac_t *krad_flac;
	krad_opus_t *krad_opus;
	krad_ogg_t *krad_ogg;
	krad_ebml_t *krad_ebml;
	krad_container_t *krad_container;
	krad_v4l2_t *krad_v4l2;
	
	krad_link_operation_mode_t operation_mode;
	krad_link_video_source_t video_source;
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
	int port;
	char mount[512];
	char content_type[512];	
	char password[512];
	
	int sd;

	int vp8_bitrate;
	
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
	int encoding_fps_numerator;
	int encoding_fps_denominator;


	int mjpeg_mode;
	
	int mjpeg_passthru;	

	int capture_buffer_frames;
	int decoding_buffer_frames;
	
	unsigned char *current_encoding_frame;
	unsigned char *current_frame;
	
	unsigned int captured_frames;
	
	int encoding;
	int capturing;
	
	float vorbis_quality;
	int capture_audio;
	krad_ringbuffer_t *audio_capture_ringbuffer[KRAD_MIXER_MAX_CHANNELS];	
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
	
	struct SwsContext *decoded_frame_converter;
	struct SwsContext *captured_frame_converter;
	struct SwsContext *encoder_frame_converter;
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

	char bug[512];
	int bug_x;
	int bug_y;

	struct stat file_stat;
	
	krad_rebuilder_t *krad_rebuilder;
	krad_slicer_t *krad_slicer;

};



void krad_linker_stop_listening (krad_linker_t *krad_linker);
int krad_linker_listen (krad_linker_t *krad_linker, int port);

krad_link_t *krad_linker_get_link_from_sysname (krad_linker_t *krad_linker, char *sysname);
krad_tags_t *krad_linker_get_tags_for_link (krad_linker_t *krad_linker, char *sysname);
krad_tags_t *krad_link_get_tags (krad_link_t *krad_link);

krad_linker_t *krad_linker_create ();
void krad_linker_destroy (krad_linker_t *krad_linker);
int krad_linker_handler ( krad_linker_t *krad_linker, krad_ipc_server_t *krad_ipc );
void krad_link_shutdown();
void krad_link_audio_samples_callback (int frames, void *userdata, float **samples);
void krad_link_destroy (krad_link_t *krad_link);
krad_link_t *krad_link_create (int linknum);
void krad_link_run (krad_link_t *krad_link);
#endif
