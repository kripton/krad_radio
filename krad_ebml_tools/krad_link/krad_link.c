#include "krad_link.h"

int do_shutdown;
int verbose;

void krad_link_activate(krad_link_t *krad_link);

void dbg (char* format, ...) {

	va_list args;

	if (verbose) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
	}
}

void *video_capture_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	int first = 1;
	struct timespec start_time;
	struct timespec current_time;
	struct timespec elapsed_time;
	void *captured_frame = NULL;
	unsigned char *captured_frame_rgb = malloc(krad_link->composited_frame_byte_size); 
	
	dbg("Video capture thread started\n");
	
	krad_link->krad_v4l2 = kradv4l2_create();

	krad_link->krad_v4l2->mjpeg_mode = krad_link->mjpeg_mode;

	kradv4l2_open(krad_link->krad_v4l2, krad_link->device, krad_link->capture_width, krad_link->capture_height, krad_link->capture_fps);
	
	kradv4l2_start_capturing (krad_link->krad_v4l2);

	while (krad_link->capturing == 1) {

		captured_frame = kradv4l2_read_frame_wait_adv (krad_link->krad_v4l2);
	
		
		if (first == 1) {
			first = 0;
			krad_link->captured_frames = 0;
			krad_link->capture_audio = 1;
			clock_gettime(CLOCK_MONOTONIC, &start_time);
		} else {
		
		
			if ((krad_link->verbose) && (krad_link->captured_frames % 300 == 0)) {
		
				clock_gettime(CLOCK_MONOTONIC, &current_time);
				elapsed_time = timespec_diff(start_time, current_time);
				printf("Frames Captured: %u Expected: %ld - %u fps * %ld seconds\n", krad_link->captured_frames, elapsed_time.tv_sec * krad_link->capture_fps, krad_link->capture_fps, elapsed_time.tv_sec);
	
			}
			
			krad_link->captured_frames++;
	
		}
	
		if (krad_link->mjpeg_mode == 0) {
	
			int rgb_stride_arr[3] = {4*krad_link->composite_width, 0, 0};

			const uint8_t *yuv_arr[4];
			int yuv_stride_arr[4];
		
			yuv_arr[0] = captured_frame;

			yuv_stride_arr[0] = krad_link->capture_width + (krad_link->capture_width/2) * 2;
			yuv_stride_arr[1] = 0;
			yuv_stride_arr[2] = 0;
			yuv_stride_arr[3] = 0;

			unsigned char *dst[4];

			dst[0] = captured_frame_rgb;

			sws_scale(krad_link->captured_frame_converter, yuv_arr, yuv_stride_arr, 0, krad_link->capture_height, dst, rgb_stride_arr);
	
		} else {
		
			kradv4l2_jpeg_to_rgb (krad_link->krad_v4l2, captured_frame_rgb, captured_frame, krad_link->krad_v4l2->jpeg_size);
		
		}
	
		kradv4l2_frame_done (krad_link->krad_v4l2);

	
		if (krad_ringbuffer_write_space(krad_link->captured_frames_buffer) >= krad_link->composited_frame_byte_size) {
	
			krad_ringbuffer_write(krad_link->captured_frames_buffer, 
								 (char *)captured_frame_rgb, 
								 krad_link->composited_frame_byte_size );
	
		}
	}

	kradv4l2_stop_capturing (krad_link->krad_v4l2);
	kradv4l2_close(krad_link->krad_v4l2);
	kradv4l2_destroy(krad_link->krad_v4l2);

	free(captured_frame_rgb);

	krad_link->capture_audio = 2;
	krad_link->encoding = 2;

	dbg("Video capture thread exited\n");
	
	return NULL;
	
}

void *video_encoding_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	if (krad_link->verbose) {
		printf("Video encoding thread started\n");
	}
	
	void *vpx_packet;
	int keyframe;
	int packet_size;
	char keyframe_char[1];
	int first = 1;

	krad_link->krad_vpx_encoder = krad_vpx_encoder_create(krad_link->encoding_width, krad_link->encoding_height, krad_link->vpx_encoder_config.rc_target_bitrate);

	krad_link->vpx_encoder_config.g_w = krad_link->encoding_width;
	krad_link->vpx_encoder_config.g_h = krad_link->encoding_height;

	krad_vpx_encoder_set(krad_link->krad_vpx_encoder, &krad_link->vpx_encoder_config);

	krad_link->krad_vpx_encoder->quality = 1000 * ((krad_link->encoding_fps / 4) * 3);

	dbg("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
	
	while ((krad_link->encoding == 1) || (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= krad_link->composited_frame_byte_size)) {

		if (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= krad_link->composited_frame_byte_size) {
	
			krad_ringbuffer_read(krad_link->composited_frames_buffer, 
								 (char *)krad_link->current_encoding_frame, 
								 krad_link->composited_frame_byte_size );
								 
						
						
			if (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= (krad_link->composited_frame_byte_size * (krad_link->encoding_buffer_frames / 2)) ) {
			
				krad_link->krad_vpx_encoder->quality = (1000 * ((krad_link->encoding_fps / 4) * 3)) / 2LU;
				dbg("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
			}
						
			if (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= (krad_link->composited_frame_byte_size * (krad_link->encoding_buffer_frames / 4)) ) {
			
				krad_link->krad_vpx_encoder->quality = (1000 * ((krad_link->encoding_fps / 4) * 3)) / 4LU;
				dbg("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
			}						 
								 
			int rgb_stride_arr[3] = {4*krad_link->composite_width, 0, 0};
			const uint8_t *rgb_arr[4];
	
			rgb_arr[0] = krad_link->current_encoding_frame;

			sws_scale(krad_link->encoding_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_link->composite_height, 
					  krad_link->krad_vpx_encoder->image->planes, krad_link->krad_vpx_encoder->image->stride);
										 
			packet_size = krad_vpx_encoder_write(krad_link->krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
								 
			//printf("packet size was %d\n", packet_size);
	
			
	
			if (packet_size) {

				keyframe_char[0] = keyframe;

				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)vpx_packet, packet_size);

				if (first == 1) {				
					first = 0;
				}
				
				krad_link->video_frames_encoded++;
				
			}
	
		} else {
		
			if (krad_link->krad_vpx_encoder->quality != (1000 * ((krad_link->encoding_fps / 4) * 3))) {
				krad_link->krad_vpx_encoder->quality = 1000 * ((krad_link->encoding_fps / 4) * 3);
				dbg("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
			}

			
			usleep(4000);
		
		}
		
	}
	
	while (packet_size) {
		krad_vpx_encoder_finish(krad_link->krad_vpx_encoder);
		packet_size = krad_vpx_encoder_write(krad_link->krad_vpx_encoder, (unsigned char **)&vpx_packet, &keyframe);
	}

	krad_vpx_encoder_destroy(krad_link->krad_vpx_encoder);
	
	krad_link->encoding = 3;

	if (krad_link->audio_codec == NOCODEC) {
		krad_link->encoding = 4;
	}
	
	dbg("Video encoding thread exited\n");
	
	return NULL;
	
}


void krad_link_audio_callback(int frames, void *userdata) {

	krad_link_t *krad_link = (krad_link_t *)userdata;
	
	int len;
	
	if (krad_link->operation_mode == CAPTURE) {
	
		kradaudio_read (krad_link->krad_audio, 0, (char *)krad_link->samples[0], frames * 4 );
		kradaudio_read (krad_link->krad_audio, 1, (char *)krad_link->samples[1], frames * 4 );

	}

	if (krad_link->capture_audio == 1) {
		krad_link->audio_frames_captured += frames;
		krad_ringbuffer_write(krad_link->audio_input_ringbuffer[0], (char *)krad_link->samples[0], frames * 4);
		krad_ringbuffer_write(krad_link->audio_input_ringbuffer[1], (char *)krad_link->samples[1], frames * 4);
	}
	
	if ((krad_link->operation_mode == RECEIVE) && (krad_link->audio_codec == VORBIS)) {
	
		len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 0, (char *)krad_link->samples[0], frames * 4);

		if (len) {
			kradaudio_write (krad_link->krad_audio, 0, (char *)krad_link->samples[0], len );
		}

		len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 1, (char *)krad_link->samples[1], frames * 4);

		if (len) {
			kradaudio_write (krad_link->krad_audio, 1, (char *)krad_link->samples[1], len );
		}
	
	}
	
	if (krad_link->capture_audio == 2) {
		krad_link->capture_audio = 3;
	}
	
	
}

