#include "krad_sdl_opengl_display.h"
#include "krad_ebml.h"
#include "krad_dirac.h"
#include "krad_vpx.h"
#include "krad_vorbis.h"
#include "krad_flac.h"
#include "krad_gui.h"
#include "krad_audio.h"
#include "krad_v4l2.h"

//#include "kradtimer.h"

#define DEFAULT_DEVICE "/dev/video0"
#define APPVERSION "Krad V4L2 VPX EBML Display TEST 0.4"
#define TEST_COUNT 100

typedef struct krad_v4l2_vpx_display_test_St krad_v4l2_vpx_display_test_t;

struct krad_v4l2_vpx_display_test_St {
//	kradtimer_t *kradtimer;
	krad_audio_t *audio;
	kradgui_t *kradgui;
	krad_vorbis_t *krad_vorbis;
	krad_flac_t *krad_flac;
	krad_ebml_t *ebml;
	float *samples[2];
	
	jack_ringbuffer_t *input_ringbuffer[2];
	
	pthread_rwlock_t ebml_write_lock;
	
	pthread_t audio_encoding_thread;
	int stop_audio_encoding;
	int audiotrack;

	int start_audio;

	int first_block;

	int audio_codec;
};


void *audio_encoding_thread(void *arg) {

	krad_v4l2_vpx_display_test_t *display_test = (krad_v4l2_vpx_display_test_t *)arg;
	
	int bytes;
	float *audio = calloc(1, 1000000);
	unsigned char *buffer = calloc(1, 1000000);
	ogg_packet *op;

	int framecnt;

	if (display_test->audio_codec == 1) {
		framecnt = 1470;
	}
	
	if (display_test->audio_codec == 2) {
		framecnt = 4096;
	}	
	
	while (!(display_test->stop_audio_encoding)) {

		while (jack_ringbuffer_read_space(display_test->input_ringbuffer[1]) > framecnt * 4) {	
	
	
			if (display_test->audio_codec == 1) {
				op = krad_vorbis_encode(display_test->krad_vorbis, framecnt, display_test->input_ringbuffer[0], display_test->input_ringbuffer[1]);
	
				if (op != NULL) {
	
					printf("bytes is %ld\n", op->bytes);
				
					if (display_test->first_block == 0) {
						pthread_rwlock_rdlock(&display_test->ebml_write_lock);
					} else {
						display_test->first_block = 0;
					}
				
					kradebml_add_audio(display_test->ebml, display_test->audiotrack, op->packet, op->bytes, framecnt);
					//kradebml_write(display_test->ebml);		
					pthread_rwlock_unlock (&display_test->ebml_write_lock);		
				}
			}
			
			if (display_test->audio_codec == 2) {
			
					jack_ringbuffer_read(display_test->input_ringbuffer[1], (char *)audio, 4096 * 4);
			
					bytes = krad_flac_encode(display_test->krad_flac, audio, 4096, buffer);

					printf("bytes is %d\n", bytes);
				
					if (display_test->first_block == 0) {
						pthread_rwlock_rdlock(&display_test->ebml_write_lock);
					} else {
						display_test->first_block = 0;
					}
					
					kradebml_add_audio(display_test->ebml, display_test->audiotrack, buffer, bytes, 4096);
					//kradebml_write(display_test->ebml);		
					pthread_rwlock_unlock (&display_test->ebml_write_lock);	
					
					
			}
			
		}
	
		while (jack_ringbuffer_read_space(display_test->input_ringbuffer[1]) < framecnt * 4) {
	
			usleep(10000);
	
			if (display_test->stop_audio_encoding) {
				break;
			}
	
		}
	}
	free(audio);
	free(buffer);
	return NULL;
	
}


void krad_v4l2_vpx_display_test_audio_callback(int frames, void *userdata) {

	krad_v4l2_vpx_display_test_t *display_test = (krad_v4l2_vpx_display_test_t *)userdata;
	
	kradaudio_read (display_test->audio, 0, (char *)display_test->samples[0], frames * 4 );
	kradaudio_read (display_test->audio, 1, (char *)display_test->samples[1], frames * 4 );

	if (display_test->start_audio == 1) {
		jack_ringbuffer_write(display_test->input_ringbuffer[0], (char *)display_test->samples[0], frames * 4);
		jack_ringbuffer_write(display_test->input_ringbuffer[1], (char *)display_test->samples[1], frames * 4);
	}
}


