#include "krad_sdl_opengl_display.h"
#include "krad_ebml.h"
#include "krad_dirac.h"
#include "krad_vpx.h"
#include "krad_v4l2.h"
#include "krad_gui.h"
#include "krad_audio.h"
#include "krad_ring.h"
#include "krad_opus.h"
#include "krad_vorbis.h"
#include "krad_flac.h"

#define APPVERSION "Krad Cam 0.3"
#define MAX_AUDIO_CHANNELS 8

#define HELP -1337
#define NOWINDOW -20

int do_shutdown;

typedef struct krad_cam_St krad_cam_t;

typedef enum {
	CAPTURE = 200,
	RECEIVE,
	FAIL,
} krad_cam_operation_mode_t;

typedef enum {
	WINDOW = 300,
	COMMAND,
	DAEMON,
} krad_cam_interface_mode_t;

typedef enum {
	TEST = 500,
	V4L2,
	NOVIDEO,
} krad_video_source_t;

struct krad_cam_St {

	kradgui_t *krad_gui;
	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_vpx_decoder_t *krad_vpx_decoder;
	krad_audio_t *krad_audio;
	krad_vorbis_t *krad_vorbis;
	krad_flac_t *krad_flac;
	krad_opus_t *krad_opus;
	kradebml_t *krad_ebml;
	krad_v4l2_t *krad_v4l2;
	krad_sdl_opengl_display_t *krad_opengl_display;
	
	krad_cam_operation_mode_t operation_mode;
	krad_cam_interface_mode_t interface_mode;
	krad_video_source_t video_source;
	krad_audio_api_t krad_audio_api;
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
	char device[512];
	char alsa_capture_device[512];
	char alsa_playback_device[512];
	char output[512];
	char input[512];
	char host[512];
	int port;
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
	
	int capture_audio;
	krad_ringbuffer_t *audio_input_ringbuffer[MAX_AUDIO_CHANNELS];
	krad_ringbuffer_t *audio_output_ringbuffer[MAX_AUDIO_CHANNELS];
	float *samples[MAX_AUDIO_CHANNELS];
	int audio_encoder_ready;
	int audio_frames_captured;
	int audio_frames_encoded;
	
	krad_ringbuffer_t *decoded_video_ringbuffer;	
	krad_ringbuffer_t *decoded_audio_ringbuffer;
	krad_ringbuffer_t *encoded_audio_ringbuffer;
	krad_ringbuffer_t *encoded_video_ringbuffer;
	
	int video_frames_encoded;
	
	int composited_frame_byte_size;
	krad_ringbuffer_t *captured_frames_buffer;
	krad_ringbuffer_t *composited_frames_buffer;
	krad_ringbuffer_t *decoded_frames_buffer;
	
	struct SwsContext *captured_frame_converter;
	struct SwsContext *encoding_frame_converter;
	struct SwsContext *display_frame_converter;
	
	int video_track;
	int audio_track;

	pthread_t video_capture_thread;
	pthread_t video_encoding_thread;
	pthread_t audio_encoding_thread;
	pthread_t ebml_output_thread;

	pthread_t video_decoding_thread;
	pthread_t audio_decoding_thread;
	pthread_t ebml_input_thread;

	int audio_channels;

	int render_meters;

	int new_capture_frame;
	int cam_started;

	float temp_peak;
	float kick;

	struct timespec next_frame_time;
	struct timespec sleep_time;
	uint64_t next_frame_time_ms;

	int decoding;
	int input_ready;
	int verbose;	
	int vpx_bitrate;

	SDL_Event event;

	char bug[512];
	int bug_x;
	int bug_y;
	
	int *shutdown;
	int daemon;

	struct stat file_stat;
	
};


void krad_cam_activate(krad_cam_t *krad_cam);
void krad_cam_destroy(krad_cam_t *krad_cam);

void *video_capture_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	int dupe_frame = 0;
	int first = 1;
	struct timespec start_time;
	struct timespec current_time;
	struct timespec elapsed_time;
	void *captured_frame = NULL;
	unsigned char *captured_frame_rgb = malloc(krad_cam->composited_frame_byte_size); 
	
	if (krad_cam->verbose) {
		printf("\n\nvideo capture thread begins\n\n");
	}
	
	krad_cam->krad_v4l2 = kradv4l2_create();

	kradv4l2_open(krad_cam->krad_v4l2, krad_cam->device, krad_cam->capture_width, krad_cam->capture_height, krad_cam->capture_fps);
	
	kradv4l2_start_capturing (krad_cam->krad_v4l2);

	while (krad_cam->capturing == 1) {

		captured_frame = kradv4l2_read_frame_wait_adv (krad_cam->krad_v4l2);
	
		
		if (first == 1) {
			first = 0;
			krad_cam->captured_frames = 0;
			krad_cam->capture_audio = 1;
			clock_gettime(CLOCK_MONOTONIC, &start_time);
		} else {
		
		
			if ((krad_cam->verbose) && (krad_cam->captured_frames % 300 == 0)) {
		
				clock_gettime(CLOCK_MONOTONIC, &current_time);
				elapsed_time = timespec_diff(start_time, current_time);
				printf("Frames Captured: %u Expected: %ld - %u fps * %ld seconds\n", krad_cam->captured_frames, elapsed_time.tv_sec * krad_cam->capture_fps, krad_cam->capture_fps, elapsed_time.tv_sec);
	
			}
			
			krad_cam->captured_frames++;
	
		}
	
		krad_cam->mjpeg_mode = 0;
	
		if (krad_cam->mjpeg_mode == 0) {
	
			int rgb_stride_arr[3] = {4*krad_cam->composite_width, 0, 0};

			const uint8_t *yuv_arr[4];
			int yuv_stride_arr[4];
		
			yuv_arr[0] = captured_frame;

			yuv_stride_arr[0] = krad_cam->capture_width + (krad_cam->capture_width/2) * 2;
			yuv_stride_arr[1] = 0;
			yuv_stride_arr[2] = 0;
			yuv_stride_arr[3] = 0;

			unsigned char *dst[4];

			dst[0] = captured_frame_rgb;

			sws_scale(krad_cam->captured_frame_converter, yuv_arr, yuv_stride_arr, 0, krad_cam->capture_height, dst, rgb_stride_arr);
	
		} else {
		
			kradv4l2_jpeg_to_rgb (krad_cam->krad_v4l2, captured_frame_rgb, captured_frame, krad_cam->krad_v4l2->jpeg_size);
		
		}
	
		kradv4l2_frame_done (krad_cam->krad_v4l2);

	
		if (krad_ringbuffer_write_space(krad_cam->captured_frames_buffer) >= krad_cam->composited_frame_byte_size) {
	
			krad_ringbuffer_write(krad_cam->captured_frames_buffer, 
								 (char *)captured_frame_rgb, 
								 krad_cam->composited_frame_byte_size );
	
		}
		
		if (dupe_frame == 1) {

			//printf("\n\nduping a frame *********** bad should not happen\n\n");

			if (krad_ringbuffer_write_space(krad_cam->captured_frames_buffer) >= krad_cam->composited_frame_byte_size) {

				krad_ringbuffer_write(krad_cam->captured_frames_buffer, 
									 (char *)captured_frame_rgb, 
									 krad_cam->composited_frame_byte_size );

			}
			

			dupe_frame = 0;
		}

	
	}

	kradv4l2_stop_capturing (krad_cam->krad_v4l2);
	kradv4l2_close(krad_cam->krad_v4l2);
	kradv4l2_destroy(krad_cam->krad_v4l2);

	free(captured_frame_rgb);

	krad_cam->capture_audio = 2;
	krad_cam->encoding = 2;

	if (krad_cam->verbose) {
		printf("\n\nvideo capture thread ends\n\n");
	}
	
	return NULL;
	
}

