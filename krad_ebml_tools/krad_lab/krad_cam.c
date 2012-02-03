#include "krad_sdl_opengl_display.h"
#include "krad_ebml.h"
#include "krad_dirac.h"
#include "krad_vpx.h"
#include "krad_vorbis.h"
#include "krad_flac.h"
#include "krad_gui.h"
#include "krad_audio.h"
#include "krad_v4l2.h"

#define DEFAULT_DEVICE "/dev/video0"
#define APPVERSION "Krad Cam 0.1"
#define TEST_COUNT 300

typedef struct krad_cam_St krad_cam_t;

struct krad_cam_St {

	kradgui_t *krad_gui;
	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_audio_t *krad_audio;
	krad_audio_api_t audio_api;
	krad_vorbis_t *krad_vorbis;
	kradebml_t *krad_ebml;
	krad_v4l2_t *krad_v4l2;
	krad_sdl_opengl_display_t *krad_opengl_display;
	
	char device[512];
	char output[512];
	
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

	int capture_buffer_frames;
	int encoding_buffer_frames;

	unsigned char *current_encoding_frame;
	unsigned char *current_frame;
	
	unsigned int captured_frames;
	
	int encoding;
	int capturing;
	
	int start_audio;
	int stop_audio;
	jack_ringbuffer_t *audio_input_ringbuffer[2];
	float *samples[2];
	int audio_encoder_ready;
	
	int composited_frame_byte_size;
	jack_ringbuffer_t *captured_frames_buffer;
	jack_ringbuffer_t *composited_frames_buffer;
	
	struct SwsContext *captured_frame_converter;
	struct SwsContext *encoding_frame_converter;
	struct SwsContext *display_frame_converter;
	
	int video_track;
	int audio_track;
	
	krad_audio_api_t krad_audio_api;
	
	pthread_t video_capture_thread;
	pthread_t video_encoding_thread;
	pthread_t audio_encoding_thread;
	pthread_t ebml_output_thread;
	
	pthread_rwlock_t ebml_write_lock;

};