void *audio_encoding_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	int c;
	int s;
	int bytes;
	int frames;
	float *samples[MAX_AUDIO_CHANNELS];
	float *interleaved_samples;
	unsigned char *buffer;
	ogg_packet *op;
	int framecnt;

	dbg("Audio encoding thread starting\n");
	
	krad_link->audio_channels = 2;	
	
	if (krad_link->krad_audio_api == ALSA) {
		krad_link->krad_audio = kradaudio_create(krad_link->alsa_capture_device, KINPUT, krad_link->krad_audio_api);
	} else {
		krad_link->krad_audio = kradaudio_create("Krad Link TX", KINPUT, krad_link->krad_audio_api);
	}
		
	if ((krad_link->krad_audio_api == JACK) && (krad_link->jack_ports[0] != '\0')) {
		dbg("Jack ports: %s\n", krad_link->jack_ports);
		jack_connect_to_ports (krad_link->krad_audio, KINPUT, krad_link->jack_ports);
	}
		
	if ((krad_link->krad_audio_api == TONE) && (strncmp(DEFAULT_TONE_PRESET, krad_link->tone_preset, strlen(DEFAULT_TONE_PRESET)) != 0)) {
		kradaudio_set_tone_preset(krad_link->krad_audio, krad_link->tone_preset);
	}
		
	switch (krad_link->audio_codec) {
		case VORBIS:
			krad_link->krad_vorbis = krad_vorbis_encoder_create(krad_link->audio_channels, krad_link->krad_audio->sample_rate, 0.7);
			framecnt = 1024;
			break;
		case FLAC:
			krad_link->krad_flac = krad_flac_encoder_create(krad_link->audio_channels, krad_link->krad_audio->sample_rate, 24);
			framecnt = 4096;
			break;
		case OPUS:
			krad_link->krad_opus = kradopus_encoder_create(krad_link->krad_audio->sample_rate, krad_link->audio_channels, 192000, OPUS_APPLICATION_AUDIO);
			framecnt = 960;
			break;
		default:
			printf("unknown audio codec\n");
			exit(1);
	}


	for (c = 0; c < krad_link->audio_channels; c++) {
		krad_link->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}

	interleaved_samples = calloc(1, 1000000);
	buffer = calloc(1, 1000000);

	kradaudio_set_process_callback(krad_link->krad_audio, krad_link_audio_callback, krad_link);
	
	krad_link->audio_encoder_ready = 1;
	
	if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {
		op = NULL;
	}
	
	while (krad_link->encoding) {

		while ((krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[1]) >= framecnt * 4) || (op != NULL)) {
			
			
			if (krad_link->audio_codec == OPUS) {

				frames = 960;

				for (c = 0; c < krad_link->audio_channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)samples[c], (frames * 4) );
					kradopus_write_audio(krad_link->krad_opus, c + 1, (char *)samples[c], frames * 4);
				}

				bytes = kradopus_read_opus(krad_link->krad_opus, buffer);
			
			}
			
			
			if (krad_link->audio_codec == FLAC) {
			
				frames = 4096;				
				
				for (c = 0; c < krad_link->audio_channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)samples[c], (frames * 4) );
				}
			
				for (s = 0; s < frames; s++) {
					for (c = 0; c < krad_link->audio_channels; c++) {
						interleaved_samples[s * krad_link->audio_channels + c] = samples[c][s];
					}
				}
			
				bytes = krad_flac_encode(krad_link->krad_flac, interleaved_samples, frames, buffer);
	
			}
			
			
			if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {
	
				if (bytes > 0) {
					
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
					
					krad_link->audio_frames_encoded += frames;
					bytes = 0;
				}
			}
			
			if (krad_link->audio_codec == VORBIS) {
			
				op = krad_vorbis_encode(krad_link->krad_vorbis, framecnt, krad_link->audio_input_ringbuffer[0], krad_link->audio_input_ringbuffer[1]);

				if (op != NULL) {
			
					frames = op->granulepos - krad_link->audio_frames_encoded;
				
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&op->bytes, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)op->packet, op->bytes);

				
					krad_link->audio_frames_encoded = op->granulepos;
					
				}
			}

			//printf("audio_encoding_thread %zu !\n", krad_link->audio_frames_encoded);			

		}
	
		while (krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[1]) < framecnt * 4) {
	
			usleep(10000);
	
			if (krad_link->encoding == 3) {
				break;
			}
	
		}
		
		if ((krad_link->encoding == 3) && (krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[1]) < framecnt * 4)) {
				break;
		}
		
	}
	
	dbg("Audio encoding thread exiting\n");
	
	krad_link->encoding = 4;
	
	while (krad_link->capture_audio != 3) {
		usleep(5000);
	}
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		free(krad_link->samples[c]);
		krad_ringbuffer_free ( krad_link->audio_input_ringbuffer[c] );
		free(samples[c]);	
	}	
	
	free(interleaved_samples);
	free(buffer);
	
	return NULL;
	
}