void *video_encoding_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;
	
	printf("\n\nvideo encoding thread begins\n\n");
	
	void *vpx_packet;
	int keyframe;
	int packet_size;
	char keyframe_char[1];

	int first = 1;

	krad_cam->krad_vpx_encoder = krad_vpx_encoder_create(krad_cam->encoding_width, krad_cam->encoding_height, krad_cam->vpx_bitrate);


	krad_cam->krad_vpx_encoder->quality = 1000 * ((krad_cam->encoding_fps / 4) * 3);
	//printf("\n\n encoding quality set to %ld\n\n", krad_cam->krad_vpx_encoder->quality);

	while ((krad_cam->encoding == 1) || (krad_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= krad_cam->composited_frame_byte_size)) {

		if (krad_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= krad_cam->composited_frame_byte_size) {
	
			krad_ringbuffer_read(krad_cam->composited_frames_buffer, 
								 (char *)krad_cam->current_encoding_frame, 
								 krad_cam->composited_frame_byte_size );
								 
						
						
			if (krad_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= (krad_cam->composited_frame_byte_size * (krad_cam->encoding_buffer_frames / 2)) ) {
			
				krad_cam->krad_vpx_encoder->quality = (1000 * ((krad_cam->encoding_fps / 4) * 3)) / 2LU;
				
				//printf("\n\n encoding quality set to %ld\n\n", krad_cam->krad_vpx_encoder->quality);
			
			}
						
			if (krad_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= (krad_cam->composited_frame_byte_size * (krad_cam->encoding_buffer_frames / 4)) ) {
			
				krad_cam->krad_vpx_encoder->quality = (1000 * ((krad_cam->encoding_fps / 4) * 3)) / 4LU;
			
				//printf("\n\n encoding quality set to %ld\n\n", krad_cam->krad_vpx_encoder->quality);
			
			}						 
								 
			int rgb_stride_arr[3] = {4*krad_cam->composite_width, 0, 0};
			const uint8_t *rgb_arr[4];
	
			rgb_arr[0] = krad_cam->current_encoding_frame;

			sws_scale(krad_cam->encoding_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_cam->composite_height, 
					  krad_cam->krad_vpx_encoder->image->planes, krad_cam->krad_vpx_encoder->image->stride);
										 
			packet_size = krad_vpx_encoder_write(krad_cam->krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
								 
			//printf("packet size was %d\n", packet_size);
	
			
	
			if (packet_size) {

				keyframe_char[0] = keyframe;

				krad_ringbuffer_write(krad_cam->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write(krad_cam->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_write(krad_cam->encoded_video_ringbuffer, (char *)vpx_packet, packet_size);

				if (first == 1) {				
					first = 0;
				}
				
				krad_cam->video_frames_encoded++;
				
			}
	
		} else {
		
			if (krad_cam->krad_vpx_encoder->quality != (1000 * ((krad_cam->encoding_fps / 4) * 3))) {
				krad_cam->krad_vpx_encoder->quality = 1000 * ((krad_cam->encoding_fps / 4) * 3);
				//printf("\n\n encoding quality set to %ld\n\n", krad_cam->krad_vpx_encoder->quality);
			}

			
			usleep(4000);
		
		}
		
	}
	
	while (packet_size) {
		krad_vpx_encoder_finish(krad_cam->krad_vpx_encoder);
		packet_size = krad_vpx_encoder_write(krad_cam->krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
	}

	krad_vpx_encoder_destroy(krad_cam->krad_vpx_encoder);
	
	krad_cam->encoding = 3;
	
	printf("\n\nvideo encoding thread ends\n\n");
	
	return NULL;
	
}


void krad_cam_audio_callback(int frames, void *userdata) {

	krad_cam_t *krad_cam = (krad_cam_t *)userdata;
	
	int len;
	
	kradaudio_read (krad_cam->krad_audio, 0, (char *)krad_cam->samples[0], frames * 4 );
	kradaudio_read (krad_cam->krad_audio, 1, (char *)krad_cam->samples[1], frames * 4 );

	if (krad_cam->capture_audio == 1) {
		krad_cam->audio_frames_captured += frames;
		krad_ringbuffer_write(krad_cam->audio_input_ringbuffer[0], (char *)krad_cam->samples[0], frames * 4);
		krad_ringbuffer_write(krad_cam->audio_input_ringbuffer[1], (char *)krad_cam->samples[1], frames * 4);
	}
	
	if ((krad_cam->decoding) && (krad_cam->audio_codec == VORBIS)) {
	
		len = krad_vorbis_decoder_read_audio(krad_cam->krad_vorbis, 0, (char *)krad_cam->samples[0], frames * 4);

		if (len) {
			kradaudio_write (krad_cam->krad_audio, 0, (char *)krad_cam->samples[0], len );
		}

		len = krad_vorbis_decoder_read_audio(krad_cam->krad_vorbis, 1, (char *)krad_cam->samples[1], frames * 4);

		if (len) {
			kradaudio_write (krad_cam->krad_audio, 1, (char *)krad_cam->samples[1], len );
		}
	
	}
	
	if (krad_cam->capture_audio == 2) {
		krad_cam->capture_audio = 3;
	}
	
	
}

void *audio_encoding_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	int c;
	int s;
	int bytes;
	int frames;
	float *samples[MAX_AUDIO_CHANNELS];
	float *interleaved_samples;
	unsigned char *buffer;
	ogg_packet *op;
	int framecnt;
	
	krad_cam->audio_channels = 2;	
	
	if (krad_cam->krad_audio_api == ALSA) {
		krad_cam->krad_audio = kradaudio_create(krad_cam->alsa_capture_device, KINPUT, krad_cam->krad_audio_api);
	} else {
		krad_cam->krad_audio = kradaudio_create("Krad Link", KINPUT, krad_cam->krad_audio_api);
	}
	
	printf("audio_encoding_thread begin!\n");
		
	switch (krad_cam->audio_codec) {
		case VORBIS:
			krad_cam->krad_vorbis = krad_vorbis_encoder_create(krad_cam->audio_channels, krad_cam->krad_audio->sample_rate, 0.7);
			framecnt = 1024;
			break;
		case FLAC:
			krad_cam->krad_flac = krad_flac_encoder_create(krad_cam->audio_channels, krad_cam->krad_audio->sample_rate, 24);
			framecnt = 4096;
			break;
		case OPUS:
			krad_cam->krad_opus = kradopus_encoder_create(krad_cam->krad_audio->sample_rate, krad_cam->audio_channels, 192000, OPUS_APPLICATION_AUDIO);
			framecnt = 960;
			break;
		default:
			printf("unknown audio codec\n");
			exit(1);
	}


	for (c = 0; c < krad_cam->audio_channels; c++) {
		krad_cam->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_cam->samples[c] = malloc(4 * 8192);
	}

	interleaved_samples = calloc(1, 1000000);
	buffer = calloc(1, 1000000);

	kradaudio_set_process_callback(krad_cam->krad_audio, krad_cam_audio_callback, krad_cam);

	if (krad_cam->krad_audio_api == JACK) {
		krad_jack_t *jack = (krad_jack_t *)krad_cam->krad_audio->api;
		kradjack_connect_port(jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "Krad Cam:InputLeft");
		kradjack_connect_port(jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "Krad Cam:InputRight");
	}
	
	krad_cam->audio_encoder_ready = 1;
	
	if ((krad_cam->audio_codec == FLAC) || (krad_cam->audio_codec == OPUS)) {
		op = NULL;
	}
	
	while (krad_cam->encoding) {

		while ((krad_ringbuffer_read_space(krad_cam->audio_input_ringbuffer[1]) >= framecnt * 4) || (op != NULL)) {
			
			
			if (krad_cam->audio_codec == OPUS) {

				frames = 960;

				for (c = 0; c < krad_cam->audio_channels; c++) {
					krad_ringbuffer_read (krad_cam->audio_input_ringbuffer[c], (char *)samples[c], (frames * 4) );
					kradopus_write_audio(krad_cam->krad_opus, c + 1, (char *)samples[c], frames * 4);
				}

				bytes = kradopus_read_opus(krad_cam->krad_opus, buffer);
			
			}
			
			
			if (krad_cam->audio_codec == FLAC) {
			
				frames = 4096;				
				
				for (c = 0; c < krad_cam->audio_channels; c++) {
					krad_ringbuffer_read (krad_cam->audio_input_ringbuffer[c], (char *)samples[c], (frames * 4) );
				}
			
				for (s = 0; s < frames; s++) {
					for (c = 0; c < krad_cam->audio_channels; c++) {
						interleaved_samples[s * krad_cam->audio_channels + c] = samples[c][s];
					}
				}
			
				bytes = krad_flac_encode(krad_cam->krad_flac, interleaved_samples, frames, buffer);
	
			}
			
			
			if ((krad_cam->audio_codec == FLAC) || (krad_cam->audio_codec == OPUS)) {
	
				if (bytes > 0) {
					
					krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&bytes, 4);
					krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&frames, 4);
					krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)buffer, bytes);
					
					krad_cam->audio_frames_encoded += frames;
					bytes = 0;
				}
			}
			
			if (krad_cam->audio_codec == VORBIS) {
			
				op = krad_vorbis_encode(krad_cam->krad_vorbis, framecnt, krad_cam->audio_input_ringbuffer[0], krad_cam->audio_input_ringbuffer[1]);

				if (op != NULL) {
			
					frames = op->granulepos - krad_cam->audio_frames_encoded;
				
					krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&op->bytes, 4);
					krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&frames, 4);
					krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)op->packet, op->bytes);

				
					krad_cam->audio_frames_encoded = op->granulepos;
					
				}
			}

			//printf("audio_encoding_thread %zu !\n", krad_cam->audio_frames_encoded);			

		}
	
		while (krad_ringbuffer_read_space(krad_cam->audio_input_ringbuffer[1]) < framecnt * 4) {
	
			usleep(10000);
	
			if (krad_cam->encoding == 3) {
				break;
			}
	
		}
		
		if ((krad_cam->encoding == 3) && (krad_ringbuffer_read_space(krad_cam->audio_input_ringbuffer[1]) < framecnt * 4)) {
				break;
		}
		
	}
	printf("audio_encoding_thread end!\n");
	krad_cam->encoding = 4;
	
	while (krad_cam->capture_audio != 3) {
		usleep(5000);
	}
	
	for (c = 0; c < krad_cam->audio_channels; c++) {
		free(krad_cam->samples[c]);
		krad_ringbuffer_free ( krad_cam->audio_input_ringbuffer[c] );
		free(samples[c]);	
	}	
	
	free(interleaved_samples);
	free(buffer);
	
	return NULL;
	
}