void *video_capture_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	int dupe_frame = 0;
	int first = 1;
	unsigned long long expected_time_ns;
	unsigned long long expected_time_ms;
	struct timespec start_time;
	struct timespec current_time;
	struct timespec elapsed_time;
	struct timespec expected_time;
	struct timespec diff_time;
	void *captured_frame = NULL;
	unsigned char *captured_frame_rgb = malloc(krad_cam->composited_frame_byte_size); 
	
	printf("\n\ncapture thread begins\n\n");

	krad_cam->krad_v4l2 = kradv4l2_create();

	kradv4l2_open(krad_cam->krad_v4l2, krad_cam->device, krad_cam->capture_width, krad_cam->capture_height, krad_cam->capture_fps);
	
	kradv4l2_start_capturing (krad_cam->krad_v4l2);

	while (krad_cam->capturing == 1) {

		captured_frame = kradv4l2_read_frame_wait_adv (krad_cam->krad_v4l2);
	
		
		if (first == 1) {
			first = 0;
			krad_cam->captured_frames = 0;
			krad_cam->start_audio = 1;
			clock_gettime(CLOCK_MONOTONIC, &start_time);
		} else {
		
		
			if (krad_cam->captured_frames % 300 == 0) {
		
				clock_gettime(CLOCK_MONOTONIC, &current_time);
				elapsed_time = timespec_diff(start_time, current_time);
				printf("Frames Captured: %u Expected: %ld - %u fps * %ld seconds\n", krad_cam->captured_frames, elapsed_time.tv_sec * krad_cam->capture_fps, krad_cam->capture_fps, elapsed_time.tv_sec);
	
			}
			
			krad_cam->captured_frames++;
	
		}
	
		/*	
			elapsed_time = timespec_diff(start_time, current_time);
	
			expected_time_ns = (1000000000 / krad_cam->capture_fps) * (unsigned long long)frames;
			expected_time_ms = expected_time_ns / 1000000;
			expected_time.tv_sec = (expected_time_ms - (expected_time_ms % 1000)) / 1000;
			expected_time.tv_nsec = (expected_time_ms % 1000) * 1000000;
	
			diff_time = timespec_diff(expected_time, elapsed_time);
	
			if (diff_time.tv_sec == 0) {
	
				if (diff_time.tv_nsec > ((unsigned long long)1000000000 / (unsigned long long)krad_cam->capture_fps)) {
					first = 1;
					dupe_frame = 1;
		
				} else {
					dupe_frame = 0;
				}
				printf("\n\ndiff time %zu %zu \n\n", diff_time.tv_sec, diff_time.tv_nsec);
			}
			
				//printf("\n\nelapsed time %zu %zu \n", elapsed_time.tv_sec, elapsed_time.tv_nsec);
				//printf("\n\nexpected_time_ms %llu \n", expected_time_ms);
				//printf("\n\nexpected time %zu %zu \n", expected_time.tv_sec, expected_time.tv_nsec);	
	
		}
		
		*/
	
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
	
		kradv4l2_frame_done (krad_cam->krad_v4l2);

	
		if (jack_ringbuffer_write_space(krad_cam->captured_frames_buffer) >= krad_cam->composited_frame_byte_size) {
	
			jack_ringbuffer_write(krad_cam->captured_frames_buffer, 
								 (char *)captured_frame_rgb, 
								 krad_cam->composited_frame_byte_size );
	
		}
		
		if (dupe_frame == 1) {

			printf("\n\nduping a frame\n\n");

			if (jack_ringbuffer_write_space(krad_cam->captured_frames_buffer) >= krad_cam->composited_frame_byte_size) {

				jack_ringbuffer_write(krad_cam->captured_frames_buffer, 
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

	printf("\n\ncapture thread ends\n\n");

	return NULL;
	
}

void *video_encoding_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;
	
	printf("\n\nencoding thread begins\n\n");
	
	void *vpx_packet;
	int keyframe;
	int packet_size;

	int first = 1;

	krad_cam->krad_vpx_encoder = krad_vpx_encoder_create(krad_cam->encoding_width, krad_cam->encoding_height);


	krad_cam->krad_vpx_encoder->quality = 1000000 / krad_cam->encoding_fps;


	while ((krad_cam->encoding == 1) || (jack_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= krad_cam->composited_frame_byte_size)) {

		if (jack_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= krad_cam->composited_frame_byte_size) {
	
			jack_ringbuffer_read(krad_cam->composited_frames_buffer, 
								 (char *)krad_cam->current_encoding_frame, 
								 krad_cam->composited_frame_byte_size );
								 
						
						
			if (jack_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= (krad_cam->composited_frame_byte_size * (krad_cam->encoding_buffer_frames / 2)) ) {
			
				krad_cam->krad_vpx_encoder->quality = (1000000 / krad_cam->encoding_fps) / 2LU;
			
			}
						
			if (jack_ringbuffer_read_space(krad_cam->composited_frames_buffer) >= (krad_cam->composited_frame_byte_size * (krad_cam->encoding_buffer_frames / 4)) ) {
			
				krad_cam->krad_vpx_encoder->quality = (1000000 / krad_cam->encoding_fps) / 4LU;
			
			}						 
								 
			int rgb_stride_arr[3] = {4*krad_cam->composite_width, 0, 0};
			const uint8_t *rgb_arr[4];
	
			rgb_arr[0] = krad_cam->current_encoding_frame;

			sws_scale(krad_cam->encoding_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_cam->composite_height, 
					  krad_cam->krad_vpx_encoder->image->planes, krad_cam->krad_vpx_encoder->image->stride);
										 
			packet_size = krad_vpx_encoder_write(krad_cam->krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
								 
			//printf("packet size was %d\n", packet_size);
	
			
	
			if (packet_size) {
				pthread_rwlock_wrlock(&krad_cam->ebml_write_lock);
				kradebml_add_video(krad_cam->krad_ebml, krad_cam->video_track, vpx_packet, packet_size, keyframe);
				pthread_rwlock_unlock(&krad_cam->ebml_write_lock);
				if (first == 1) {				
					first = 0;
				}
				
			}
	
		} else {
		
			krad_cam->krad_vpx_encoder->quality = 1000000 / krad_cam->encoding_fps;
		
			usleep(5000);
		
		}
		
	}
	
	while (packet_size) {
		krad_vpx_encoder_finish(krad_cam->krad_vpx_encoder);
		packet_size = krad_vpx_encoder_write(krad_cam->krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
	}

	krad_vpx_encoder_destroy(krad_cam->krad_vpx_encoder);
	
	krad_cam->encoding = 2;
	
	printf("\n\nencoding thread ends\n\n");
	
	return NULL;
	
}


void krad_cam_audio_callback(int frames, void *userdata) {

	krad_cam_t *krad_cam = (krad_cam_t *)userdata;
	
	kradaudio_read (krad_cam->krad_audio, 0, (char *)krad_cam->samples[0], frames * 4 );
	kradaudio_read (krad_cam->krad_audio, 1, (char *)krad_cam->samples[1], frames * 4 );

	if (krad_cam->start_audio == 1) {
		jack_ringbuffer_write(krad_cam->audio_input_ringbuffer[0], (char *)krad_cam->samples[0], frames * 4);
		jack_ringbuffer_write(krad_cam->audio_input_ringbuffer[1], (char *)krad_cam->samples[1], frames * 4);
	}
}

void *audio_encoding_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;
	
	krad_cam->krad_audio = kradaudio_create("Krad Cam", krad_cam->krad_audio_api);
	
	krad_cam->krad_vorbis = krad_vorbis_encoder_create(2, 44100, 0.7);
	
	int framecnt = 1470;
	int bytes;
	float *audio = calloc(1, 1000000);
	unsigned char *buffer = calloc(1, 1000000);
	ogg_packet *op;

	krad_cam->audio_input_ringbuffer[0] = jack_ringbuffer_create (1000000);
	krad_cam->audio_input_ringbuffer[1] = jack_ringbuffer_create (1000000);

	krad_cam->samples[0] = malloc(4 * 8192);
	krad_cam->samples[1] = malloc(4 * 8192);

	kradaudio_set_process_callback(krad_cam->krad_audio, krad_cam_audio_callback, krad_cam);

	if (krad_cam->audio_api == JACK) {
		krad_jack_t *jack = (krad_jack_t *)krad_cam->krad_audio->api;
		kradjack_connect_port(jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "Krad Cam:InputLeft");
		kradjack_connect_port(jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "Krad Cam:InputRight");
	}
	
	krad_cam->audio_encoder_ready = 1;
	
	while (!(krad_cam->stop_audio)) {

		while (jack_ringbuffer_read_space(krad_cam->audio_input_ringbuffer[1]) > framecnt * 4) {
			
			op = krad_vorbis_encode(krad_cam->krad_vorbis, framecnt, krad_cam->audio_input_ringbuffer[0], krad_cam->audio_input_ringbuffer[1]);

			if (op != NULL) {

				printf("bytes is %ld\n", op->bytes);
				pthread_rwlock_wrlock(&krad_cam->ebml_write_lock);
				kradebml_add_audio(krad_cam->krad_ebml, krad_cam->audio_track, op->packet, op->bytes, framecnt);
				pthread_rwlock_unlock(&krad_cam->ebml_write_lock);
			}
		}
	
		while (jack_ringbuffer_read_space(krad_cam->audio_input_ringbuffer[1]) < framecnt * 4) {
	
			usleep(10000);
	
			if (krad_cam->stop_audio) {
				break;
			}
	
		}
	}
	
	
	free(krad_cam->samples[0]);
	free(krad_cam->samples[1]);

	jack_ringbuffer_free ( krad_cam->audio_input_ringbuffer[0] );
	jack_ringbuffer_free ( krad_cam->audio_input_ringbuffer[1] );
	
	free(audio);
	free(buffer);
	return NULL;
	
}

void *ebml_output_thread(void *arg) {

	krad_cam_t *krad_cam = (krad_cam_t *)arg;

	printf("\n\nmuxing thread begins\n\n");

	krad_cam->krad_ebml = kradebml_create();
	
	//	kradebml_open_output_stream(krad_cam->krad_ebml, "192.168.1.2", 9080, "/teststream.webm", "secretkode");
	kradebml_open_output_file(krad_cam->krad_ebml, krad_cam->output);
	kradebml_header(krad_cam->krad_ebml, "webm", APPVERSION);
	
	krad_cam->video_track = kradebml_add_video_track(krad_cam->krad_ebml, "V_VP8", krad_cam->encoding_fps,
										 			 krad_cam->encoding_width, krad_cam->encoding_height);
	
	
	while ( krad_cam->audio_encoder_ready != 1) {
	
		usleep(10000);
	
	}
	
	krad_cam->audio_track = kradebml_add_audio_track(krad_cam->krad_ebml, "A_VORBIS", 44100, 2, krad_cam->krad_vorbis->header, 
													 krad_cam->krad_vorbis->headerpos);
	
	kradebml_write(krad_cam->krad_ebml);
	
	pthread_rwlock_init(&krad_cam->ebml_write_lock, NULL);
	
	printf("\n\nebml thread waiting..\n\n");
	
	while ( krad_cam->encoding != 2) {
	
		usleep(100000);
	
	}
	pthread_rwlock_wrlock(&krad_cam->ebml_write_lock);
	kradebml_destroy(krad_cam->krad_ebml);
	pthread_rwlock_unlock(&krad_cam->ebml_write_lock);
	pthread_rwlock_destroy(&krad_cam->ebml_write_lock);
	
	printf("\n\nmuxing thread ends\n\n");
	
	return NULL;
	
}


void krad_cam_destroy(krad_cam_t *krad_cam) {

	krad_cam->capturing = 0;
	krad_cam->encoding = 0;
	pthread_join(krad_cam->video_capture_thread, NULL);
	pthread_join(krad_cam->video_encoding_thread, NULL);
	pthread_join(krad_cam->ebml_output_thread, NULL);
	
	krad_sdl_opengl_display_destroy(krad_cam->krad_opengl_display);

	kradgui_destroy(krad_cam->krad_gui);

	sws_freeContext ( krad_cam->captured_frame_converter );
	sws_freeContext ( krad_cam->encoding_frame_converter );
	sws_freeContext ( krad_cam->display_frame_converter );
	
	// must be before vorbis
	kradaudio_destroy(krad_cam->krad_audio);
	krad_vorbis_encoder_destroy(krad_cam->krad_vorbis);
	
	jack_ringbuffer_free( krad_cam->captured_frames_buffer );

	free(krad_cam->current_encoding_frame);
	free(krad_cam->current_frame);

	free(krad_cam);
}


krad_cam_t *krad_cam_create(char *device, char *output, krad_audio_api_t audio_api, int capture_width, int capture_height, int capture_fps, int composite_width, 
							int composite_height, int composite_fps, int display_width, int display_height, int encoding_width, 
							int encoding_height, int encoding_fps) {

	krad_cam_t *krad_cam;
	
	krad_cam = calloc(1, sizeof(krad_cam_t));

	krad_cam->capture_width = capture_width;
	krad_cam->capture_height = capture_height;
	krad_cam->capture_fps = capture_fps;
	krad_cam->composite_width = composite_width;
	krad_cam->composite_height = composite_height;
	krad_cam->composite_fps = composite_fps;
	krad_cam->display_width = display_width;
	krad_cam->display_height = display_height;
	krad_cam->encoding_width = encoding_width;
	krad_cam->encoding_height = encoding_height;
	krad_cam->encoding_fps = encoding_fps;
	
	krad_cam->krad_audio_api = audio_api;
	
	krad_cam->capture_buffer_frames = 5;
	krad_cam->encoding_buffer_frames = 10;
	
	strncpy(krad_cam->device, device, sizeof(krad_cam->device));
	strncpy(krad_cam->output, output, sizeof(krad_cam->output)); 

	krad_cam->krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, krad_cam->display_width, krad_cam->display_height, 
														 		   krad_cam->composite_width, krad_cam->composite_height);


	krad_cam->composited_frame_byte_size = krad_cam->composite_width * krad_cam->composite_height * 4;
	krad_cam->current_frame = calloc(1, krad_cam->composited_frame_byte_size);
	krad_cam->current_encoding_frame = calloc(1, krad_cam->composited_frame_byte_size);

	krad_cam->captured_frame_converter = sws_getContext ( krad_cam->capture_width, krad_cam->capture_height, PIX_FMT_YUYV422, 
														  krad_cam->composite_width, krad_cam->composite_height, PIX_FMT_RGB32, 
														  SWS_BICUBIC, NULL, NULL, NULL);
														  
	krad_cam->encoding_frame_converter = sws_getContext ( krad_cam->composite_width, krad_cam->composite_height, PIX_FMT_RGB32, 
															krad_cam->encoding_width, krad_cam->encoding_height, PIX_FMT_YUV420P, 
															SWS_BICUBIC, NULL, NULL, NULL);
															
	krad_cam->display_frame_converter = sws_getContext ( krad_cam->composite_width, krad_cam->composite_height, PIX_FMT_RGB32, 
														 krad_cam->display_width, krad_cam->display_height, PIX_FMT_RGB32, 
														 SWS_BICUBIC, NULL, NULL, NULL);
	
	krad_cam->captured_frames_buffer = jack_ringbuffer_create (krad_cam->composited_frame_byte_size * krad_cam->capture_buffer_frames);
	krad_cam->composited_frames_buffer = jack_ringbuffer_create (krad_cam->composited_frame_byte_size * krad_cam->encoding_buffer_frames);

	krad_cam->krad_gui = kradgui_create_with_internal_surface(krad_cam->composite_width, krad_cam->composite_height);

	krad_cam->krad_gui->update_drawtime = 1;
	krad_cam->krad_gui->print_drawtime = 1;
	
	//krad_cam->krad_gui->render_ftest = 1;
	//krad_cam->krad_gui->render_tearbar = 1;
	krad_cam->krad_gui->clear = 0;
	//kradgui_test_screen(krad_cam->krad_gui, "Krad Cam Test");

	printf("mem use approx %d MB\n", (krad_cam->composited_frame_byte_size * (krad_cam->capture_buffer_frames + krad_cam->encoding_buffer_frames + 4)) / 1000 / 1000);

	krad_cam->encoding = 1;
	krad_cam->capturing = 1;

	pthread_create(&krad_cam->video_capture_thread, NULL, video_capture_thread, (void *)krad_cam);
	pthread_create(&krad_cam->video_encoding_thread, NULL, video_encoding_thread, (void *)krad_cam);
	pthread_create(&krad_cam->audio_encoding_thread, NULL, audio_encoding_thread, (void *)krad_cam);
	pthread_create(&krad_cam->ebml_output_thread, NULL, ebml_output_thread, (void *)krad_cam);	

	return krad_cam;

}


int main (int argc, char *argv[]) {

	krad_cam_t *krad_cam;
	
	int new_capture_frame;
	int cam_started;
	int read_composited;
	int frames;
	int shutdown;
	int capture_width, capture_height, capture_fps;
	int display_width, display_height;
	int composite_width, composite_height, composite_fps;
	int encoding_width, encoding_height, encoding_fps;
	char *device;
	krad_audio_api_t krad_audio_api;
	char output[512];
	
	SDL_Event event;

	cam_started = 0;
	read_composited = 0;	
	shutdown = 0;
	capture_width = 1280;
	capture_height = 720;
	capture_fps = 10;

	capture_width = 640;
	capture_height = 480;
	capture_fps = 30;

	composite_fps = capture_fps;
	encoding_fps = capture_fps;

	composite_width = capture_width;
	composite_height = capture_height;

	encoding_width = capture_width;
	encoding_height = capture_height;

	display_width = capture_width;
	display_height = capture_height;
	
	device = DEFAULT_DEVICE;
	krad_audio_api = PULSE;

	sprintf(output, "%s/kode/testmedia/capture/krad_cam_%zu.webm", getenv ("HOME"), time(NULL));

	krad_cam = krad_cam_create( device, output, PULSE, capture_width, capture_height, capture_fps, composite_width, composite_height, 
								composite_fps, display_width, display_height, encoding_width, encoding_height, encoding_fps );

	while (1) {

		if (jack_ringbuffer_read_space(krad_cam->captured_frames_buffer) >= krad_cam->composited_frame_byte_size) {
	
			jack_ringbuffer_read(krad_cam->captured_frames_buffer, 
								 (char *)krad_cam->current_frame, 
								 krad_cam->composited_frame_byte_size );

			if (cam_started == 0) {
				cam_started = 1;
				kradgui_go_live(krad_cam->krad_gui);
			}
			
			new_capture_frame = 1;
		} else {
			new_capture_frame = 0;
		}		
		
		memcpy( krad_cam->krad_gui->data, krad_cam->current_frame, krad_cam->krad_gui->bytes );
		
		kradgui_render( krad_cam->krad_gui );

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

		if (read_composited) {
			krad_sdl_opengl_read_screen( krad_cam->krad_opengl_display, krad_cam->krad_opengl_display->rgb_frame_data);
		}

		if ((cam_started == 1) && (new_capture_frame == 1) && (jack_ringbuffer_write_space(krad_cam->composited_frames_buffer) >= krad_cam->composited_frame_byte_size)) {
	
			jack_ringbuffer_write(krad_cam->composited_frames_buffer, 
								 (char *)krad_cam->krad_opengl_display->rgb_frame_data, 
								 krad_cam->composited_frame_byte_size );
	
		} else {
		
			if ((cam_started == 1) && (new_capture_frame == 1)) {
				printf("encoding to slow! overflow!\n");
			}
		
		}

		frames++;

		while ( SDL_PollEvent( &event ) ){
			switch( event.type ){
				/* Look for a keypress */
				case SDL_KEYDOWN:
					/* Check the SDLKey values and move change the coords */
					switch( event.key.keysym.sym ){
					    case SDLK_LEFT:
					        break;
					    case SDLK_RIGHT:
					        break;
					    case SDLK_UP:
					        break;
					    case SDLK_j:
					    	//render_hud = 0;
					        break;
					    case SDLK_h:
					    	//render_hud = 1;
					        break;
					    case SDLK_q:
					    	shutdown = 1;
					        break;
					    case SDLK_f:
					    	
					        break;
					    default:
					        break;
					}
					break;

				case SDL_KEYUP:
					switch( event.key.keysym.sym ) {
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
					krad_cam->krad_gui->cursor_x = event.motion.x;
					krad_cam->krad_gui->cursor_y = event.motion.y;
					break;	
			
				default:
					break;
			}
		}

		if (shutdown == 1) {
			break;
		}
		
	}

	krad_cam_destroy( krad_cam );

	
	return 0;

}

