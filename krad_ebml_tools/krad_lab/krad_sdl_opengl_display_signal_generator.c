#include <krad_sdl_opengl_display.h>
#include <krad_gui.h>
#include <krad_ebml.h>
#include <krad_dirac.h>
#include <krad_vpx.h>
#include <krad_vorbis.h>
#include <krad_flac.h>
#include <krad_opus.h>
#include <krad_tone.h>

#include "SDL.h"

#define APPVERSION "Krad Signal Generator"
#define TEST_COUNT 444444444

void rgb_to_yv12(struct SwsContext *sws_context, unsigned char *pRGBData, int src_w, int src_h, unsigned char *dst[4], int dst_stride_arr[4]) {

	int rgb_stride_arr[3] = {4*src_w, 0, 0};
	const uint8_t *rgb_arr[4];
	
	rgb_arr[0] = pRGBData;

    sws_scale(sws_context, rgb_arr, rgb_stride_arr, 0, src_h, dst, dst_stride_arr);
}


void switch_test(kradgui_t *kradgui) {

	if (kradgui->render_tearbar == 1) {
		kradgui->render_tearbar = 0;
		kradgui->render_wheel = 0;
		kradgui->render_rgb = 1;
		kradgui->render_test_text = 1;
		kradgui->render_rotator = 1;
		return;
	}

	if (kradgui->render_wheel == 1) {
		kradgui->render_tearbar = 1;
		kradgui->render_wheel = 0;
		kradgui->render_rgb = 0;
		kradgui->render_test_text = 0;
		kradgui->render_rotator = 0;
		return;
	}
	
	if (kradgui->render_rgb == 1) {
		kradgui->render_tearbar = 0;
		kradgui->render_wheel = 1;
		kradgui->render_rgb = 0;
		kradgui->render_test_text = 0;
		kradgui->render_rotator = 0;
		return;
	}



}