void *ebml_output_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;


	unsigned char *packet;
	int packet_size;
	int keyframe;
	int frames;
	char keyframe_char[1];
	
	int video_frames_muxed;
	int audio_frames_muxed;
	int audio_frames_per_video_frame;

	printf("\n\nebml muxing thread begins\n\n");

	audio_frames_muxed = 0;
	video_frames_muxed = 0;

	while (krad_cam->krad_audio == NULL) {
		usleep(5000);
	}

	if (krad_cam->audio_codec == OPUS) {
		audio_frames_per_video_frame = 48000 / krad_cam->capture_fps;
	} else {
		audio_frames_per_video_frame = krad_cam->krad_audio->sample_rate / krad_cam->capture_fps;
	}
	
	packet = malloc(2000000);


	krad_cam->krad_ebml = kradebml_create();
	
	if (krad_cam->host[0] != '\0') {
		kradebml_open_output_stream(krad_cam->krad_ebml, krad_cam->host, krad_cam->port, krad_cam->mount, krad_cam->password);
	} else {
		printf("Outputing to file: %s\n", krad_cam->output);
		kradebml_open_output_file(krad_cam->krad_ebml, krad_cam->output);
	}
	
	while ( krad_cam->audio_encoder_ready != 1) {
	
		usleep(10000);
	
	}
	
	if ((krad_cam->audio_codec == VORBIS) && (krad_cam->video_codec == VP8)) {
		kradebml_header(krad_cam->krad_ebml, "webm", APPVERSION);
	} else {
		kradebml_header(krad_cam->krad_ebml, "matroska", APPVERSION);
	}
	
	if (krad_cam->video_source != NOVIDEO) {
	
		krad_cam->video_track = kradebml_add_video_track(krad_cam->krad_ebml, "V_VP8", krad_cam->encoding_fps,
											 			 krad_cam->encoding_width, krad_cam->encoding_height);
	}	
	
	if (krad_cam->audio_codec != NOCODEC) {
	
		switch (krad_cam->audio_codec) {
			case VORBIS:
				krad_cam->audio_track = kradebml_add_audio_track(krad_cam->krad_ebml, "A_VORBIS", krad_cam->krad_audio->sample_rate, krad_cam->audio_channels, krad_cam->krad_vorbis->header, 
																 krad_cam->krad_vorbis->headerpos);
				break;
			case FLAC:
				krad_cam->audio_track = kradebml_add_audio_track(krad_cam->krad_ebml, "A_FLAC", krad_cam->krad_audio->sample_rate, krad_cam->audio_channels, (unsigned char *)krad_cam->krad_flac->min_header, FLAC_MINIMAL_HEADER_SIZE);
				break;
			case OPUS:
				krad_cam->audio_track = kradebml_add_audio_track(krad_cam->krad_ebml, "A_OPUS", 48000, krad_cam->audio_channels, krad_cam->krad_opus->header_data, krad_cam->krad_opus->header_data_size);
				break;
			default:
				printf("unknown audio codec\n");
				exit(1);
		}
	
	}
	
	kradebml_write(krad_cam->krad_ebml);
	
	printf("\n\nebml muxing thread waiting..\n\n");
	
	while ( krad_cam->encoding ) {

		if (krad_cam->video_source != NOVIDEO) {
			if ((krad_ringbuffer_read_space(krad_cam->encoded_video_ringbuffer) >= 4) && (krad_cam->encoding < 3)) {

				krad_ringbuffer_read(krad_cam->encoded_video_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_cam->encoded_video_ringbuffer) < packet_size + 1) && (krad_cam->encoding < 3)) {
					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space(krad_cam->encoded_video_ringbuffer) < packet_size + 1) && (krad_cam->encoding > 2)) {
					continue;
				}
			
				krad_ringbuffer_read(krad_cam->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_read(krad_cam->encoded_video_ringbuffer, (char *)packet, packet_size);

				keyframe = keyframe_char[0];
	

				kradebml_add_video(krad_cam->krad_ebml, krad_cam->video_track, packet, packet_size, keyframe);

				video_frames_muxed++;
		
			}
		}
		
		
		if (krad_cam->audio_codec != NOCODEC) {
		
			if ((krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) >= 4) && 
				((krad_cam->video_source != NOVIDEO) || ((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed))) {

				krad_ringbuffer_read(krad_cam->encoded_audio_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) < packet_size + 4) && (krad_cam->encoding != 4)) {
					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) < packet_size + 4) && (krad_cam->encoding == 4)) {
					break;
				}
			
				krad_ringbuffer_read(krad_cam->encoded_audio_ringbuffer, (char *)&frames, 4);
				krad_ringbuffer_read(krad_cam->encoded_audio_ringbuffer, (char *)packet, packet_size);
	

				kradebml_add_audio(krad_cam->krad_ebml, krad_cam->audio_track, packet, packet_size, frames);

				audio_frames_muxed += frames;
				
				//printf("ebml muxed audio frames: %d\n\n", audio_frames_muxed);
				
			} else {
				if (krad_cam->encoding == 4) {
					break;
				}
			}
		}
		
		
		if ((krad_cam->audio_codec != NOCODEC) && (!(krad_cam->video_source))) {
		
			if (krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) < 4) {
				
				if (krad_cam->encoding == 4) {
					break;
				}
		
				usleep(10000);
			
			}
		
		}
		
		if ((!(krad_cam->audio_codec != NOCODEC)) && (krad_cam->video_source)) {
		
			if (krad_ringbuffer_read_space(krad_cam->encoded_video_ringbuffer) < 4) {
		
				if (krad_cam->encoding == 4) {
					break;
				}
		
				usleep(10000);
			
			}
		
		}


		if ((krad_cam->audio_codec != NOCODEC) && (krad_cam->video_source)) {

			if (((krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) < 4) || 
				((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed)) && 
			   (krad_ringbuffer_read_space(krad_cam->encoded_video_ringbuffer) < 4)) {
		
				if (krad_cam->encoding == 4) {
					break;
				}
		
				usleep(10000);
			
			}
		
		
		}		
		
	
	}

	printf("ebml thred end!\n");

	kradebml_destroy(krad_cam->krad_ebml);
	
	free(packet);
	
	//printf("\n\nebml muxing thread ends\n\n");
	
	return NULL;
	
}