void *ebml_output_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;


	unsigned char *packet;
	int packet_size;
	int keyframe;
	int frames;
	char keyframe_char[1];
	
	int video_frames_muxed;
	int audio_frames_muxed;
	int audio_frames_per_video_frame;

	dbg("Output/Muxing thread starting\n");
	
	audio_frames_muxed = 0;
	video_frames_muxed = 0;

	if (krad_link->krad_audio_api != NOAUDIO) {
		while (krad_link->krad_audio == NULL) {
			usleep(5000);
		}

		if (krad_link->audio_codec == OPUS) {
			audio_frames_per_video_frame = 48000 / krad_link->capture_fps;
		} else {
			audio_frames_per_video_frame = krad_link->krad_audio->sample_rate / krad_link->capture_fps;
		}
	}

	packet = malloc(2000000);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad2_ebml = krad2_ebml_open_stream(krad_link->host, krad_link->port, krad_link->mount, krad_link->password);
	} else {
		printf("Outputing to file: %s\n", krad_link->output);
		krad_link->krad2_ebml = krad2_ebml_open_file(krad_link->output, KRAD2_EBML_IO_WRITEONLY);
	}
	
	if (krad_link->audio_codec != NOCODEC) {
		while ( krad_link->audio_encoder_ready != 1) {
			usleep(10000);
		}
	}
	
	if (((krad_link->audio_codec == VORBIS) || (krad_link->audio_codec == NOCODEC)) && 
		((krad_link->video_codec == VP8) || (krad_link->video_codec == NOCODEC))) {
		
		krad2_ebml_header(krad_link->krad2_ebml, "webm", APPVERSION);
	} else {
		krad2_ebml_header(krad_link->krad2_ebml, "matroska", APPVERSION);
	}
	
	if (krad_link->video_source != NOVIDEO) {
	
		krad_link->video_track = krad2_ebml_add_video_track(krad_link->krad2_ebml, "V_VP8", krad_link->encoding_fps,
											 			 krad_link->encoding_width, krad_link->encoding_height);
	}	
	
	if (krad_link->audio_codec != NOCODEC) {
	
		switch (krad_link->audio_codec) {
			case VORBIS:
				krad_link->audio_track = krad2_ebml_add_audio_track(krad_link->krad2_ebml, "A_VORBIS", krad_link->krad_audio->sample_rate, krad_link->audio_channels, krad_link->krad_vorbis->header, 
																 krad_link->krad_vorbis->headerpos);
				break;
			case FLAC:
				krad_link->audio_track = krad2_ebml_add_audio_track(krad_link->krad2_ebml, "A_FLAC", krad_link->krad_audio->sample_rate, krad_link->audio_channels, (unsigned char *)krad_link->krad_flac->min_header, FLAC_MINIMAL_HEADER_SIZE);
				break;
			case OPUS:
				krad_link->audio_track = krad2_ebml_add_audio_track(krad_link->krad2_ebml, "A_OPUS", 48000, krad_link->audio_channels, krad_link->krad_opus->header_data, krad_link->krad_opus->header_data_size);
				break;
			default:
				printf("Unknown audio codec\n");
				exit(1);
		}
	
	}
	
	//kradebml_write(krad_link->krad2_ebml);
	
	dbg("Output/Muxing thread waiting..\n");
		
	while ( krad_link->encoding ) {

		if (krad_link->video_source != NOVIDEO) {
			if ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) >= 4) && (krad_link->encoding < 3)) {

				krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < packet_size + 1) && (krad_link->encoding < 3)) {
					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < packet_size + 1) && (krad_link->encoding > 2)) {
					continue;
				}
			
				krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)packet, packet_size);

				keyframe = keyframe_char[0];
	

				krad2_ebml_add_video(krad_link->krad2_ebml, krad_link->video_track, packet, packet_size, keyframe);

				video_frames_muxed++;
		
			}
		}
		
		if (krad_link->audio_codec != NOCODEC) {
		
			if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) >= 4) && 
				((krad_link->video_source == NOVIDEO) || ((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed))) {

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding != 4)) {
					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding == 4)) {
					break;
				}
			
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)packet, packet_size);

				krad2_ebml_add_audio(krad_link->krad2_ebml, krad_link->audio_track, packet, packet_size, frames);

				audio_frames_muxed += frames;
				
				//printf("ebml muxed audio frames: %d\n\n", audio_frames_muxed);
				
			} else {
				if (krad_link->encoding == 4) {
					break;
				}
			}
		}
		
		
		if ((krad_link->audio_codec != NOCODEC) && (!(krad_link->video_source))) {
		
			if (krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) {
				
				if (krad_link->encoding == 4) {
					break;
				}
		
				usleep(10000);
			
			}
		
		}
		
		if ((!(krad_link->audio_codec != NOCODEC)) && (krad_link->video_source)) {
		
			if (krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4) {
		
				if (krad_link->encoding == 4) {
					break;
				}
		
				usleep(10000);
			
			}
		
		}


		if ((krad_link->audio_codec != NOCODEC) && (krad_link->video_source)) {

			if (((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) || 
				((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed)) && 
			   (krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4)) {
		
				if (krad_link->encoding == 4) {
					break;
				}
		
				usleep(10000);
			
			}
		
		
		}		
		
	
	}

	krad2_ebml_destroy(krad_link->krad2_ebml);
	
	free(packet);

	dbg("Output/Muxing thread exiting\n");
	
	return NULL;
	
}