int main (int argc, char *argv[]) {

	krad_sdl_opengl_display_t *krad_sdl_opengl_display;
	kradgui_t *kradgui;
	krad_ebml_t *krad_ebml;
	krad_vorbis_t *krad_vorbis;
	krad_flac_t *krad_flac;
	krad_dirac_t *krad_dirac;
	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_tone_t *krad_tone;
	
	struct SwsContext *sws_context;
	
	struct timespec signal_time;
	struct timespec sleep_time;
	uint64_t signal_time_ms;
	
	int width, height;
	int fps;
	int count;
	int stride;
	int gui_byte_size;
	unsigned char *gui_data;
	int sleeped;
	
	int channels;
	int sample_rate;
	int bit_depth;
	
	char *filename;
	char *test_info;
	
	cairo_surface_t *cst;
	cairo_t *cr;
	
	unsigned char *audio_buffer;
	unsigned char *video_buffer;
	float *sample_buffer;
	
	int videotrack;
	int audiotrack;
	
	int samples_per_frame;
	
	int bytes;
	int keyframe;
	
	void *vpx_packet;
	
	test_info = "This is a test of the krad signal generator";
	filename = "/home/oneman/Videos/krad_signal3.webm";
	
	width = 1280;
	height = 720;
	fps = 30;
	
	sleeped = 0;
	channels = 1;
	sample_rate = 44100;
	bit_depth = 16;


	samples_per_frame = 1000 / fps * (sample_rate / 1000);

	printf("samples per frame is %d\n", samples_per_frame);
	//exit(0);

	count = 0;
	
	audio_buffer = calloc(1, 2000000);
	video_buffer = calloc(1, 2000000);
	sample_buffer = calloc(1, 2000000);
	
	sws_context = sws_getContext ( width, height, PIX_FMT_RGB32, width, height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	
	krad_tone = krad_tone_create(sample_rate);
	krad_tone_add_preset(krad_tone, "dialtone");
	
	stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);
	gui_byte_size = stride * height;
	gui_data = calloc (1, gui_byte_size);
	
	cst = cairo_image_surface_create_for_data (gui_data, CAIRO_FORMAT_ARGB32, width, height, stride);
	
	krad_sdl_opengl_display = krad_sdl_opengl_display_create(width, height, width, height);
	kradgui = kradgui_create(width, height);
	krad_flac = krad_flac_encoder_create(channels, sample_rate, bit_depth);
	krad_vpx_encoder = krad_vpx_encoder_create(width, height);
	//krad_dirac = krad_dirac_encoder_create(width, height);
	//krad_vorbis = krad_vorbis_encoder_create(2, 44100, 0.7);

	krad_ebml = kradebml_create();

	//kradebml_open_output_file(krad_ebml, filename);
	kradebml_open_output_stream(krad_ebml, "192.168.1.2", 9080, "/krad_test_signal.webm", "secretkode");
	
	
	kradebml_header(krad_ebml, "matroska", APPVERSION);

	videotrack = kradebml_add_video_track(krad_ebml, "V_VP8", fps, width, height);

	bytes = krad_flac_encoder_read_min_header(krad_flac, audio_buffer);
	//audiotrack = kradebml_add_audio_track(krad_ebml, "A_FLAC", sample_rate, channels, audio_buffer, bytes);

	kradebml_write(krad_ebml);

	kradgui_test_screen(kradgui, test_info);

	//kradgui->render_tearbar = 1;


		//krad_tone_run(krad_tone, sample_buffer, 4096);
		//bytes = krad_flac_encode(krad_flac, sample_buffer, 4096, audio_buffer);
		//if (bytes) {
		//	kradebml_add_audio(krad_ebml, audiotrack, audio_buffer, bytes, 4096);
		//	kradebml_write(krad_ebml);
		//}
	printf("\n");
	kradgui_reset_elapsed_time(kradgui);
		
	while (count < TEST_COUNT) {

		cr = cairo_create(cst);
		kradgui->cr = cr;
		kradgui_render(kradgui);
		cairo_destroy(cr);
	
		memcpy(krad_sdl_opengl_display->rgb_frame_data, gui_data, gui_byte_size);
	
		//usleep(20000);
	
		krad_sdl_opengl_draw_screen( krad_sdl_opengl_display );

		//krad_tone_run(krad_tone, sample_buffer, samples_per_frame);
		//bytes = krad_flac_encode(krad_flac, sample_buffer, samples_per_frame, audio_buffer);
		//if (bytes) {
		//	kradebml_add_audio(krad_ebml, audiotrack, audio_buffer, bytes, 4096);
		//	kradebml_write(krad_ebml);
		//}
		
		//if (video_codec == 1) {
		
			rgb_to_yv12(sws_context, gui_data, width, height, krad_vpx_encoder->image->planes, krad_vpx_encoder->image->stride);		
		
			bytes = krad_vpx_encoder_write(krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
		
			if (bytes) {
				kradebml_add_video(krad_ebml, videotrack, vpx_packet, bytes, keyframe);
				kradebml_write(krad_ebml);
			}


		//}
		
		kradgui_update_elapsed_time(kradgui);
		
		count++;
		
		signal_time_ms = count * (1000.0f / (float)fps);
		
		signal_time.tv_sec = (signal_time_ms - (signal_time_ms % 1000)) / 1000;
		signal_time.tv_nsec = (signal_time_ms % 1000) * 1000000;
		
		sleep_time = timespec_diff(kradgui->elapsed_time, signal_time);
		
		if ((sleep_time.tv_sec > -1) && (sleep_time.tv_nsec > 0))  {
			sleeped++;
			nanosleep (&sleep_time, NULL);
		}
		
		printf("Elapsed time %s Sleeped %d times\r", kradgui->elapsed_time_timecode_string, sleeped);
		fflush(stdout);
		
		if ((count % 600) == 0) {
			switch_test(kradgui);
		}	
	}
	printf("\n");
	krad_sdl_opengl_display_destroy(krad_sdl_opengl_display);
	kradgui_destroy(kradgui);
	
	kradebml_destroy(krad_ebml);
	
	//krad_dirac_encoder_destroy(krad_dirac);
	krad_vpx_encoder_destroy(krad_vpx_encoder);
	//krad_vorbis_encoder_destroy(krad_vorbis);
	krad_flac_encoder_destroy(krad_flac);
	
	cairo_surface_destroy(cst);
	
	krad_tone_destroy(krad_tone);

	sws_freeContext (sws_context);

	free (sample_buffer);	
	free (audio_buffer);
	free (video_buffer);
	
	return 0;

}