void krad_cam_handle_input(krad_cam_t *krad_cam) {

	while ( SDL_PollEvent( &krad_cam->event ) ){
		switch( krad_cam->event.type ){
			/* Look for a keypress */
			case SDL_KEYDOWN:
				/* Check the SDLKey values and move change the coords */
				switch( krad_cam->event.key.keysym.sym ){
					case SDLK_LEFT:
						break;
					case SDLK_RIGHT:
						break;
					case SDLK_UP:
						break;
					case SDLK_z:
						kradgui_remove_bug (krad_cam->krad_gui);
						break;
					/*			        
					case SDLK_m:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/DinoHead-r2_small.png", 30, 30);
						break;
					case SDLK_x:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/fish_xiph_org.png", 30, 30);
						break;
					case SDLK_h:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/html_5logo.png", 30, 30);
						break;
					case SDLK_w:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/WebM-logo1.png", 30, 30);
						break;
					case SDLK_e:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/airmoz3.png", 30, 30);
						break;
					case SDLK_r:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/airmoz4.png", 30, 30);
						break;					        
					case SDLK_s:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/airmoz5.png", 30, 30);
						break;
					case SDLK_a:
						kradgui_set_bug (krad_cam->krad_gui, "/home/oneman/Pictures/airmoz6.png", 30, 30);
						break;
					*/
					case SDLK_b:
						if (krad_cam->bug[0] != '\0') {
							kradgui_set_bug (krad_cam->krad_gui, krad_cam->bug, krad_cam->bug_x, krad_cam->bug_y);
						}
						break;
					case SDLK_l:
						if (krad_cam->krad_gui->live == 1) {
							krad_cam->krad_gui->live = 0;
						} else {
							krad_cam->krad_gui->live = 1;
						}
						break;
					case SDLK_0:
						krad_cam->krad_gui->bug_alpha += 0.1f;
						if (krad_cam->krad_gui->bug_alpha > 1.0f) {
							krad_cam->krad_gui->bug_alpha = 1.0f;
						}
						break;
					case SDLK_9:
						krad_cam->krad_gui->bug_alpha -= 0.1f;
						if (krad_cam->krad_gui->bug_alpha < 0.1f) {
							krad_cam->krad_gui->bug_alpha = 0.1f;
						}
						break;
					case SDLK_q:
						*krad_cam->shutdown = 1;
						break;
					case SDLK_f:
						if (krad_cam->krad_gui->render_ftest == 1) {
							krad_cam->krad_gui->render_ftest = 0;
						} else {
							krad_cam->krad_gui->render_ftest = 1;
						}
						break;
					case SDLK_v:
						if (krad_cam->render_meters == 1) {
							krad_cam->render_meters = 0;
						} else {
							krad_cam->render_meters = 1;
						}
						break;
					default:
						break;
				}
				break;

			case SDL_KEYUP:
				switch( krad_cam->event.key.keysym.sym ) {
					case SDLK_LEFT:
						break;
					case SDLK_RIGHT:
						break;
					case SDLK_UP:
						break;
					case SDLK_DOWN:
						break;
					default:
						break;
				}
				break;

			case SDL_MOUSEMOTION:
				//printf("mouse!\n");
				krad_cam->krad_gui->cursor_x = krad_cam->event.motion.x;
				krad_cam->krad_gui->cursor_y = krad_cam->event.motion.y;
				break;
	
			case SDL_QUIT:
				*krad_cam->shutdown = 1;	 			
				break;

			default:
				break;
		}
	}

}

void krad_cam_display(krad_cam_t *krad_cam) {

	if ((krad_cam->composite_width == krad_cam->display_width) && (krad_cam->composite_height == krad_cam->display_height)) {

		memcpy( krad_cam->krad_opengl_display->rgb_frame_data, krad_cam->krad_gui->data, krad_cam->krad_gui->bytes );

	} else {


		int rgb_stride_arr[3] = {4*krad_cam->display_width, 0, 0};

		const uint8_t *rgb_arr[4];
		int rgb1_stride_arr[4];

		rgb_arr[0] = krad_cam->krad_gui->data;

		rgb1_stride_arr[0] = krad_cam->composite_width * 4;
		rgb1_stride_arr[1] = 0;
		rgb1_stride_arr[2] = 0;
		rgb1_stride_arr[3] = 0;

		unsigned char *dst[4];

		dst[0] = krad_cam->krad_opengl_display->rgb_frame_data;

		sws_scale(krad_cam->display_frame_converter, rgb_arr, rgb1_stride_arr, 0, krad_cam->display_height, dst, rgb_stride_arr);


	}

	krad_sdl_opengl_draw_screen( krad_cam->krad_opengl_display );

	//if (read_composited) {
	//	krad_sdl_opengl_read_screen( krad_cam->krad_opengl_display, krad_cam->krad_opengl_display->rgb_frame_data);
	//}

}

void krad_cam_composite(krad_cam_t *krad_cam) {

	int c;

	if (krad_cam->capturing) {
		if (krad_ringbuffer_read_space(krad_cam->captured_frames_buffer) >= krad_cam->composited_frame_byte_size) {
	
			krad_ringbuffer_read(krad_cam->captured_frames_buffer, 
									 (char *)krad_cam->current_frame, 
									 krad_cam->composited_frame_byte_size );
		
			if (krad_cam->cam_started == 0) {
				krad_cam->cam_started = 1;
				//krad_cam->krad_gui->render_ftest = 1;
				kradgui_go_live(krad_cam->krad_gui);
			}
		
			krad_cam->new_capture_frame = 1;
		} else {
			krad_cam->new_capture_frame = 0;
		}

		memcpy( krad_cam->krad_gui->data, krad_cam->current_frame, krad_cam->krad_gui->bytes );
	
	} else {
		krad_cam->capture_audio = 1;
		krad_cam->new_capture_frame = 1;
		krad_cam->cam_started = 1;
	}
	
	
	
	kradgui_render( krad_cam->krad_gui );

	if (krad_cam->new_capture_frame == 1) {
		if (krad_cam->krad_audio != NULL) {
			for (c = 0; c < krad_cam->audio_channels; c++) {
				krad_cam->temp_peak = read_peak(krad_cam->krad_audio, KINPUT, c);
				if (krad_cam->temp_peak >= krad_cam->krad_gui->output_peak[c]) {
					if (krad_cam->temp_peak > 2.7f) {
						krad_cam->krad_gui->output_peak[c] = krad_cam->temp_peak;
						krad_cam->kick = ((krad_cam->krad_gui->output_peak[c] - krad_cam->krad_gui->output_current[c]) / 300.0);
					}
				} else {
					if (krad_cam->krad_gui->output_peak[c] == krad_cam->krad_gui->output_current[c]) {
						krad_cam->krad_gui->output_peak[c] -= (0.9 * (60/krad_cam->capture_fps));
						if (krad_cam->krad_gui->output_peak[c] < 0.0f) {
							krad_cam->krad_gui->output_peak[c] = 0.0f;
						}
						krad_cam->krad_gui->output_current[c] = krad_cam->krad_gui->output_peak[c];
					}
				}
			
				if (krad_cam->krad_gui->output_peak[c] > krad_cam->krad_gui->output_current[c]) {
					krad_cam->krad_gui->output_current[c] = (krad_cam->krad_gui->output_current[c] + 1.4) * (1.3 + krad_cam->kick); ;
				}
		
				if (krad_cam->krad_gui->output_peak[c] < krad_cam->krad_gui->output_current[c]) {
					krad_cam->krad_gui->output_current[c] = krad_cam->krad_gui->output_peak[c];
				}
			
			
			}
		}
	}
	
	if (krad_cam->render_meters) {
		kradgui_render_meter (krad_cam->krad_gui, 110, krad_cam->composite_height - 30, 96, krad_cam->krad_gui->output_current[0]);
		kradgui_render_meter (krad_cam->krad_gui, krad_cam->composite_width - 110, krad_cam->composite_height - 30, 96, krad_cam->krad_gui->output_current[1]);
	}
	
	
	if ((krad_cam->cam_started == 1) && (krad_cam->new_capture_frame == 1) && (krad_ringbuffer_write_space(krad_cam->composited_frames_buffer) >= krad_cam->composited_frame_byte_size)) {

			krad_ringbuffer_write(krad_cam->composited_frames_buffer, 
								  (char *)krad_cam->krad_gui->data, 
								  krad_cam->composited_frame_byte_size );

	} else {
	
		if ((krad_cam->cam_started == 1) && (krad_cam->new_capture_frame == 1)) {
			printf("encoding to slow! overflow!\n");
		}
	
	}
		
		
}