void krad_link_handle_input(krad_link_t *krad_link) {

	while ( SDL_PollEvent( &krad_link->event ) ){
		switch( krad_link->event.type ){
			/* Look for a keypress */
			case SDL_KEYDOWN:
				/* Check the SDLKey values and move change the coords */
				switch( krad_link->event.key.keysym.sym ){
					case SDLK_LEFT:
						break;
					case SDLK_RIGHT:
						break;
					case SDLK_UP:
						break;
					case SDLK_z:
						kradgui_remove_bug (krad_link->krad_gui);
						break;
					case SDLK_b:
						if (krad_link->bug[0] != '\0') {
							kradgui_set_bug (krad_link->krad_gui, krad_link->bug, krad_link->bug_x, krad_link->bug_y);
						}
						break;
					case SDLK_l:
						if (krad_link->krad_gui->live == 1) {
							krad_link->krad_gui->live = 0;
						} else {
							krad_link->krad_gui->live = 1;
						}
						break;
					case SDLK_0:
						krad_link->krad_gui->bug_alpha += 0.1f;
						if (krad_link->krad_gui->bug_alpha > 1.0f) {
							krad_link->krad_gui->bug_alpha = 1.0f;
						}
						break;
					case SDLK_9:
						krad_link->krad_gui->bug_alpha -= 0.1f;
						if (krad_link->krad_gui->bug_alpha < 0.1f) {
							krad_link->krad_gui->bug_alpha = 0.1f;
						}
						break;
					case SDLK_q:
						*krad_link->shutdown = 1;
						break;
					case SDLK_f:
						if (krad_link->krad_gui->render_ftest == 1) {
							krad_link->krad_gui->render_ftest = 0;
						} else {
							krad_link->krad_gui->render_ftest = 1;
						}
						break;
					case SDLK_v:
						if (krad_link->render_meters == 1) {
							krad_link->render_meters = 0;
						} else {
							krad_link->render_meters = 1;
						}
						break;
					default:
						break;
				}
				break;

			case SDL_KEYUP:
				switch( krad_link->event.key.keysym.sym ) {
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
				krad_link->krad_gui->cursor_x = krad_link->event.motion.x;
				krad_link->krad_gui->cursor_y = krad_link->event.motion.y;
				break;
	
			case SDL_QUIT:
				*krad_link->shutdown = 1;	 			
				break;

			default:
				break;
		}
	}

}

void krad_link_display(krad_link_t *krad_link) {

	if ((krad_link->composite_width == krad_link->display_width) && (krad_link->composite_height == krad_link->display_height)) {

		memcpy( krad_link->krad_opengl_display->rgb_frame_data, krad_link->krad_gui->data, krad_link->krad_gui->bytes );

	} else {


		int rgb_stride_arr[3] = {4*krad_link->display_width, 0, 0};

		const uint8_t *rgb_arr[4];
		int rgb1_stride_arr[4];

		rgb_arr[0] = krad_link->krad_gui->data;

		rgb1_stride_arr[0] = krad_link->composite_width * 4;
		rgb1_stride_arr[1] = 0;
		rgb1_stride_arr[2] = 0;
		rgb1_stride_arr[3] = 0;

		unsigned char *dst[4];

		dst[0] = krad_link->krad_opengl_display->rgb_frame_data;

		sws_scale(krad_link->display_frame_converter, rgb_arr, rgb1_stride_arr, 0, krad_link->display_height, dst, rgb_stride_arr);


	}

	krad_sdl_opengl_draw_screen( krad_link->krad_opengl_display );

	//if (read_composited) {
	//	krad_sdl_opengl_read_screen( krad_link->krad_opengl_display, krad_link->krad_opengl_display->rgb_frame_data);
	//}

}

void krad_link_composite(krad_link_t *krad_link) {

	int c;

	if (krad_link->operation_mode == RECEIVE) {
		if (krad_ringbuffer_read_space(krad_link->decoded_frames_buffer) >= krad_link->composited_frame_byte_size) {
	
			krad_ringbuffer_read(krad_link->decoded_frames_buffer, 
									 (char *)krad_link->current_frame, 
									 krad_link->composited_frame_byte_size );
		
			if (krad_link->link_started == 0) {
				krad_link->link_started = 1;
				//krad_link->krad_gui->render_ftest = 1;
				//kradgui_go_live(krad_link->krad_gui);
			}
		
			krad_link->krad_gui->clear = 0;
		
			krad_link->new_capture_frame = 1;
		} else {
			krad_link->new_capture_frame = 0;
		}

		memcpy( krad_link->krad_gui->data, krad_link->current_frame, krad_link->krad_gui->bytes );
	
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		if (krad_link->capturing) {
			if (krad_ringbuffer_read_space(krad_link->captured_frames_buffer) >= krad_link->composited_frame_byte_size) {
	
				krad_ringbuffer_read(krad_link->captured_frames_buffer, 
										 (char *)krad_link->current_frame, 
										 krad_link->composited_frame_byte_size );
		
				if (krad_link->link_started == 0) {
					krad_link->link_started = 1;
					//krad_link->krad_gui->render_ftest = 1;
					kradgui_go_live(krad_link->krad_gui);
				}
		
				krad_link->new_capture_frame = 1;
			} else {
				krad_link->new_capture_frame = 0;
			}

			memcpy( krad_link->krad_gui->data, krad_link->current_frame, krad_link->krad_gui->bytes );
	
		} else {
			krad_link->capture_audio = 1;
			krad_link->new_capture_frame = 1;
			krad_link->link_started = 1;
		}
	}
	
	kradgui_render( krad_link->krad_gui );
	
	if (krad_link->render_meters) {
		if ((krad_link->new_capture_frame == 1) || ((krad_link->operation_mode == RECEIVE) && (krad_link->video_codec == NOCODEC))) {
			if (krad_link->krad_audio != NULL) {
				for (c = 0; c < krad_link->audio_channels; c++) {
				
					if (krad_link->operation_mode == CAPTURE) {
						krad_link->temp_peak = read_peak(krad_link->krad_audio, KINPUT, c);
					} else {
						krad_link->temp_peak = read_peak(krad_link->krad_audio, KOUTPUT, c);
					}
				
					if (krad_link->temp_peak >= krad_link->krad_gui->output_peak[c]) {
						if (krad_link->temp_peak > 2.7f) {
							krad_link->krad_gui->output_peak[c] = krad_link->temp_peak;
							krad_link->kick = ((krad_link->krad_gui->output_peak[c] - krad_link->krad_gui->output_current[c]) / 300.0);
						}
					} else {
						if (krad_link->krad_gui->output_peak[c] == krad_link->krad_gui->output_current[c]) {
							krad_link->krad_gui->output_peak[c] -= (0.9 * (60/krad_link->capture_fps));
							if (krad_link->krad_gui->output_peak[c] < 0.0f) {
								krad_link->krad_gui->output_peak[c] = 0.0f;
							}
							krad_link->krad_gui->output_current[c] = krad_link->krad_gui->output_peak[c];
						}
					}
			
					if (krad_link->krad_gui->output_peak[c] > krad_link->krad_gui->output_current[c]) {
						krad_link->krad_gui->output_current[c] = (krad_link->krad_gui->output_current[c] + 1.4) * (1.3 + krad_link->kick); ;
					}
		
					if (krad_link->krad_gui->output_peak[c] < krad_link->krad_gui->output_current[c]) {
						krad_link->krad_gui->output_current[c] = krad_link->krad_gui->output_peak[c];
					}
			
			
				}
			}
		}
	

		kradgui_render_meter (krad_link->krad_gui, 110, krad_link->composite_height - 30, 96, krad_link->krad_gui->output_current[0]);
		kradgui_render_meter (krad_link->krad_gui, krad_link->composite_width - 110, krad_link->composite_height - 30, 96, krad_link->krad_gui->output_current[1]);
	}
	
	
	if ((krad_link->operation_mode == CAPTURE) && (krad_link->video_source != NOVIDEO)) {
	
		if ((krad_link->link_started == 1) && (krad_link->new_capture_frame == 1) && (krad_ringbuffer_write_space(krad_link->composited_frames_buffer) >= krad_link->composited_frame_byte_size)) {

				krad_ringbuffer_write(krad_link->composited_frames_buffer, 
									  (char *)krad_link->krad_gui->data, 
									  krad_link->composited_frame_byte_size );

		} else {
	
			if ((krad_link->link_started == 1) && (krad_link->new_capture_frame == 1)) {
				dbg("encoding to slow! overflow!\n");
			}
	
		}
	}
		
}


void krad_link_run(krad_link_t *krad_link) {

	krad_link_activate ( krad_link );

	if (krad_link->operation_mode == RECEIVE) {
	
		while (krad_link->input_ready != 1) {
			usleep(5000);
			if (*krad_link->shutdown) {
				break;
			}
		}
	
		if ((!*krad_link->shutdown) && (krad_link->input_ready) && (krad_link->interface_mode == WINDOW)) {
	
			//krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, 1920, 1080, krad_ebml->vparams.width, krad_ebml->vparams.height);
			if (krad_link->video_codec != NOCODEC) {
				krad_link->krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, krad_link->krad2_ebml->width, krad_link->krad2_ebml->height, krad_link->krad2_ebml->width, krad_link->krad2_ebml->height);
			} else {
				krad_link->krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, krad_link->display_width, krad_link->display_height, krad_link->display_width, krad_link->display_height);
			}
		}
	
		while (!*krad_link->shutdown) {

			if (krad_link->interface_mode != WINDOW) {
	
				usleep(40000);

			} else {

				
				krad_link_composite(krad_link);
				krad_link_display(krad_link);
				krad_link_handle_input(krad_link);

				
				kradgui_update_elapsed_time(krad_link->krad_gui);

				krad_link->next_frame_time_ms = krad_link->krad_gui->frame * (1000.0f / (float)krad_link->composite_fps);

				krad_link->next_frame_time.tv_sec = (krad_link->next_frame_time_ms - (krad_link->next_frame_time_ms % 1000)) / 1000;
				krad_link->next_frame_time.tv_nsec = (krad_link->next_frame_time_ms % 1000) * 1000000;

				krad_link->sleep_time = timespec_diff(krad_link->krad_gui->elapsed_time, krad_link->next_frame_time);

				if ((krad_link->sleep_time.tv_sec > -1) && (krad_link->sleep_time.tv_nsec > 0))  {
				//	nanosleep (&krad_link->sleep_time, NULL);
				}

				//printf("\n\nElapsed time %s\r", krad_link->krad_gui->elapsed_time_timecode_string);
				//fflush(stdout);
		
			}
		}

	
	} else {

		while (!*krad_link->shutdown) {

			if ((krad_link->capturing) && (krad_link->interface_mode != WINDOW)) {
				while (krad_ringbuffer_read_space(krad_link->captured_frames_buffer) < krad_link->composited_frame_byte_size) {
					usleep(5000);
				}
			}

			if (!krad_link->capturing) {
			
			}

			krad_link_composite ( krad_link );

			if (krad_link->interface_mode == WINDOW) {
				krad_link_display(krad_link);
				krad_link_handle_input(krad_link);
			}
		
		
		
			if (!krad_link->capturing) {
			
				// Waiting so that test signal creation is "in actual time" like live video sources
			
				kradgui_update_elapsed_time(krad_link->krad_gui);

				krad_link->next_frame_time_ms = krad_link->krad_gui->frame * (1000.0f / (float)krad_link->composite_fps);

				krad_link->next_frame_time.tv_sec = (krad_link->next_frame_time_ms - (krad_link->next_frame_time_ms % 1000)) / 1000;
				krad_link->next_frame_time.tv_nsec = (krad_link->next_frame_time_ms % 1000) * 1000000;

				krad_link->sleep_time = timespec_diff(krad_link->krad_gui->elapsed_time, krad_link->next_frame_time);

				if ((krad_link->sleep_time.tv_sec > -1) && (krad_link->sleep_time.tv_nsec > 0))  {
					nanosleep (&krad_link->sleep_time, NULL);
				}

				printf("Elapsed time %s\r", krad_link->krad_gui->elapsed_time_timecode_string);
				fflush(stdout);
	
			}
		
		}
	}


	krad_link_destroy ( krad_link );

}

