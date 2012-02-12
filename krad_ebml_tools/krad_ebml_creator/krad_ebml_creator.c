#include "krad_ebml_creator.h"

krad_ebml_creator_t *krad_ebml_creator_create(char *appname, krad_audio_api_t api) {

	krad_ebml_creator_t *krad_ebml_creator = calloc(1, sizeof(krad_ebml_creator_t));
	
	krad_ebml_creator->samples[0] = malloc(4 * 8192);
	krad_ebml_creator->samples[1] = malloc(4 * 8192);

	krad_ebml_creator->callback_samples[0] = malloc(4 * 8192);
	krad_ebml_creator->callback_samples[1] = malloc(4 * 8192);

	krad_ebml_creator->audio = kradaudio_create(appname, KINPUT, api);
	krad_ebml_creator->ebml = kradebml_create();
	//krad_ebml_creator->opus = kradopus_encoder_create(44100.0f, 2, 192000, OPUS_APPLICATION_AUDIO);
	krad_ebml_creator->vorbis = krad_vorbis_encoder_create(2, 44100, 0.7);
	strncpy(krad_ebml_creator->appname, appname, 512);
	
	return krad_ebml_creator;

}


void krad_ebml_creator_destroy(krad_ebml_creator_t *krad_ebml_creator) {

	
	krad_ebml_creator_stop(krad_ebml_creator);
	
	
	kradaudio_destroy(krad_ebml_creator->audio);
	
	free(krad_ebml_creator->samples[0]);
	free(krad_ebml_creator->samples[1]);
	
	free(krad_ebml_creator->callback_samples[0]);
	free(krad_ebml_creator->callback_samples[1]);
	kradebml_write(krad_ebml_creator->ebml);
	kradebml_destroy(krad_ebml_creator->ebml);
	//kradopus_encoder_destroy(krad_ebml_creator->opus);
	krad_vorbis_encoder_destroy(krad_ebml_creator->vorbis);
	free(krad_ebml_creator);

}

char *krad_ebml_creator_get_output_name (krad_ebml_creator_t *krad_ebml_creator) {

	return krad_ebml_creator->ebml->output_name;

}

char *krad_ebml_creator_get_output_type (krad_ebml_creator_t *krad_ebml_creator) {

	switch (krad_ebml_creator->ebml->output_type) {
	
		case KRADEBML_FILE:
			return "File";
			break;
		case KRADEBML_STREAM:
			return "Stream";
			break;
		case KRADEBML_BUFFER:
			return "Buffer";
			break;
	}
	
	return "";

}

void krad_ebml_creator_stop(krad_ebml_creator_t *krad_ebml_creator) {

	krad_ebml_creator->command = STOP;

}

void krad_ebml_creator_stop_wait(krad_ebml_creator_t *krad_ebml_creator) {

	krad_ebml_creator->command = STOP;

	pthread_join(krad_ebml_creator->creator_thread, NULL);

	krad_ebml_creator->state = STOPPED;

}

void krad_ebml_creator_wait(krad_ebml_creator_t *krad_ebml_creator) {
	
	pthread_join(krad_ebml_creator->creator_thread, NULL);

}

int krad_ebml_creator_get_state(krad_ebml_creator_t *krad_ebml_creator) {

	return krad_ebml_creator->state;

}




void *krad_ebml_creator_thread(void *arg) {

	krad_ebml_creator_t *krad_ebml_creator = (krad_ebml_creator_t *)arg;

	krad_ebml_creator_stream_blocking(krad_ebml_creator, krad_ebml_creator->input);

	return NULL;

}

void krad_ebml_creator_stream_start(krad_ebml_creator_t *krad_ebml_creator, char *stream_url, int video) {

	strncpy(krad_ebml_creator->input, stream_url, 1024);

	krad_ebml_creator->state = STREAMING;
	krad_ebml_creator->command = STREAM;

	krad_ebml_creator->video = video;

	pthread_create( &krad_ebml_creator->creator_thread, NULL, krad_ebml_creator_thread, krad_ebml_creator);

}

void krad_ebml_creator_stream(krad_ebml_creator_t *krad_ebml_creator, char *stream_url) {

	krad_ebml_creator_stream_start(krad_ebml_creator, stream_url, 0);

}

void krad_ebml_creator_stream_video(krad_ebml_creator_t *krad_ebml_creator, char *stream_url) {

	krad_ebml_creator_stream_start(krad_ebml_creator, stream_url, 1);

}

void krad_ebml_creator_audio_callback(int frames, void *userdata) {

	krad_ebml_creator_t *krad_ebml_creator = (krad_ebml_creator_t *)userdata;

	kradaudio_read (krad_ebml_creator->audio, 0, (char *)krad_ebml_creator->samples[0], frames * 4 );
	kradaudio_read (krad_ebml_creator->audio, 1, (char *)krad_ebml_creator->samples[1], frames * 4 );

	//kradopus_write_audio(krad_ebml_creator->opus, 1, (char *)krad_ebml_creator->samples[0], frames * 4);
	//kradopus_write_audio(krad_ebml_creator->opus, 2, (char *)krad_ebml_creator->samples[1], frames * 4);

	//printf("has %d frames\n", frames);	
	
}