void krad_cam_run(krad_cam_t *krad_cam) {

	krad_cam_activate ( krad_cam );

	if (krad_cam->operation_mode == RECEIVE) {
	
		while (krad_cam->input_ready != 1) {
			usleep(5000);
			if (*krad_cam->shutdown) {
				break;
			}
		}
	
		if ((!*krad_cam->shutdown) && (krad_cam->input_ready) && (krad_cam->interface_mode == WINDOW)) {
	
			//krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, 1920, 1080, krad_ebml->vparams.width, krad_ebml->vparams.height);
			krad_cam->krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, krad_cam->krad_ebml->vparams.width, krad_cam->krad_ebml->vparams.height, krad_cam->krad_ebml->vparams.width, krad_cam->krad_ebml->vparams.height);
	
		}
	
		while (!*krad_cam->shutdown) {

			if (krad_cam->video_source == NOVIDEO) {
	
				usleep(60000);

			} else {

				while (krad_ringbuffer_read_space(krad_cam->decoded_frames_buffer) < krad_cam->composited_frame_byte_size) {
					usleep(5000);
				}
				
				krad_cam_composite(krad_cam);

				if (krad_cam->interface_mode == WINDOW) {
					krad_cam_display(krad_cam);
					krad_cam_handle_input(krad_cam);
				}
			
			
			
				//if (!krad_cam->capturing) {
				
					kradgui_update_elapsed_time(krad_cam->krad_gui);

					krad_cam->next_frame_time_ms = krad_cam->krad_gui->frame * (1000.0f / (float)krad_cam->composite_fps);
	
					krad_cam->next_frame_time.tv_sec = (krad_cam->next_frame_time_ms - (krad_cam->next_frame_time_ms % 1000)) / 1000;
					krad_cam->next_frame_time.tv_nsec = (krad_cam->next_frame_time_ms % 1000) * 1000000;
	
					krad_cam->sleep_time = timespec_diff(krad_cam->krad_gui->elapsed_time, krad_cam->next_frame_time);
	
					if ((krad_cam->sleep_time.tv_sec > -1) && (krad_cam->sleep_time.tv_nsec > 0))  {
						nanosleep (&krad_cam->sleep_time, NULL);
					}
	
					printf("Elapsed time %s\r", krad_cam->krad_gui->elapsed_time_timecode_string);
					fflush(stdout);
		
				//}
		
			}
		}

	
	} else {

		while (!*krad_cam->shutdown) {

			if (krad_cam->video_source == NOVIDEO) {
	
				usleep(60000);

			} else {

				if ((krad_cam->capturing) && (krad_cam->interface_mode != WINDOW)) {
					while (krad_ringbuffer_read_space(krad_cam->captured_frames_buffer) < krad_cam->composited_frame_byte_size) {
						usleep(5000);
					}
				}

				if (!krad_cam->capturing) {
				
				}

				krad_cam_composite ( krad_cam );

				if (krad_cam->interface_mode == WINDOW) {
					krad_cam_display(krad_cam);
					krad_cam_handle_input(krad_cam);
				}
			
			
			
				if (!krad_cam->capturing) {
				
					kradgui_update_elapsed_time(krad_cam->krad_gui);

					krad_cam->next_frame_time_ms = krad_cam->krad_gui->frame * (1000.0f / (float)krad_cam->composite_fps);
	
					krad_cam->next_frame_time.tv_sec = (krad_cam->next_frame_time_ms - (krad_cam->next_frame_time_ms % 1000)) / 1000;
					krad_cam->next_frame_time.tv_nsec = (krad_cam->next_frame_time_ms % 1000) * 1000000;
	
					krad_cam->sleep_time = timespec_diff(krad_cam->krad_gui->elapsed_time, krad_cam->next_frame_time);
	
					if ((krad_cam->sleep_time.tv_sec > -1) && (krad_cam->sleep_time.tv_nsec > 0))  {
						nanosleep (&krad_cam->sleep_time, NULL);
					}
	
					printf("Elapsed time %s\r", krad_cam->krad_gui->elapsed_time_timecode_string);
					fflush(stdout);
		
				}
		
			}
		}
	}


	krad_cam_destroy ( krad_cam );

}

void *ebml_input_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	printf("\n\nebml demuxing thread begins\n\n");


	unsigned char *buffer;
	int bytes;
	int p;
	int video_packets;
	int audio_packets;
	int audio_tracknum;
	video_packets = 0;
	audio_packets = 0;

	buffer = malloc(2000000);
	bytes = 0;

	//krad_dirac = krad_dirac_decoder_create();
	krad_cam->krad_vpx_decoder = krad_vpx_decoder_create();
	krad_cam->krad_ebml = kradebml_create();
	krad_cam->krad_flac = krad_flac_decoder_create();
	
	if (krad_cam->host[0] != '\0') {
		kradebml_open_input_stream(krad_cam->krad_ebml, krad_cam->host, krad_cam->port, krad_cam->mount);
	} else {
		kradebml_open_input_file(krad_cam->krad_ebml, krad_cam->input);
	}
	kradebml_debug(krad_cam->krad_ebml);

	kradgui_set_total_track_time_ms(krad_cam->krad_gui, (krad_cam->krad_ebml->duration / 1e9) * 1000);
	krad_cam->krad_gui->render_timecode = 1;
	
	krad_cam->video_codec = krad_ebml_track_codec(krad_cam->krad_ebml, 0);
	krad_cam->audio_codec = krad_ebml_track_codec(krad_cam->krad_ebml, 1);
	audio_tracknum = 1;
	
	if (krad_cam->krad_ebml->tracks == 1) {
		krad_cam->audio_codec = krad_ebml_track_codec(krad_cam->krad_ebml, 0);
		audio_tracknum = 0;
	}
	
	printf("video codec is %d and audio codec is %d\n", krad_cam->video_codec, krad_cam->audio_codec);
	
	if (krad_cam->audio_codec == FLAC) {
		krad_cam->krad_flac = krad_flac_decoder_create();
		bytes = kradebml_read_audio_header(krad_cam->krad_ebml, audio_tracknum, buffer);
		printf("got flac header bytes of %d\n", bytes);
		//exit(1);
		krad_flac_decode(krad_cam->krad_flac, buffer, bytes, NULL);
	}
	
	if (krad_cam->audio_codec == VORBIS) {
		printf("got vorbis header bytes of %d %d %d\n", krad_cam->krad_ebml->vorbis_header1_len, krad_cam->krad_ebml->vorbis_header2_len, krad_cam->krad_ebml->vorbis_header3_len);
		krad_cam->krad_vorbis = krad_vorbis_decoder_create(krad_cam->krad_ebml->vorbis_header1, krad_cam->krad_ebml->vorbis_header1_len, krad_cam->krad_ebml->vorbis_header2, krad_cam->krad_ebml->vorbis_header2_len, krad_cam->krad_ebml->vorbis_header3, krad_cam->krad_ebml->vorbis_header3_len);
	}
	
	if (krad_cam->audio_codec == OPUS) {
		bytes = kradebml_read_audio_header(krad_cam->krad_ebml, 1, buffer);
		printf("got opus header bytes of %d\n", bytes);
		krad_cam->krad_opus = kradopus_decoder_create(buffer, bytes, 44100.0f);
	}
	
	//if (video_codec == KRAD_THEORA) {
	//	printf("got theora header bytes of %d %d %d\n", krad_ebml->theora_header1_len, krad_ebml->theora_header2_len, krad_ebml->theora_header3_len);
	//	krad_theora_decoder = krad_theora_decoder_create(krad_ebml->theora_header1, krad_ebml->theora_header1_len, krad_ebml->theora_header2, krad_ebml->theora_header2_len, krad_ebml->theora_header3, krad_ebml->theora_header3_len);
	//} else {
	//	krad_theora_decoder = NULL;
	//}

	krad_cam->input_ready = 1;

	kradgui_reset_elapsed_time(krad_cam->krad_gui);
	
	while ((!*krad_cam->shutdown) && (nestegg_read_packet(krad_cam->krad_ebml->ctx, &krad_cam->krad_ebml->pkt) > 0)) {
		
		nestegg_packet_track(krad_cam->krad_ebml->pkt, &krad_cam->krad_ebml->pkt_track);
		nestegg_packet_count(krad_cam->krad_ebml->pkt, &krad_cam->krad_ebml->pkt_cnt);
		nestegg_packet_tstamp(krad_cam->krad_ebml->pkt, &krad_cam->krad_ebml->pkt_tstamp);
		
		
		while (((krad_cam->krad_audio != NULL) && (kradaudio_buffered_frames(krad_cam->krad_audio) > 44100)) || 
			(krad_ringbuffer_write_space(krad_cam->encoded_audio_ringbuffer) < 100000))
		{
			usleep(10000);
		}
		
		if (krad_cam->video_source != NOVIDEO) {
		
			if (krad_cam->krad_ebml->pkt_track == krad_cam->krad_ebml->video_track) {


				//printf("\npacket time ms %d :: %ld : %ld\n", packet_time_ms % 1000, packet_time.tv_sec, packet_time.tv_nsec);
	//			kradgui_set_current_track_time_ms(krad_cam->krad_gui, packet_time_ms);
		
				video_packets++;

				for (p = 0; p < krad_cam->krad_ebml->pkt_cnt; ++p) {

					nestegg_packet_data(krad_cam->krad_ebml->pkt, p, &buffer, &krad_cam->krad_ebml->size);
	
					krad_ringbuffer_write(krad_cam->encoded_video_ringbuffer, (char *)&krad_cam->krad_ebml->size, 4);
					//krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&frames, 4);

					krad_ringbuffer_write(krad_cam->encoded_video_ringbuffer, (char *)buffer, krad_cam->krad_ebml->size);

				}
			}
		}


		
		
		if (krad_cam->krad_ebml->pkt_track == krad_cam->krad_ebml->audio_track) {
			audio_packets++;

			nestegg_packet_data(krad_cam->krad_ebml->pkt, 0, &buffer, &krad_cam->krad_ebml->size);
			printf("\nAudio packet! %zu bytes\n", krad_cam->krad_ebml->size);

			krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&krad_cam->krad_ebml->size, 4);
			//krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)&frames, 4);
			krad_ringbuffer_write(krad_cam->encoded_audio_ringbuffer, (char *)buffer, krad_cam->krad_ebml->size);
			
		}
		
		if ((krad_cam->krad_ebml->pkt_track == krad_cam->krad_ebml->video_track) && (krad_cam->krad_audio != NULL)) {
			printf("Timecode: %f :: Video Packets %d Audio Packets: %d buf frames %zu\r", krad_cam->krad_ebml->pkt_tstamp / 1e9, video_packets, audio_packets, kradaudio_buffered_frames(krad_cam->krad_audio));
			fflush(stdout);
		}
		
		nestegg_free_packet(krad_cam->krad_ebml->pkt);

	}

	free(buffer);
	
	printf("\n\nebml demuxing thread ends\n\n");


	return NULL;

}