void *ebml_input_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("Input/Demuxing thread starting\n");

	unsigned char *buffer;
	int bytes;
	int codec_bytes;
	int p;
	int video_packets;
	int audio_packets;
	int current_track;
	krad_codec_t codec;
	int packet_size;
	int tracks;
	int audio_track;
	int video_track;
	
	video_track = 0;
	audio_track = 0;
	
	packet_size = 0;
	codec_bytes = 0;	
	bytes = 0;	
	video_packets = 0;
	audio_packets = 0;
	current_track = -1;

	buffer = malloc(4096 * 16);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad2_ebml = krad2_ebml_open_stream(krad_link->host, krad_link->port, krad_link->mount, NULL);
	} else {
		krad_link->krad2_ebml = krad2_ebml_open_file(krad_link->input, KRAD2_EBML_IO_READONLY);
	}

	//kradgui_set_total_track_time_ms(krad_link->krad_gui, (5000000 / 1e9) * 1000);

	krad_link->krad_gui->render_timecode = 1;
	
	tracks = krad_link->krad2_ebml->track_count;
	
	krad_link->audio_codec = NOCODEC;
	krad_link->video_codec = NOCODEC;
	
	while (tracks) {
	
		codec =	krad2_ebml_get_track_codec (krad_link->krad2_ebml, tracks);
	
		if ((codec == VP8) || (codec == DIRAC) || (codec == THEORA)) {
		
			krad_link->video_codec = codec;
			video_track = tracks;
		
		}
		
		if ((codec == VORBIS) || (codec == FLAC) || (codec == OPUS)) {
		
			krad_link->audio_codec = codec;
			audio_track = tracks;
		
		}
		
		
		tracks --;
	}
	
	dbg("Video codec is %d and audio codec is %d\n", krad_link->video_codec, krad_link->audio_codec);
	
	if (krad_link->audio_codec == FLAC) {
		krad_link->krad_flac = krad_flac_decoder_create();
		bytes = krad2_ebml_get_track_codec_data(krad_link->krad2_ebml, audio_track, buffer);
		dbg("FLAC Header size: %d\n", bytes);
		krad_flac_decode(krad_link->krad_flac, buffer, bytes, NULL);
	}
	
	if (krad_link->audio_codec == VORBIS) {
		dbg("Vorbis Header byte sizes: %d %d %d\n", krad_link->krad2_ebml->tracks[audio_track].xiph_header_len[0], krad_link->krad2_ebml->tracks[audio_track].xiph_header_len[1], krad_link->krad2_ebml->tracks[audio_track].xiph_header_len[2]);
		krad_link->krad_vorbis = krad_vorbis_decoder_create(krad_link->krad2_ebml->tracks[audio_track].xiph_header[0], krad_link->krad2_ebml->tracks[audio_track].xiph_header_len[0], krad_link->krad2_ebml->tracks[audio_track].xiph_header[1], krad_link->krad2_ebml->tracks[audio_track].xiph_header_len[1], krad_link->krad2_ebml->tracks[audio_track].xiph_header[2], krad_link->krad2_ebml->tracks[audio_track].xiph_header_len[2]);
	}
	
	if (krad_link->audio_codec == OPUS) {
		bytes = krad2_ebml_get_track_codec_data(krad_link->krad2_ebml, audio_track, buffer);
		dbg("Opus Header size: %d\n", bytes);
		krad_link->krad_opus = kradopus_decoder_create(buffer, bytes, 44100.0f);
	}
	
	if (krad_link->video_codec == VP8) {
		krad_link->krad_vpx_decoder = krad_vpx_decoder_create();
	}
	
	if (krad_link->video_codec == THEORA) {
		dbg("Theora Header byte sizes: %d %d %d\n", krad_link->krad2_ebml->tracks[video_track].xiph_header_len[0], krad_link->krad2_ebml->tracks[video_track].xiph_header_len[1], krad_link->krad2_ebml->tracks[video_track].xiph_header_len[2]);
		//krad_link->krad_theora = krad_theora_decoder_create(krad_link->krad2_ebml->tracks[video_track].xiph_header[0], krad_link->krad2_ebml->tracks[video_track].xiph_header_len[0], krad_link->krad2_ebml->tracks[video_track].xiph_header[1], krad_link->krad2_ebml->tracks[video_track].xiph_header_len[1], krad_link->krad2_ebml->tracks[video_track].xiph_header[2], krad_link->krad2_ebml->tracks[video_track].xiph_header_len[2]);
	}
	
	if (krad_link->video_codec == DIRAC) {
		//krad_dirac = krad_dirac_decoder_create();
	}	
	
	if ((krad_link->interface_mode == WINDOW) && (krad_link->video_codec != NOCODEC)) {
		
		krad_link->composite_width = krad_link->krad2_ebml->width;
		krad_link->composite_height = krad_link->krad2_ebml->height;

		krad_link->display_width = krad_link->composite_width;
		krad_link->display_height = krad_link->composite_height;

		krad_link->composited_frame_byte_size = krad_link->composite_width * krad_link->composite_height * 4;
		krad_link->current_frame = realloc(krad_link->current_frame, krad_link->composited_frame_byte_size);
		kradgui_destroy(krad_link->krad_gui);
		krad_link->krad_gui = kradgui_create_with_internal_surface(krad_link->composite_width, krad_link->composite_height);
		
		krad_link->decoded_frame_converter = sws_getContext ( krad_link->composite_width, krad_link->composite_height, PIX_FMT_YUV420P,
															  krad_link->display_width, krad_link->display_height, PIX_FMT_RGB32, 
															  SWS_BICUBIC, NULL, NULL, NULL);
	}
	
	if ((krad_link->interface_mode == WINDOW) && (krad_link->video_codec == NOCODEC)) {
		krad_link->render_meters = 1;
	}
	
	krad_link->input_ready = 1;

	kradgui_reset_elapsed_time(krad_link->krad_gui);
	
	while (!*krad_link->shutdown) {
		
		packet_size = krad2_ebml_read_packet ( krad_link->krad2_ebml, &current_track, buffer);
		
		if (packet_size <= 0) {
			printf("\nebml input thread packet size was: %d\n", packet_size);
			break;
		}	
		
		if (current_track == video_track) {

			video_packets++;

			if (krad_link->interface_mode == WINDOW) {
				while ((krad_ringbuffer_write_space(krad_link->encoded_video_ringbuffer) < packet_size + 4) && (!*krad_link->shutdown)) {
					usleep(10000);
				}
				
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);

				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)buffer, packet_size);
				codec_bytes += packet_size;
			}
		}
		
		if (current_track == audio_track) {

			audio_packets++;

			//kradgui_set_current_track_time_ms(krad_link->krad_gui, (krad_link->krad2_ebml->current_timecode / 1e9) * 1000);

			if (krad_link->krad_audio_api != NOAUDIO) {
			
				while ((krad_ringbuffer_write_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (!*krad_link->shutdown)) {
					usleep(50000);
					if (krad_ringbuffer_write_space(krad_link->encoded_audio_ringbuffer) < 100000) {
						usleep(100000);
					}
				}
				
				krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
				//krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
				krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)buffer, packet_size);
				codec_bytes += packet_size;
			}
			
		}

		if (krad_link->krad_audio != NULL) {
			dbg("Input Timecode: %6.1f :: Video Packets %d Audio Packets: %d Codec bytes Read %d\r", krad_link->krad2_ebml->current_timecode / 1000.0, video_packets, audio_packets, codec_bytes);
			fflush(stdout);
		}

	}


	
	printf("\n");
	dbg("Input/Demuxing thread exiting\n");

	krad2_ebml_destroy(krad_link->krad2_ebml);
	free(buffer);
	return NULL;

}

