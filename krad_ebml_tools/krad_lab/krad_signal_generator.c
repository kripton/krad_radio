#include "krad_gui.h"
#include "krad_ebml.h"
#include "krad_vpx.h"
#include "krad_tone.h"
#include "krad_vorbis.h"

#define APPVERSION "Krad Signal Generator"

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

	kradgui_t *kradgui;
	krad_ebml_t *krad_ebml;
	krad_vorbis_t *krad_vorbis;
	krad_vpx_encoder_t *krad_vpx_encoder;
	krad_tone_t *krad_tone;
	
	struct SwsContext *sws_context;
	
	struct timespec signal_time;
	struct timespec sleep_time;
	uint64_t signal_time_ms;
	
	char output[512];
	char host[512];
	int port;
	char mount[512];
	char password[512];
	
	int width, height;
	int fps;
	int bitrate;
	int count;
	int sleeped;
	
	int channels;
	int sample_rate;
	int bit_depth;
	
	char filename[512];
	char test_info[512];
	int c;
	//unsigned char *audio_buffer;
	//unsigned char *video_buffer;
	//float *sample_buffer;
	
	int videotrack;
	int audiotrack;
	
	int samples_per_frame;
	
	int bytes;
	int total_bytes;
	int keyframe;
	
	void *vpx_packet;
	
	strncpy(test_info, "This is a test of the krad signal generator", sizeof(test_info));
	sprintf(output, "%s/Videos/krad_test_signal_%zu.webm", getenv ("HOME"), time(NULL));
	
	width = 1280;
	height = 720;
	fps = 10;
	bitrate = 500;
	
	total_bytes = 0;
	
	sleeped = 0;
	channels = 1;
	sample_rate = 44100;
	bit_depth = 16;


	samples_per_frame = 1000 / fps * (sample_rate / 1000);

	//printf("samples per frame is %d\n", samples_per_frame);

	count = 0;
	memset(host, '\0', sizeof(host));
	memset(mount, '\0', sizeof(mount));
	memset(password, '\0', sizeof(password));
	
	port = 0;
	
	//sprintf(output, "%s/Videos/krad_cam_%zu.webm", getenv ("HOME"), time(NULL));

	while ((c = getopt (argc, argv, "m:s:i:g:p:o:t:f:w:h:b:")) != -1) {
		switch (c) {
			case 'm':
				strncpy(mount, optarg, sizeof(mount));
				break;
			case 's':
				strncpy(password, optarg, sizeof(password));
				break;
			case 'i':
				strncpy(host, optarg, sizeof(password));
				break;
			case 'g':
				bitrate = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'o':
				strncpy(output, optarg, sizeof(output));
				break;
			case 't':
				strncpy(test_info, optarg, sizeof(test_info));
				break;
			case 'f':
				fps = atoi(optarg);
				break;
			case 'w':
				width = atoi(optarg);
				break;
			case 'h':
				height = atoi(optarg);
				break;
			case 'b':
				bitrate = atoi(optarg);
				break;
			default:
				break;
		}
	}
	
	//audio_buffer = calloc(1, 2000000);
	//video_buffer = calloc(1, 2000000);
	//sample_buffer = calloc(1, 2000000);
	
	sws_context = sws_getContext ( width, height, PIX_FMT_RGB32, width, height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	
	krad_tone = krad_tone_create(sample_rate);
	krad_tone_add_preset(krad_tone, "dialtone");

	kradgui = kradgui_create_with_internal_surface(width, height);
	
	krad_vpx_encoder = krad_vpx_encoder_create(width, height, bitrate);

	//krad_vorbis = krad_vorbis_encoder_create(2, 44100, 0.7);

	krad_ebml = kradebml_create();

	if (host[0] != '\0') {
		kradebml_open_output_stream(krad_ebml, host, port, mount, password);
	} else {
		kradebml_open_output_file(krad_ebml, output);
	}
	
	
	kradebml_header(krad_ebml, "webm", APPVERSION);

	videotrack = kradebml_add_video_track(krad_ebml, "V_VP8", fps, width, height);

	kradebml_write(krad_ebml);

	kradgui_test_screen(kradgui, test_info);

	printf("\n");

	kradgui_reset_elapsed_time(kradgui);
		
	while (1) {

		kradgui_render(kradgui);
		
		rgb_to_yv12(sws_context, kradgui->data, width, height, krad_vpx_encoder->image->planes, krad_vpx_encoder->image->stride);		

		bytes = krad_vpx_encoder_write(krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);

		if (bytes) {
			kradebml_add_video(krad_ebml, videotrack, vpx_packet, bytes, keyframe);
			total_bytes += bytes;		
		}
		
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
		
		printf("Elapsed time: %s Total bytes sent: %d\r", kradgui->elapsed_time_timecode_string, total_bytes);
		fflush(stdout);
		
		if ((count % 600) == 0) {
			switch_test(kradgui);
		}	
	}

	printf("\n");

	kradgui_destroy(kradgui);	
	kradebml_destroy(krad_ebml);
	krad_vpx_encoder_destroy(krad_vpx_encoder);
	//krad_vorbis_encoder_destroy(krad_vorbis);
	
	krad_tone_destroy(krad_tone);

	sws_freeContext (sws_context);

	//free (sample_buffer);	
	//free (audio_buffer);
	//free (video_buffer);
	
	return 0;

}