void *video_decoding_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	printf("\n\nvideo decoding thread begins\n\n");


	/*
				if (video_codec == KRAD_VP8) {

					krad_vpx_decoder_decode(krad_vpx_decoder, buffer, krad_ebml->size);
				
					if (krad_vpx_decoder->img != NULL) {

						//printf("vpx img: %d %d %d\n", krad_vpx_decoder->img->stride[0],  krad_vpx_decoder->img->stride[1],  krad_vpx_decoder->img->stride[2]); 
						//cr = cairo_create(cst);
						//kradgui->cr = cr;
						//kradgui_render(kradgui);
						//cairo_destroy(cr);

						//krad_sdl_opengl_display_hud_item(krad_opengl_display, hud_data);
						krad_sdl_opengl_display_render(krad_opengl_display, krad_vpx_decoder->img->planes[0], krad_vpx_decoder->img->stride[0], krad_vpx_decoder->img->planes[1], krad_vpx_decoder->img->stride[1], krad_vpx_decoder->img->planes[2], krad_vpx_decoder->img->stride[2]);

						//usleep(13000);
					}
				}
				
				if (video_codec == KRAD_THEORA) {

					krad_theora_decoder_decode(krad_theora_decoder, buffer, krad_ebml->size);
				
					//if (krad_vpx_decoder->img != NULL) {

						//printf("vpx img: %d %d %d\n", krad_vpx_decoder->img->stride[0],  krad_vpx_decoder->img->stride[1],  krad_vpx_decoder->img->stride[2]); 
						//cr = cairo_create(cst);
						//kradgui->cr = cr;
						//kradgui_render(kradgui);
						//cairo_destroy(cr);

						//krad_sdl_opengl_display_hud_item(krad_opengl_display, hud_data);
					krad_sdl_opengl_display_render(krad_opengl_display, krad_theora_decoder->ycbcr[0].data, krad_theora_decoder->ycbcr[0].stride, krad_theora_decoder->ycbcr[1].data, krad_theora_decoder->ycbcr[1].stride, krad_theora_decoder->ycbcr[2].data, krad_theora_decoder->ycbcr[2].stride);

						//usleep(13000);
					//}
				}

							
				if (video_codec == KRAD_DIRAC) {
				
					//printf("dirac packet size %zu\n", krad_ebml->size);		

					krad_dirac_decode(krad_dirac, buffer, krad_ebml->size);

					if ((krad_dirac->format != NULL) && (dirac_output_unset == true)) {
						switch (krad_dirac->format->chroma_format) {
	
							case SCHRO_CHROMA_444:
								krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV444P);
							case SCHRO_CHROMA_422:
								krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV422P);
							case SCHRO_CHROMA_420:
								break;
								// default
								//krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display, PIX_FMT_YUV420P);

						}
				
						dirac_output_unset = false;				
					}

					if (krad_dirac->frame != NULL) {
						krad_sdl_opengl_display_render(krad_opengl_display, krad_dirac->frame->components[0].data, krad_dirac->frame->components[0].stride, krad_dirac->frame->components[1].data, krad_dirac->frame->components[1].stride, krad_dirac->frame->components[2].data, krad_dirac->frame->components[2].stride);
					}

				}

	*/

	printf("\n\nvideo decoding thread ends\n\n");


	return NULL;

}

void *audio_decoding_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	printf("\n\naudio decoding thread begins\n\n");

	int c;
	int bytes;
	unsigned char *buffer;
	float *audio;
	float *samples[MAX_AUDIO_CHANNELS];
	int audio_frames;
		
	while (krad_cam->input_ready != 1) {
		usleep(5000);
		if (*krad_cam->shutdown) {
			return NULL;
		}
	}
	
	krad_cam->audio_channels = 2;

	for (c = 0; c < krad_cam->audio_channels; c++) {
		krad_cam->audio_output_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_cam->samples[c] = malloc(4 * 8192);
	}
	
	buffer = malloc(2000000);
	audio = calloc(1, 8192 * 4 * 4);


	if (krad_cam->krad_audio_api == ALSA) {
		krad_cam->krad_audio = kradaudio_create(krad_cam->alsa_playback_device, KOUTPUT, krad_cam->krad_audio_api);
	} else {
		krad_cam->krad_audio = kradaudio_create("Krad Link", KOUTPUT, krad_cam->krad_audio_api);
	}

	if (krad_cam->audio_codec == VORBIS) {
		kradaudio_set_process_callback(krad_cam->krad_audio, krad_cam_audio_callback, krad_cam);
	}	
	
	while (!*krad_cam->shutdown) {


		while (krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) < 4) {
			usleep(10000);
		}
		
		krad_ringbuffer_read(krad_cam->encoded_audio_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_cam->encoded_audio_ringbuffer) < bytes) && (krad_cam->decoding) && (!*krad_cam->shutdown)) {
			usleep(10000);
		}

		//krad_ringbuffer_read(krad_cam->encoded_audio_ringbuffer, (char *)&frames, 4);
		krad_ringbuffer_read(krad_cam->encoded_audio_ringbuffer, (char *)buffer, bytes);
		
			printf("deocded vorbis byteeeeeeeees %d\n", bytes);
		if (krad_cam->audio_codec == VORBIS) {
			krad_vorbis_decoder_decode(krad_cam->krad_vorbis, buffer, bytes);
			printf("deocded vorbis bytes %d\n", bytes);
		}
			
		if (krad_cam->audio_codec == FLAC) {
			audio_frames = krad_flac_decode(krad_cam->krad_flac, buffer, bytes, samples);
			
			//for (s = 0; s < audio_frames; s++) {
			//	for (c = 0; c < krad_cam->audio_channels; c++) {
			//		samples[c][s] = audio[s * krad_cam->audio_channels + c];
			//	}
			//}
			
			for (c = 0; c < krad_cam->audio_channels; c++) {
				kradaudio_write (krad_cam->krad_audio, c, (char *)samples[c], audio_frames * 4 );
			}
		}
			
		if (krad_cam->audio_codec == OPUS) {
			kradopus_decoder_set_speed(krad_cam->krad_opus, 100);
			kradopus_write_opus(krad_cam->krad_opus, buffer, bytes);

			int c;

			bytes = -1;			
			while (bytes != 0) {
				for (c = 0; c < 2; c++) {
		
					bytes = kradopus_read_audio(krad_cam->krad_opus, c + 1, (char *)audio, 960 * 4);
					if (bytes) {
						printf("\nAudio data! %d samplers\n", bytes / 4);
						kradaudio_write (krad_cam->krad_audio, c, (char *)audio, bytes );
					}
				}
			}
		}
	}

	free(buffer);
	free(audio);
	
	
	for (c = 0; c < krad_cam->audio_channels; c++) {
		free(krad_cam->samples[c]);
		krad_ringbuffer_free ( krad_cam->audio_output_ringbuffer[c] );
		free(samples[c]);	
	}	
	
	printf("\n\naudio decoding thread ends\n\n");


	return NULL;

}