void krad_link_yuv_to_rgb(krad_link_t *krad_link, unsigned char *output, unsigned char *y, int ys, unsigned char *u, int us, unsigned char *v, int vs) {

	int rgb_stride_arr[3] = {4*krad_link->display_width, 0, 0};

	const uint8_t *yv12_arr[4];
	
	yv12_arr[0] = y;
	yv12_arr[1] = u;
	yv12_arr[2] = v;
	
	int yv12_str_arr[4];
	
	yv12_str_arr[0] = ys;
	yv12_str_arr[1] = us;
	yv12_str_arr[2] = vs;

	unsigned char *dst[4];
	dst[0] = output;

	sws_scale(krad_link->decoded_frame_converter, yv12_arr, yv12_str_arr, 0, krad_link->composite_height, dst, rgb_stride_arr);

}

void *video_decoding_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("Video decoding thread starting\n");

	int bytes;
	unsigned char *buffer;	
	unsigned char *converted_buffer;

	bytes = 0;
	buffer = malloc(1000000);
	converted_buffer = malloc(4000000);

	while (!*krad_link->shutdown) {

		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4) && (!*krad_link->shutdown)) {
			usleep(10000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < bytes) && (!*krad_link->shutdown)) {
			usleep(10000);
		}

		//krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
		
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)buffer, bytes);

		if (krad_link->video_codec == VP8) {

			krad_vpx_decoder_decode(krad_link->krad_vpx_decoder, buffer, bytes);
				
			if (krad_link->krad_vpx_decoder->img != NULL) {

				krad_link_yuv_to_rgb(krad_link, converted_buffer, krad_link->krad_vpx_decoder->img->planes[0], 
									 krad_link->krad_vpx_decoder->img->stride[0], krad_link->krad_vpx_decoder->img->planes[1], 
									 krad_link->krad_vpx_decoder->img->stride[1], krad_link->krad_vpx_decoder->img->planes[2], 
									 krad_link->krad_vpx_decoder->img->stride[2]);

				while ((krad_ringbuffer_write_space(krad_link->decoded_frames_buffer) < krad_link->composited_frame_byte_size) && (!*krad_link->shutdown)) {
					usleep(10000);
				}
				
				krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)converted_buffer, krad_link->composited_frame_byte_size);

				//dbg("wrote %d to decoded frames buffer\n", krad_link->composited_frame_byte_size);
			}
		}
		

	/*	
		if (video_codec == KRAD_THEORA) {

			krad_theora_decoder_decode(krad_theora_decoder, buffer, krad_ebml->size);

			krad_sdl_opengl_display_render(krad_opengl_display, krad_theora_decoder->ycbcr[0].data, krad_theora_decoder->ycbcr[0].stride, krad_theora_decoder->ycbcr[1].data, krad_theora_decoder->ycbcr[1].stride, krad_theora_decoder->ycbcr[2].data, krad_theora_decoder->ycbcr[2].stride);

		}

		if (video_codec == KRAD_DIRAC) {

			krad_dirac_decode(krad_dirac, buffer, krad_ebml->size);

			if ((krad_dirac->format != NULL) && (dirac_output_unset == true)) {

				switch (krad_dirac->format->chroma_format) {
					case SCHRO_CHROMA_444:
						krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV444P);
					case SCHRO_CHROMA_422:
						krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV422P);
					case SCHRO_CHROMA_420:
						// default
						//krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display, PIX_FMT_YUV420P);
						break;
				}

				dirac_output_unset = false;				
			}

			if (krad_dirac->frame != NULL) {
				krad_dirac->frame->components[0].data, krad_dirac->frame->components[0].stride, krad_dirac->frame->components[1].data, krad_dirac->frame->components[1].stride, krad_dirac->frame->components[2].data, krad_dirac->frame->components[2].stride);
			}

		}
	*/


	}

	free (converted_buffer);
	free (buffer);

	dbg("Video decoding thread exiting\n");

	return NULL;

}

void *audio_decoding_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("Audio decoding thread starting\n");

	int c;
	int bytes;
	unsigned char *buffer;
	float *audio;
	float *samples[MAX_AUDIO_CHANNELS];
	int audio_frames;
		
	while (krad_link->input_ready != 1) {
		usleep(5000);
		if (*krad_link->shutdown) {
			return NULL;
		}
	}
	
	krad_link->audio_channels = 2;

	for (c = 0; c < krad_link->audio_channels; c++) {
		krad_link->audio_output_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}
	
	buffer = malloc(2000000);
	audio = calloc(1, 8192 * 4 * 4);


	if (krad_link->krad_audio_api == ALSA) {
		krad_link->krad_audio = kradaudio_create(krad_link->alsa_playback_device, KOUTPUT, krad_link->krad_audio_api);
	} else {
		krad_link->krad_audio = kradaudio_create("Krad Link RX", KOUTPUT, krad_link->krad_audio_api);
	}
	
	if ((krad_link->krad_audio_api == JACK) && (krad_link->jack_ports[0] != '\0')) {
		dbg("Jack ports: %s\n", krad_link->jack_ports);
		jack_connect_to_ports (krad_link->krad_audio, KOUTPUT, krad_link->jack_ports);
	}

	if (krad_link->audio_codec == VORBIS) {
		kradaudio_set_process_callback(krad_link->krad_audio, krad_link_audio_callback, krad_link);
	}	
	
	while (!*krad_link->shutdown) {


		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!*krad_link->shutdown)) {
			usleep(20000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < bytes) && (!*krad_link->shutdown)) {
			usleep(30000);
		}

		//krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
		
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
		
		if (krad_link->audio_codec == VORBIS) {
			krad_vorbis_decoder_decode(krad_link->krad_vorbis, buffer, bytes);
		}
			
		if (krad_link->audio_codec == FLAC) {

			audio_frames = krad_flac_decode(krad_link->krad_flac, buffer, bytes, samples);
			
			for (c = 0; c < krad_link->audio_channels; c++) {
				kradaudio_write (krad_link->krad_audio, c, (char *)samples[c], audio_frames * 4 );
			}
		}
			
		if (krad_link->audio_codec == OPUS) {
		
			kradopus_decoder_set_speed(krad_link->krad_opus, 100);
			kradopus_write_opus(krad_link->krad_opus, buffer, bytes);

			int c;

			bytes = -1;			
			while (bytes != 0) {
				for (c = 0; c < 2; c++) {
					bytes = kradopus_read_audio(krad_link->krad_opus, c + 1, (char *)audio, 960 * 4);
					if (bytes) {
						//printf("\nAudio data! %d samplers\n", bytes / 4);
						kradaudio_write (krad_link->krad_audio, c, (char *)audio, bytes );
					}
				}
			}
		}
	}
	
	// must be before vorbis
	if (krad_link->krad_audio != NULL) {
		kradaudio_destroy (krad_link->krad_audio);
		krad_link->krad_audio = NULL;
	}

	if (krad_link->krad_vorbis != NULL) {
		krad_vorbis_decoder_destroy (krad_link->krad_vorbis);
		krad_link->krad_vorbis = NULL;
	}
	
	if (krad_link->krad_flac != NULL) {
		krad_flac_decoder_destroy (krad_link->krad_flac);
		krad_link->krad_flac = NULL;
	}

	if (krad_link->krad_opus != NULL) {
		kradopus_decoder_destroy(krad_link->krad_opus);
		krad_link->krad_opus = NULL;
	}

	free(buffer);
	free(audio);
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		free(krad_link->samples[c]);
		krad_ringbuffer_free ( krad_link->audio_output_ringbuffer[c] );
		free(samples[c]);	
	}	

	dbg("Audio decoding thread exiting\n");

	return NULL;

}