void krad_ebml_creator_stream_blocking(krad_ebml_creator_t *krad_ebml_creator, char *stream_url) {


	int t;
	int len;
	unsigned char buffer[8192];

	krad_ebml_creator->state = STREAMING;
	krad_ebml_creator->command = STREAM;


	krad_vpx_encoder_t *krad_vpx_encoder;

	char *device;
	krad_v4l2_t *kradv4l2;

	int width;
	int height;
	int count;
	int fps;

	int keyframe;

	width = 640;
	height = 360;
	fps = 30;
	
	void *frame = NULL;
	unsigned char *vpx_packet;
	int packet_size;
	int videotrack;
	
	videotrack = 0;

	device = DEFAULT_DEVICE;

	printf("Using %s\n", device);




	if (krad_ebml_creator->video == 1) {
	
		krad_vpx_encoder = krad_vpx_encoder_create(width, height, 1000);

		kradv4l2 = kradv4l2_create();

		kradv4l2_open(kradv4l2, device, width, height, fps);
	}
	

	//kradebml_open_output_stream(krad_ebml_creator->ebml, "192.168.1.2", 9080, "/teststream.webm", "secretkode");
		kradebml_open_output_file(krad_ebml_creator->ebml, stream_url);
	//kradebml_header(krad_ebml_creator->ebml, "krad_experimental_opus", krad_ebml_creator->appname);
	
	kradebml_header(krad_ebml_creator->ebml, "webm", krad_ebml_creator->appname);
	
	if (krad_ebml_creator->video == 1) {
		krad_ebml_creator->videotrack = kradebml_add_video_track(krad_ebml_creator->ebml, "V_VP8", fps, width, height);
	}
		
	//krad_ebml_creator->audiotrack = kradebml_add_audio_track(krad_ebml_creator->ebml, "A_KRAD_OPUS", 48000, 2, krad_ebml_creator->opus->header_data, krad_ebml_creator->opus->header_data_size);
	krad_ebml_creator->audiotrack = kradebml_add_audio_track(krad_ebml_creator->ebml, "A_VORBIS", 44100, 2, krad_ebml_creator->vorbis->header, krad_ebml_creator->vorbis->headerpos);

	//kradebml_write(krad_ebml_creator->ebml);

	//kradaudio_set_process_callback(krad_ebml_creator->audio, krad_ebml_creator_audio_callback, krad_ebml_creator);

	if (krad_ebml_creator->video == 1) {
		kradv4l2_start_capturing (kradv4l2);
	}

	while (krad_ebml_creator->command != STOP) {
		
		while (kradaudio_buffered_input_frames(krad_ebml_creator->audio) < 1470) {
			usleep(5000);
			//printf("h3 %zu\n", kradaudio_buffered_input_frames(krad_ebml_creator->audio));
		}
		
		ogg_packet *op;
		
		
		op = krad_vorbis_encode(krad_ebml_creator->vorbis, 1470, krad_ebml_creator->audio->input_ringbuffer[0], krad_ebml_creator->audio->input_ringbuffer[1]);
		
		if (op != NULL) {
		
			//printf("bytes is %ld\n", op->bytes);
		
			kradebml_add_audio(krad_ebml_creator->ebml, krad_ebml_creator->audiotrack, op->packet, op->bytes, 1470);
			kradebml_write(krad_ebml_creator->ebml);		
		
		}
		//printf("h2\n");
		//len = kradopus_read_opus(krad_ebml_creator->opus, buffer);

		//if (len) {
		//	kradebml_add_audio(krad_ebml_creator->ebml, krad_ebml_creator->audiotrack, buffer, len, 960);
		//	kradebml_write(krad_ebml_creator->ebml);
		//	//printf("wroteed\n");
		//} else {
			frame = kradv4l2_read_frame_wait (kradv4l2);
			krad_vpx_convert_uyvy2yv12(krad_vpx_encoder->image, frame, width, height);
			kradv4l2_frame_done (kradv4l2);
			packet_size = krad_vpx_encoder_write(krad_vpx_encoder, &vpx_packet, &keyframe);
		//printf("h4\n");
			if (packet_size) {
				kradebml_add_video(krad_ebml_creator->ebml, videotrack, vpx_packet, packet_size, keyframe);
				kradebml_write(krad_ebml_creator->ebml);
			}
		//}	
		
	}
	
	krad_ebml_creator->state = STOPPED;
	
	if (krad_ebml_creator->video == 1) {
	
		kradv4l2_stop_capturing (kradv4l2);

		kradv4l2_close(kradv4l2);

		kradv4l2_destroy(kradv4l2);
	
		krad_vpx_encoder_destroy(krad_vpx_encoder);
	}	
	
}