void daemonize() {

		char *daemon_name = "krad_cam";

		/* Our process ID and Session ID */
		pid_t pid, sid;

        printf("Daemonizing..\n");
 
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
 
        /* Change the file mode mask */
        umask(0);
 
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        //close(STDOUT_FILENO);
        //close(STDERR_FILENO);
        
        
		char err_log_file[98] = "";
		char log_file[98] = "";
		char *homedir = getenv ("HOME");
		
		char timestamp[64];
		
		time_t ltime;
    	ltime=time(NULL);
		sprintf(timestamp, "%s",asctime( localtime(&ltime) ) );
		
		int i;
		
		for (i = 0; i< strlen(timestamp); i++) {
			if (timestamp[i] == ' ') {
				timestamp[i] = '_';
			}
			if (timestamp[i] == ':') {
				timestamp[i] = '.';
			}
			if (timestamp[i] == '\n') {
				timestamp[i] = '\0';
			}
		
		}
		
		sprintf(err_log_file, "%s/%s_%s_err.log", homedir, daemon_name, timestamp);
		sprintf(log_file, "%s/%s_%s.log", homedir, daemon_name, timestamp);
		
		FILE *fp;
		
		fp = freopen( err_log_file, "w", stderr );
		if (fp == NULL) {
			printf("ruh oh error in freopen stderr\n");
		}
		fp = freopen( log_file, "w", stdout );
		if (fp == NULL) {
			printf("ruh oh error in freopen stdout\n");
		}
        
}

void krad_cam_shutdown() {

	if (do_shutdown == 1) {
		exit(1);
	}

	do_shutdown = 1;

}

void krad_cam_destroy(krad_cam_t *krad_cam) {

	printf("destroy krad cam!\n");

	krad_cam->capturing = 0;
	
	if (krad_cam->capturing) {
		krad_cam->capturing = 0;
		pthread_join(krad_cam->video_capture_thread, NULL);
	} else {
	
		krad_cam->capture_audio = 2;
		krad_cam->encoding = 2;
	
	}
	
	if (krad_cam->krad_vpx_decoder) {
		krad_vpx_decoder_destroy(krad_cam->krad_vpx_decoder);
	}
	
	if (krad_cam->video_source != NOVIDEO) {
		pthread_join(krad_cam->video_encoding_thread, NULL);
	} else {
		krad_cam->encoding = 3;
	}
	
	if (krad_cam->audio_codec != NOCODEC) {
		pthread_join(krad_cam->audio_encoding_thread, NULL);
	}
	
	pthread_join(krad_cam->ebml_output_thread, NULL);

	if (krad_cam->interface_mode == WINDOW) {
		krad_sdl_opengl_display_destroy(krad_cam->krad_opengl_display);
	}

	kradgui_destroy(krad_cam->krad_gui);

	sws_freeContext ( krad_cam->captured_frame_converter );
	sws_freeContext ( krad_cam->encoding_frame_converter );
	sws_freeContext ( krad_cam->display_frame_converter );

	// must be before vorbis
	kradaudio_destroy (krad_cam->krad_audio);
	
	if (krad_cam->krad_vorbis != NULL) {
		krad_vorbis_encoder_destroy (krad_cam->krad_vorbis);
	}
	
	if (krad_cam->krad_flac != NULL) {
		krad_flac_encoder_destroy (krad_cam->krad_flac);
	}

	if (krad_cam->krad_opus != NULL) {
		kradopus_encoder_destroy(krad_cam->krad_opus);
	}
	
	krad_ringbuffer_free ( krad_cam->captured_frames_buffer );

	krad_ringbuffer_free ( krad_cam->encoded_audio_ringbuffer );
	krad_ringbuffer_free ( krad_cam->encoded_video_ringbuffer );

	free(krad_cam->current_encoding_frame);
	free(krad_cam->current_frame);

	free(krad_cam);
}

krad_cam_t *krad_cam_create() {

	krad_cam_t *krad_cam;
	
	krad_cam = calloc(1, sizeof(krad_cam_t));
		
	krad_cam->capture_buffer_frames = 5;
	krad_cam->encoding_buffer_frames = 15;
	
	krad_cam->capture_width = 640;
	krad_cam->capture_height = 480;
	krad_cam->capture_fps = 15;

	krad_cam->composite_fps = krad_cam->capture_fps;
	krad_cam->encoding_fps = krad_cam->capture_fps;
	krad_cam->composite_width = krad_cam->capture_width;
	krad_cam->composite_height = krad_cam->capture_height;
	krad_cam->encoding_width = krad_cam->capture_width;
	krad_cam->encoding_height = krad_cam->capture_height;
	krad_cam->display_width = krad_cam->capture_width;
	krad_cam->display_height = krad_cam->capture_height;

	krad_cam->vpx_bitrate = 1250;
	
	strncpy(krad_cam->device, DEFAULT_V4L2_DEVICE, sizeof(krad_cam->device));
	strncpy(krad_cam->alsa_capture_device, DEFAULT_ALSA_CAPTURE_DEVICE, sizeof(krad_cam->alsa_capture_device));
	strncpy(krad_cam->alsa_playback_device, DEFAULT_ALSA_PLAYBACK_DEVICE, sizeof(krad_cam->alsa_playback_device));
	sprintf(krad_cam->output, "%s/Videos/krad_cam_%zu.webm", getenv ("HOME"), time(NULL));

	krad_cam->krad_audio_api = ALSA;
	krad_cam->operation_mode = CAPTURE;
	krad_cam->interface_mode = WINDOW;
	krad_cam->video_codec = VP8;
	krad_cam->audio_codec = VORBIS;
	krad_cam->video_source = V4L2;
	
	return krad_cam;
}