void daemonize() {

	char *daemon_name;
	pid_t pid, sid;
	char err_log_file[98] = "";
	char log_file[98] = "";
	char *homedir = getenv ("HOME");
	int i;
	char timestamp[64];
	FILE *fp;
	time_t ltime;

	daemon_name = "krad_link";

	printf("Krad Link Daemonizing..\n");

	pid = fork();

	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}
	
	umask(0);
 
	sid = setsid();
	
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	close (STDIN_FILENO);
	//close(STDOUT_FILENO);
	//close(STDERR_FILENO);

	ltime = time (NULL);
	
	sprintf( timestamp, "%s", asctime (localtime(&ltime)) );

	for (i = 0; i < strlen (timestamp); i++) {

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
		
	fp = freopen( err_log_file, "w", stderr );

	if (fp == NULL) {
		printf("Error in freopen stderr\n");
	}

	fp = freopen( log_file, "w", stdout );

	if (fp == NULL) {
		printf("Error in freopen stdout\n");
	}
        
}

void *signal_thread(void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	int s, sig;

	s = sigwait(&krad_link->set, &sig);
	if (s != 0) {
		printf("\n\nproblem getting signal %d\n\n", sig);
	}

	printf("\n\nSignal handling thread got signal %d\n\n", sig);

	if (krad_link->krad_audio != NULL) {
		krad_link->krad_audio->destroy = 1;
	}

	krad_link_shutdown();

	
	return NULL;
}

void krad_link_shutdown() {

	if (do_shutdown == 1) {
		printf("\nKrad Link Terminated.\n");
		exit(1);
	}



	do_shutdown = 1;

}

void krad_link_destroy(krad_link_t *krad_link) {

	dbg("Krad Link shutting down\n");
	pthread_sigmask (SIG_UNBLOCK, &krad_link->set, NULL);
	signal(SIGINT, krad_link_shutdown);
	signal(SIGTERM, krad_link_shutdown);

	krad_link->capturing = 0;
	
	if (krad_link->capturing) {
		krad_link->capturing = 0;
		pthread_join(krad_link->video_capture_thread, NULL);
	} else {
		krad_link->capture_audio = 2;
		krad_link->encoding = 2;
	}
	
	if (krad_link->krad_vpx_decoder) {
		krad_vpx_decoder_destroy(krad_link->krad_vpx_decoder);
	}
	
	if (krad_link->video_source != NOVIDEO) {
		pthread_join(krad_link->video_encoding_thread, NULL);
	} else {
		krad_link->encoding = 3;
	}
	
	if (krad_link->audio_codec != NOCODEC) {
		pthread_join(krad_link->audio_encoding_thread, NULL);
	}
	
	pthread_join(krad_link->ebml_output_thread, NULL);

	if (krad_link->interface_mode == WINDOW) {
		krad_sdl_opengl_display_destroy(krad_link->krad_opengl_display);
	}

	if (krad_link->operation_mode == RECEIVE) {


		if ((krad_link->video_codec != NOCODEC) && (krad_link->video_source != NOVIDEO)) {
			pthread_join(krad_link->video_decoding_thread, NULL);
		}
		
		if ((krad_link->audio_codec != NOCODEC) && (krad_link->krad_audio_api != NOAUDIO)) {
			pthread_join(krad_link->audio_decoding_thread, NULL);
		}
		
		pthread_join(krad_link->ebml_input_thread, NULL);
		
	}


	kradgui_destroy(krad_link->krad_gui);

	if (krad_link->decoded_frame_converter != NULL) {
		sws_freeContext ( krad_link->decoded_frame_converter );
	}
	
	sws_freeContext ( krad_link->captured_frame_converter );
	sws_freeContext ( krad_link->encoding_frame_converter );
	sws_freeContext ( krad_link->display_frame_converter );

	// must be before vorbis
	if (krad_link->krad_audio != NULL) {
		kradaudio_destroy (krad_link->krad_audio);
	}
	
	if (krad_link->krad_vorbis != NULL) {
		krad_vorbis_encoder_destroy (krad_link->krad_vorbis);
	}
	
	if (krad_link->krad_flac != NULL) {
		krad_flac_encoder_destroy (krad_link->krad_flac);
	}

	if (krad_link->krad_opus != NULL) {
		kradopus_encoder_destroy(krad_link->krad_opus);
	}
	
	if (krad_link->decoded_frames_buffer != NULL) {
		krad_ringbuffer_free ( krad_link->decoded_frames_buffer );
	}
	
	krad_ringbuffer_free ( krad_link->captured_frames_buffer );
	krad_ringbuffer_free ( krad_link->encoded_audio_ringbuffer );
	krad_ringbuffer_free ( krad_link->encoded_video_ringbuffer );

	free(krad_link->current_encoding_frame);
	free(krad_link->current_frame);

	free(krad_link);
}