void rgb_to_yv12(struct SwsContext *sws_context, unsigned char *pRGBData, int src_w, int src_h, unsigned char *dst[4], int dst_stride_arr[4]) {

	int rgb_stride_arr[3] = {4*src_w, 0, 0};
	const uint8_t *rgb_arr[4];
	
	rgb_arr[0] = pRGBData;

    sws_scale(sws_context, rgb_arr, rgb_stride_arr, 0, src_h, dst, dst_stride_arr);
}


int main (int argc, char *argv[]) {

	krad_v4l2_vpx_display_test_t *display_test;
	krad_flac_t *krad_flac;
	krad_dirac_t *krad_dirac;
	krad_vpx_encoder_t *krad_vpx_encoder;
	//krad_vpx_decoder_t *krad_vpx_decoder;
	kradgui_t *kradgui;
	krad_ebml_t *ebml;
	cairo_surface_t *cst;
	cairo_t *cr;
	char *filename = "/home/oneman/kode/new_testfile4.webm";
	int hud_width, hud_height;
	int hud_stride;
	int hud_byte_size;
	unsigned char *hud_data;
	struct SwsContext *sws_context;

	int videotrack;
	int audiotrack;
	char *device;
	krad_v4l2_t *kradv4l2;
	krad_sdl_opengl_display_t *krad_opengl_display;
	int width;
	int height;
	int count;
	//int first_frame;
	int read_composited;
	
	unsigned char *read_screen_buffer;
	unsigned char *dbuffer;
	unsigned char *dfbuffer;
	krad_audio_t *audio;
	krad_audio_api_t audio_api;
	krad_vorbis_t *krad_vorbis;
	
	hud_width = 320;
	hud_height = 240;

	read_composited = 1;
	//first_frame = 1;
	
	width = 1280;
	height = 720;
	count = 0;
	int keyframe;
	void *frame = NULL;
	void *vpx_packet;
	int packet_size;
	int took;
	int video_codec;
	int audio_codec;
	int framenum;
	int bytes;
	
	video_codec = 2;
	audio_codec = 2;
		
	if (argc < 2) {
		device = DEFAULT_DEVICE;
		printf("no device provided, using %s , to provide a device, example: krad_v4l2_test /dev/video0\n", device);
	} else {
		device = argv[1];
		printf("Using %s\n", device);
	}

	display_test = calloc(1, sizeof(krad_v4l2_vpx_display_test_t));
	pthread_rwlock_init(&display_test->ebml_write_lock, NULL);
	pthread_rwlock_rdlock(&display_test->ebml_write_lock);
	//display_test->kradtimer = kradtimer_create();
	display_test->input_ringbuffer[0] = jack_ringbuffer_create (RINGBUFFER_SIZE);
	display_test->input_ringbuffer[1] = jack_ringbuffer_create (RINGBUFFER_SIZE);
	display_test->samples[0] = malloc(4 * 8192);
	display_test->samples[1] = malloc(4 * 8192);
	display_test->first_block = 1;
	display_test->audio_codec = audio_codec;
	
	dbuffer = calloc(1, 2000000);
	dfbuffer = calloc(1, 2000000);
	audio_api = JACK;
	//audio = kradaudio_create("krad v4l2 vpx display test", audio_api);
	krad_vorbis = krad_vorbis_encoder_create(2, 44100, 0.7);
	krad_flac = krad_flac_encoder_create(1, 44100, 16);
	
	read_screen_buffer = calloc(1, width * height * 4 * 4 * 4 * 4);
	sws_context = sws_getContext ( width, height, PIX_FMT_RGB32, width, height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	ebml = kradebml_create();
	//kradebml_open_output_stream(ebml, "192.168.1.2", 9080, "/teststream.webm", "secretkode");
	kradebml_open_output_file(ebml, filename);
	if (video_codec == 1) {
		kradebml_header(ebml, "webm", APPVERSION);
	} else {
		kradebml_header(ebml, "matroska", APPVERSION);
	}
	if (video_codec == 1) {
		videotrack = kradebml_add_video_track(ebml, "V_VP8", 10, width, height);
	}
	if (video_codec == 2) {
		videotrack = kradebml_add_video_track(ebml, "V_DIRAC", 10, width, height);
	}
	if (audio_codec == 1) {
		audiotrack = kradebml_add_audio_track(ebml, "A_VORBIS", 44100, 2, krad_vorbis->header, krad_vorbis->headerpos);
	}
	if (audio_codec == 2) {
		bytes = krad_flac_encoder_read_min_header(krad_flac, dfbuffer);
		audiotrack = kradebml_add_audio_track(ebml, "A_FLAC", 44100, 1, dfbuffer, bytes);
		display_test->krad_flac = krad_flac;
	}	
	kradebml_write(ebml);

	krad_opengl_display = krad_sdl_opengl_display_create(width, height, width, height);
	//krad_opengl_display = krad_sdl_opengl_display_create(1920, 1080, width, height);
	
	krad_vpx_encoder = krad_vpx_encoder_create(width, height);
	krad_dirac = krad_dirac_encoder_create(width, height);
	kradgui = kradgui_create(width, height);
	kradgui_add_item(kradgui, REEL_TO_REEL);
	
	hud_stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, hud_width);
	hud_byte_size = hud_stride * hud_height;
	hud_data = calloc (1, hud_byte_size);
	cst = cairo_image_surface_create_for_data (hud_data, CAIRO_FORMAT_ARGB32, hud_width, hud_height, hud_stride);
	
	krad_opengl_display->hud_width = hud_width;
	krad_opengl_display->hud_height = hud_height;
	krad_opengl_display->hud_data = hud_data;
	display_test->audiotrack = audiotrack;
	display_test->ebml = ebml;
	//display_test->audio = audio;
	display_test->krad_vorbis = krad_vorbis;
	display_test->kradgui = kradgui;

	kradv4l2 = kradv4l2_create();

	kradv4l2_open(kradv4l2, device, width, height);
	
	kradv4l2_start_capturing (kradv4l2);
	
	kradgui_reset_elapsed_time(kradgui);
	kradgui->render_timecode = 1;
	
	audio = kradaudio_create("krad v4l2 vpx display test", audio_api);
	display_test->audio = audio;
	pthread_create(&display_test->audio_encoding_thread, NULL, audio_encoding_thread, (void *)display_test);
	
	kradaudio_set_process_callback(audio, krad_v4l2_vpx_display_test_audio_callback, display_test);
	if (audio_api == JACK) {
		krad_jack_t *jack = (krad_jack_t *)audio->api;
		kradjack_connect_port(jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "krad v4l2 vpx display test:InputLeft");
		kradjack_connect_port(jack->jack_client, "firewire_pcm:001486af2e61ac6b_Unknown0_in", "krad v4l2 vpx display test:InputRight");
	}
	while (count < TEST_COUNT) {
		//kradtimer_start(display_test->kradtimer, "cycle");


		cr = cairo_create(cst);
		kradgui->cr = cr;
		kradgui_render(kradgui);
		cairo_destroy(cr);
		
		frame = kradv4l2_read_frame_wait (kradv4l2);
		
		if (display_test->start_audio == 0) {
			display_test->start_audio = 1;
			usleep(200000);
		}
		
		if (video_codec == 2) {
		//	memcpy(dfbuffer, frame, width * height + (((width * height) / 2) * 2));
		}
		krad_vpx_convert_uyvy2yv12(krad_vpx_encoder->image, frame, width, height);
		///krad_vpx_convert_frame_for_local_gl_display(krad_vpx_encoder);
		kradv4l2_frame_done (kradv4l2);
		
		
		
		krad_sdl_opengl_display_render(krad_opengl_display, krad_vpx_encoder->image->planes[0], krad_vpx_encoder->image->stride[0], krad_vpx_encoder->image->planes[1], krad_vpx_encoder->image->stride[1], krad_vpx_encoder->image->planes[2], krad_vpx_encoder->image->stride[2]);

		krad_sdl_opengl_draw_screen( krad_opengl_display );
		
		if (read_composited) {
			krad_sdl_opengl_read_screen( krad_opengl_display, read_screen_buffer);
			rgb_to_yv12(sws_context, read_screen_buffer, width, height, krad_vpx_encoder->image->planes, krad_vpx_encoder->image->stride);
			vpx_img_flip(krad_vpx_encoder->image);
		}
		
		if (video_codec == 1) {
		
			count++;
		
			packet_size = krad_vpx_encoder_write(krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);

			//printf("packet size was %d\n", packet_size);
			if (read_composited) {
				vpx_img_flip(krad_vpx_encoder->image);
			}
		
			if (packet_size) {
				pthread_rwlock_wrlock(&display_test->ebml_write_lock);
				kradebml_add_video(ebml, videotrack, vpx_packet, packet_size, keyframe);
				//kradebml_write(ebml);
				pthread_rwlock_unlock (&display_test->ebml_write_lock);		
			}


		}
		
		
		if (video_codec == 2) {

//			packet_size = krad_dirac_encode (krad_dirac, dfbuffer, dbuffer, &framenum, &took);
			packet_size = krad_dirac_encode (krad_dirac, krad_vpx_encoder->image->img_data, dbuffer, &framenum, &took);
			if (took == 1) {
				took = 0;
				count++;
			}
	
			if (packet_size > 0) {
				krad_dirac_packet_type(dbuffer[4]);
				//write(fd, buffer, len);
				printf("Encoded size is %d for frame %d\n", packet_size, framenum);
			}
		
			//printf("packet size was %d\n", packet_size);
			if (read_composited) {
				vpx_img_flip(krad_vpx_encoder->image);
			}
		
			keyframe = 0;
		
			if (packet_size) {
				pthread_rwlock_wrlock(&display_test->ebml_write_lock);
				kradebml_add_video(ebml, videotrack, dbuffer, packet_size, keyframe);
				//kradebml_write(ebml);
				pthread_rwlock_unlock (&display_test->ebml_write_lock);		
			}
		
		}
		
	//kradtimer_finish_show(display_test->kradtimer);
	}
	
	display_test->start_audio = 0;
	
	if (video_codec == 2) {
		
		schro_encoder_end_of_stream (krad_dirac->encoder);
//			packet_size = krad_dirac_encode (krad_dirac, dfbuffer, dbuffer, &framenum, &took);

		while ((framenum != count - 1) && (framenum != 0)) {

			packet_size = krad_dirac_encode (krad_dirac, krad_vpx_encoder->image->img_data, dbuffer, &framenum, &took);

			if (packet_size > 0) {
				krad_dirac_packet_type(dbuffer[4]);
				//write(fd, buffer, len);
				printf("Encoded size is %d for frame %d\n", packet_size, framenum);
			} else {
				usleep(50000);
			}
	
			//printf("packet size was %d\n", packet_size);
			if (read_composited) {
				//vpx_img_flip(krad_vpx_encoder->image);
			}
	
			keyframe = 0;
	
			if (packet_size) {
				pthread_rwlock_wrlock(&display_test->ebml_write_lock);
				kradebml_add_video(ebml, videotrack, dbuffer, packet_size, keyframe);
				//kradebml_write(ebml);
				pthread_rwlock_unlock (&display_test->ebml_write_lock);		
			}
	
		}
	
	}
	

	//display_test->start_audio = 0;
	printf("finish audio encoding\n");
	usleep(2000000);
	display_test->stop_audio_encoding = 1;
	
	
	pthread_join(display_test->audio_encoding_thread, NULL);
	
	kradebml_destroy(ebml);

	kradv4l2_stop_capturing (kradv4l2);

	kradv4l2_close(kradv4l2);

	kradv4l2_destroy(kradv4l2);
	krad_dirac_encoder_destroy(krad_dirac);
	krad_vpx_encoder_destroy(krad_vpx_encoder);
	
	// must be before vorbis
	kradaudio_destroy(audio);
	
	krad_vorbis_encoder_destroy(krad_vorbis);
	krad_flac_encoder_destroy(krad_flac);
	free(display_test->samples[0]);
	free(display_test->samples[1]);
	
	krad_sdl_opengl_display_destroy(krad_opengl_display);
	kradgui_destroy(kradgui);
	
	free(read_screen_buffer);
	sws_freeContext (sws_context);
	
	jack_ringbuffer_free ( display_test->input_ringbuffer[0] );
	jack_ringbuffer_free ( display_test->input_ringbuffer[1] );
	pthread_rwlock_destroy(&display_test->ebml_write_lock);
	//kradtimer_destroy(display_test->kradtimer);
	free(display_test);
	free(dbuffer);
	free(dfbuffer);
	return 0;

}