void krad_cam_activate(krad_cam_t *krad_cam) {


	krad_cam->composite_fps = krad_cam->capture_fps;
	krad_cam->encoding_fps = krad_cam->capture_fps;
	krad_cam->composite_width = krad_cam->capture_width;
	krad_cam->composite_height = krad_cam->capture_height;
	krad_cam->encoding_width = krad_cam->capture_width;
	krad_cam->encoding_height = krad_cam->capture_height;
	krad_cam->display_width = krad_cam->capture_width;
	krad_cam->display_height = krad_cam->capture_height;

	signal(SIGINT, krad_cam_shutdown);
	signal(SIGTERM, krad_cam_shutdown);

	if (krad_cam->daemon) {
		daemonize();
	}

	if (krad_cam->interface_mode == WINDOW) {

		krad_cam->krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, krad_cam->display_width, krad_cam->display_height, 
															 		   krad_cam->composite_width, krad_cam->composite_height);
	}
	
	krad_cam->composited_frame_byte_size = krad_cam->composite_width * krad_cam->composite_height * 4;
	krad_cam->current_frame = calloc(1, krad_cam->composited_frame_byte_size);
	krad_cam->current_encoding_frame = calloc(1, krad_cam->composited_frame_byte_size);


	if (krad_cam->video_source == V4L2) {
		krad_cam->captured_frame_converter = sws_getContext ( krad_cam->capture_width, krad_cam->capture_height, PIX_FMT_YUYV422, 
															  krad_cam->composite_width, krad_cam->composite_height, PIX_FMT_RGB32, 
															  SWS_BICUBIC, NULL, NULL, NULL);
	}
		  
	if (krad_cam->video_source != NOVIDEO) {
		krad_cam->encoding_frame_converter = sws_getContext ( krad_cam->composite_width, krad_cam->composite_height, PIX_FMT_RGB32, 
															  krad_cam->encoding_width, krad_cam->encoding_height, PIX_FMT_YUV420P, 
															  SWS_BICUBIC, NULL, NULL, NULL);
	}
	
	if (krad_cam->interface_mode == WINDOW) {
															
		krad_cam->display_frame_converter = sws_getContext ( krad_cam->composite_width, krad_cam->composite_height, PIX_FMT_RGB32, 
															 krad_cam->display_width, krad_cam->display_height, PIX_FMT_RGB32, 
															 SWS_BICUBIC, NULL, NULL, NULL);
	}
		
	krad_cam->captured_frames_buffer = krad_ringbuffer_create (krad_cam->composited_frame_byte_size * krad_cam->capture_buffer_frames);
	krad_cam->composited_frames_buffer = krad_ringbuffer_create (krad_cam->composited_frame_byte_size * krad_cam->encoding_buffer_frames);
	krad_cam->decoded_frames_buffer = krad_ringbuffer_create (krad_cam->composited_frame_byte_size * krad_cam->encoding_buffer_frames);
	krad_cam->encoded_audio_ringbuffer = krad_ringbuffer_create (2000000);
	krad_cam->encoded_video_ringbuffer = krad_ringbuffer_create (2000000);

	krad_cam->krad_gui = kradgui_create_with_internal_surface(krad_cam->composite_width, krad_cam->composite_height);

	if (krad_cam->bug[0] != '\0') {
		kradgui_set_bug (krad_cam->krad_gui, krad_cam->bug, krad_cam->bug_x, krad_cam->bug_y);
	}

	if (krad_cam->verbose) {	
		krad_cam->krad_gui->update_drawtime = 1;
		krad_cam->krad_gui->print_drawtime = 1;
	}


	if (krad_cam->operation_mode == CAPTURE) {

		if (krad_cam->video_source == V4L2) {
			krad_cam->krad_gui->clear = 0;
		}
	
		if (krad_cam->video_source == TEST) {
			kradgui_test_screen(krad_cam->krad_gui, "Krad Test");
		}
	
		if (krad_cam->verbose) {
			printf("mem use approx %d MB\n", (krad_cam->composited_frame_byte_size * (krad_cam->capture_buffer_frames + krad_cam->encoding_buffer_frames + 4)) / 1000 / 1000);
		}
		
		if (krad_cam->video_source != NOVIDEO) {
		
			krad_cam->capturing = 1;
		
			pthread_create(&krad_cam->video_capture_thread, NULL, video_capture_thread, (void *)krad_cam);
		}
	
		if (krad_cam->interface_mode == WINDOW) {
		
			//if ((!krad_cam->capturing) && (!krad_cam->display)) {
				krad_cam->render_meters = 1;
			//}
	
		}
	
		if (krad_cam->video_source != NOVIDEO) {
			krad_cam->encoding = 1;
		
			pthread_create(&krad_cam->video_encoding_thread, NULL, video_encoding_thread, (void *)krad_cam);
		}
	
		if (krad_cam->video_source != NOVIDEO) {
			pthread_create(&krad_cam->audio_encoding_thread, NULL, audio_encoding_thread, (void *)krad_cam);
		}
	
		pthread_create(&krad_cam->ebml_output_thread, NULL, ebml_output_thread, (void *)krad_cam);	

	}
	
	if (krad_cam->operation_mode == RECEIVE) {

		krad_cam->composited_frame_byte_size = krad_cam->composite_width * krad_cam->composite_height * 4;
		krad_cam->current_frame = calloc(1, krad_cam->composited_frame_byte_size);
		krad_cam->current_encoding_frame = calloc(1, krad_cam->composited_frame_byte_size);
		krad_cam->decoded_frames_buffer = krad_ringbuffer_create (krad_cam->composited_frame_byte_size * krad_cam->encoding_buffer_frames);
		
		pthread_create(&krad_cam->ebml_input_thread, NULL, ebml_input_thread, (void *)krad_cam);	
		
		//if (krad_cam->video) {
			pthread_create(&krad_cam->video_decoding_thread, NULL, video_decoding_thread, (void *)krad_cam);
		//}
	
		//if (krad_cam->audio) {
			pthread_create(&krad_cam->audio_decoding_thread, NULL, audio_decoding_thread, (void *)krad_cam);
		//}

	}

}




	




void help() {

	printf("help, coming soon!\n");
	exit (0);

}

int main ( int argc, char *argv[] ) {

	krad_cam_t *krad_cam;
	int o;
	int option_index;
	
	do_shutdown = 0;
	
	krad_cam = krad_cam_create ();

	krad_cam->shutdown = &do_shutdown;

	while (1) {

		static struct option long_options[] = {
			{"verbose",			no_argument, 0, 'v'},
			{"help",			no_argument, 0, 'h'},
	
			{"window",			no_argument, 0, WINDOW},
			{"nowindow",		no_argument, 0, COMMAND},
			{"daemon",			no_argument, 0, DAEMON},
			
			{"novideo",			no_argument, 0, NOVIDEO},
			{"noaudio",			no_argument, 0, NOAUDIO},
			
			{"testscreen",		no_argument, 0, 't'},
			
			{"width",			required_argument, 0, 'w'},
			{"height",			required_argument, 0, 'h'},
			{"fps",				required_argument, 0, 'f'},
			{"mjpeg",			no_argument, 0, 'm'},
			{"yuv",				no_argument, 0, 'y'},

			{"alsa",			optional_argument, 0, ALSA},
			{"jack",			optional_argument, 0, JACK},
			{"pulse",			optional_argument, 0, PULSE},
			{"tone",			optional_argument, 0, TONE},
				
			{"flac",			optional_argument, 0, FLAC},
			{"opus",			optional_argument, 0, OPUS},
			{"vorbis",			optional_argument, 0, VORBIS},
						
			{"bitrate",			no_argument, 0, 'b'},
			{"keyframes",		no_argument, 0, 'k'},
			
			{"password",		no_argument, 0, 'p'},
			
			{0, 0, 0, 0}
		};

		option_index = 0;

		o = getopt_long ( argc, argv, "vhpbwh", long_options, &option_index );

		if (o == -1) {
			break;
		}

		switch ( o ) {
			case 'v':
				krad_cam->verbose = 1;
				break;
			case 'p':
				strncpy(krad_cam->password, optarg, sizeof(krad_cam->password));
				krad_cam->operation_mode = CAPTURE;
				break;
			case 'w':
				krad_cam->capture_width = atoi(optarg);
				break;
			case 'h':
				if (optarg != NULL) {
					krad_cam->capture_height = atoi(optarg);
					break;
				} else {
					help ();
				}
		//memcpy(krad_cam->bug, bug, sizeof(krad_cam->bug));
		//krad_cam->bug_x = bug_x;
		//krad_cam->bug_y = bug_y;
			case 'b':
				krad_cam->vpx_bitrate = atoi(optarg);
				break;
			case 'k':
				//
				break;
			case NOVIDEO:
				krad_cam->video_source = NOVIDEO;
				break;
			case FLAC:
				krad_cam->audio_codec = FLAC;
				break;
			case OPUS:
				krad_cam->audio_codec = OPUS;
				break;
			case VORBIS:
				krad_cam->audio_codec = VORBIS;
				break;
			case WINDOW:
				krad_cam->interface_mode = WINDOW;
				break;
			case NOWINDOW:
				krad_cam->interface_mode = NOWINDOW;
				break;
			case DAEMON:
				krad_cam->interface_mode = DAEMON;
				break;
			case NOAUDIO:
				krad_cam->krad_audio_api = NOAUDIO;
				break;
			case TONE:
				krad_cam->krad_audio_api = TONE;
				break;
			case PULSE:
				krad_cam->krad_audio_api = PULSE;
				break;
			case ALSA:
				krad_cam->krad_audio_api = ALSA;
				break;
			case JACK:
				krad_cam->krad_audio_api = JACK;
				break;
			case HELP:
				help ();
				break;

			default:
				abort ();
		}
	}

	if (optind < argc) {
		printf ("non-option ARGV-elements: ");
		while (optind < argc) {
			printf ("%s ", argv[optind]);
			putchar ('\n');
			
			
			if (argv[optind][0] == '/')  {		

				if (strncmp("/dev", argv[optind], 4) == 0) {
					memcpy(krad_cam->device, argv[optind], sizeof(krad_cam->device));
					krad_cam->operation_mode = CAPTURE;
				} else {
				
					if (stat(argv[optind], &krad_cam->file_stat) != 0) {
						strncpy(krad_cam->output, argv[optind], sizeof(krad_cam->output));
						krad_cam->operation_mode = CAPTURE;
					} else {
						krad_cam->operation_mode = RECEIVE;
						strncpy(krad_cam->input, argv[optind], sizeof(krad_cam->input));
					}				
				}

			} else {
			
				krad_cam->port = 8080;
				memcpy(krad_cam->host, "deimos.kradradio.com", sizeof(krad_cam->host));
				memcpy(krad_cam->mount, "/teststream.webm", sizeof(krad_cam->mount));
				//memcpy(krad_cam->host, host, sizeof(krad_cam->host));
				//memcpy(krad_cam->mount, mount, sizeof(krad_cam->mount));
			
			}
			
			optind++;
		}
	}

	krad_cam_run ( krad_cam );

	return 0;

}