krad_link_t *krad_link_create() {

	krad_link_t *krad_link;
	
	krad_link = calloc(1, sizeof(krad_link_t));
		
	krad_link->capture_buffer_frames = DEFAULT_CAPTURE_BUFFER_FRAMES;
	krad_link->encoding_buffer_frames = DEFAULT_ENCODING_BUFFER_FRAMES;
	
	krad_link->capture_width = DEFAULT_WIDTH;
	krad_link->capture_height = DEFAULT_HEIGHT;
	krad_link->capture_fps = DEFAULT_FPS;

	krad_link->composite_fps = krad_link->capture_fps;
	krad_link->encoding_fps = krad_link->capture_fps;
	krad_link->composite_width = krad_link->capture_width;
	krad_link->composite_height = krad_link->capture_height;
	krad_link->encoding_width = krad_link->capture_width;
	krad_link->encoding_height = krad_link->capture_height;
	krad_link->display_width = krad_link->capture_width;
	krad_link->display_height = krad_link->capture_height;
	
	strncpy(krad_link->device, DEFAULT_V4L2_DEVICE, sizeof(krad_link->device));
	strncpy(krad_link->alsa_capture_device, DEFAULT_ALSA_CAPTURE_DEVICE, sizeof(krad_link->alsa_capture_device));
	strncpy(krad_link->alsa_playback_device, DEFAULT_ALSA_PLAYBACK_DEVICE, sizeof(krad_link->alsa_playback_device));
	sprintf(krad_link->output, "%s/Videos/krad_link_%zu.webm", getenv ("HOME"), time(NULL));

	krad_link->krad_audio_api = ALSA;
	krad_link->operation_mode = CAPTURE;
	krad_link->interface_mode = WINDOW;
	krad_link->video_codec = VP8;
	krad_link->audio_codec = VORBIS;
	krad_link->video_source = V4L2;
	
	strncpy(krad_link->tone_preset, DEFAULT_TONE_PRESET, sizeof(krad_link->tone_preset));
	vpx_codec_enc_config_default(interface, &krad_link->vpx_encoder_config, 0);
	
	krad_link->vpx_encoder_config.rc_target_bitrate = DEFAULT_VPX_BITRATE;
	krad_link->vpx_encoder_config.kf_max_dist = krad_link->capture_fps * 3;	
	krad_link->vpx_encoder_config.g_w = krad_link->encoding_width;
	krad_link->vpx_encoder_config.g_h = krad_link->encoding_height;
	krad_link->vpx_encoder_config.g_threads = 4;
	krad_link->vpx_encoder_config.kf_mode = VPX_KF_AUTO;
	krad_link->vpx_encoder_config.rc_end_usage = VPX_VBR;
	
	return krad_link;
}

void krad_link_activate(krad_link_t *krad_link) {


	krad_link->composite_fps = krad_link->capture_fps;
	krad_link->encoding_fps = krad_link->capture_fps;
	krad_link->composite_width = krad_link->capture_width;
	krad_link->composite_height = krad_link->capture_height;
	krad_link->encoding_width = krad_link->capture_width;
	krad_link->encoding_height = krad_link->capture_height;
	krad_link->display_width = krad_link->capture_width;
	krad_link->display_height = krad_link->capture_height;

	if (krad_link->interface_mode == DAEMON) {
		daemonize();
	}

	sigemptyset(&krad_link->set);
	sigaddset(&krad_link->set, SIGINT);
	sigaddset(&krad_link->set, SIGTERM);
	//sigaddset(&krad_link->set, SIGQUIT);
	//sigaddset(&krad_link->set, SIGUSR1);
	if (pthread_sigmask(SIG_BLOCK, &krad_link->set, NULL) != 0) {
		printf("signal error!\n");
		exit(1);
	}
	pthread_create(&krad_link->signal_thread, NULL, signal_thread, (void *)krad_link);	
	pthread_detach(krad_link->signal_thread);
	
	if (krad_link->interface_mode == WINDOW) {

		krad_link->krad_opengl_display = krad_sdl_opengl_display_create(APPVERSION, krad_link->display_width, krad_link->display_height, 
															 		   krad_link->composite_width, krad_link->composite_height);
	}
	
	krad_link->composited_frame_byte_size = krad_link->composite_width * krad_link->composite_height * 4;
	krad_link->current_frame = calloc(1, krad_link->composited_frame_byte_size);
	krad_link->current_encoding_frame = calloc(1, krad_link->composited_frame_byte_size);


	if (krad_link->video_source == V4L2) {
		krad_link->captured_frame_converter = sws_getContext ( krad_link->capture_width, krad_link->capture_height, PIX_FMT_YUYV422, 
															  krad_link->composite_width, krad_link->composite_height, PIX_FMT_RGB32, 
															  SWS_BICUBIC, NULL, NULL, NULL);
	}
		  
	if (krad_link->video_source != NOVIDEO) {
		krad_link->encoding_frame_converter = sws_getContext ( krad_link->composite_width, krad_link->composite_height, PIX_FMT_RGB32, 
															  krad_link->encoding_width, krad_link->encoding_height, PIX_FMT_YUV420P, 
															  SWS_BICUBIC, NULL, NULL, NULL);
	}
	
	if (krad_link->interface_mode == WINDOW) {
															
		krad_link->display_frame_converter = sws_getContext ( krad_link->composite_width, krad_link->composite_height, PIX_FMT_RGB32, 
															 krad_link->display_width, krad_link->display_height, PIX_FMT_RGB32, 
															 SWS_BICUBIC, NULL, NULL, NULL);
	}
		
	krad_link->captured_frames_buffer = krad_ringbuffer_create (krad_link->composited_frame_byte_size * krad_link->capture_buffer_frames);
	krad_link->composited_frames_buffer = krad_ringbuffer_create (krad_link->composited_frame_byte_size * krad_link->encoding_buffer_frames);
	krad_link->encoded_audio_ringbuffer = krad_ringbuffer_create (2000000);
	krad_link->encoded_video_ringbuffer = krad_ringbuffer_create (2000000);

	krad_link->krad_gui = kradgui_create_with_internal_surface(krad_link->composite_width, krad_link->composite_height);

	if (krad_link->bug[0] != '\0') {
		kradgui_set_bug (krad_link->krad_gui, krad_link->bug, krad_link->bug_x, krad_link->bug_y);
	}

	if (krad_link->verbose) {	
	//	krad_link->krad_gui->update_drawtime = 1;
	//	krad_link->krad_gui->print_drawtime = 1;
	}


	if (krad_link->operation_mode == CAPTURE) {

		krad_link->encoding = 1;

		if (krad_link->video_source == V4L2) {
			krad_link->krad_gui->clear = 0;
		}
	
		if (krad_link->video_source == TEST) {
			kradgui_test_screen(krad_link->krad_gui, "Krad Test");
		}
		
		if (krad_link->video_source == V4L2) {

			krad_link->capturing = 1;
			pthread_create(&krad_link->video_capture_thread, NULL, video_capture_thread, (void *)krad_link);
		}
	
		if (krad_link->interface_mode == WINDOW) {
			krad_link->render_meters = 1;
		}
	
		if (krad_link->video_source != NOVIDEO) {
			pthread_create(&krad_link->video_encoding_thread, NULL, video_encoding_thread, (void *)krad_link);
		}
	
		if (krad_link->krad_audio_api != NOAUDIO) {
			pthread_create(&krad_link->audio_encoding_thread, NULL, audio_encoding_thread, (void *)krad_link);
		}
	
		pthread_create(&krad_link->ebml_output_thread, NULL, ebml_output_thread, (void *)krad_link);	

	}
	
	if (krad_link->operation_mode == RECEIVE) {

		krad_link->decoded_frames_buffer = krad_ringbuffer_create (krad_link->composited_frame_byte_size * krad_link->encoding_buffer_frames);
		
		pthread_create(&krad_link->ebml_input_thread, NULL, ebml_input_thread, (void *)krad_link);	
		
		if (krad_link->interface_mode == WINDOW) {
			pthread_create(&krad_link->video_decoding_thread, NULL, video_decoding_thread, (void *)krad_link);
		}
	
		if (krad_link->krad_audio_api != NOAUDIO) {
			pthread_create(&krad_link->audio_decoding_thread, NULL, audio_decoding_thread, (void *)krad_link);
		}

	}
	
	dbg("mem use approx %d MB\n", (krad_link->composited_frame_byte_size * (krad_link->capture_buffer_frames + krad_link->encoding_buffer_frames + 4)) / 1000 / 1000);


}
