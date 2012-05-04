#include "krad_link.h"

extern int verbose;

void krad_link_activate(krad_link_t *krad_link);

void dbg (char* format, ...) {

	va_list args;

	if (verbose) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
	}
}

char *krad_codec_to_string(krad_codec_t codec) {

	switch (codec) {
		case VORBIS:
			return "Vorbix";
		case FLAC:
			return "FLAC";
		case OPUS:
			return "Opus";
		case VP8:
			return "VP8";
		case THEORA:
			return "Theora";
		case DIRAC:
			return "Dirac";
		default:
			return "No Codec";
	}
}

void *video_capture_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_vidcap", 0, 0, 0);

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

	prctl (PR_SET_NAME, (unsigned long) "kradlink_videnc", 0, 0, 0);

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

	krad_link->krad_vpx_encoder->quality = (1000 / krad_link->encoding_fps) * 1000;

	dbg("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
	
	while ((krad_link->encoding == 1) || (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= krad_link->composited_frame_byte_size)) {

		if (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= krad_link->composited_frame_byte_size) {
	
			krad_ringbuffer_read(krad_link->composited_frames_buffer, 
								 (char *)krad_link->current_encoding_frame,
								 krad_link->composited_frame_byte_size );
								 
						
						
			if (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= (krad_link->composited_frame_byte_size * (krad_link->encoding_buffer_frames / 2)) ) {
			
				krad_link->krad_vpx_encoder->quality = ((1000 / krad_link->encoding_fps) * 1000) / 2LU;
				dbg("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
			}
						
			if (krad_ringbuffer_read_space(krad_link->composited_frames_buffer) >= (krad_link->composited_frame_byte_size * (krad_link->encoding_buffer_frames / 4)) ) {
			
				krad_link->krad_vpx_encoder->quality = ((1000 / krad_link->encoding_fps) * 1000) / 4LU;
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
		
			if (krad_link->krad_vpx_encoder->quality != (1000 / krad_link->encoding_fps) * 1000) {
				krad_link->krad_vpx_encoder->quality = (1000 / krad_link->encoding_fps) * 1000;
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


void krad_link_audio_samples_callback (int frames, void *userdata, float **samples) {

	krad_link_t *krad_link = (krad_link_t *)userdata;
	
	if (krad_link->operation_mode == RECEIVE) {
		if ((krad_ringbuffer_read_space (krad_link->audio_output_ringbuffer[0]) >= frames * 4) && 
			(krad_ringbuffer_read_space (krad_link->audio_output_ringbuffer[1]) >= frames * 4)) {
			krad_ringbuffer_read (krad_link->audio_output_ringbuffer[0], (char *)samples[0], frames * 4);
			krad_ringbuffer_read (krad_link->audio_output_ringbuffer[1], (char *)samples[1], frames * 4);
		} else {
			memset(samples[0], '0', frames * 4);
			memset(samples[1], '0', frames * 4);
		}
	}

	if (krad_link->operation_mode == TRANSMIT) {
		krad_link->audio_frames_captured += frames;
		krad_ringbuffer_write (krad_link->audio_input_ringbuffer[0], (char *)samples[0], frames * 4);
		krad_ringbuffer_write (krad_link->audio_input_ringbuffer[1], (char *)samples[1], frames * 4);
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		krad_link->audio_frames_captured += frames;
		krad_ringbuffer_read (krad_link->audio_capture_ringbuffer[0], (char *)samples[0], frames * 4);
		krad_ringbuffer_read (krad_link->audio_capture_ringbuffer[1], (char *)samples[1], frames * 4);
	}	
	
	if (krad_link->capture_audio == 2) {
		krad_link->capture_audio = 3;
	}
	
	
}

void *audio_encoding_thread (void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_audenc", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	int c;
	int s;
	int bytes;
	int frames;
	float *samples[KRAD_MIXER_MAX_CHANNELS];
	float *interleaved_samples;
	unsigned char *buffer;
	ogg_packet *op;
	int framecnt;

	krad_mixer_portgroup_t *mixer_portgroup;

	dbg("Audio encoding thread starting\n");
	
	krad_link->audio_channels = 2;	
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		krad_link->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}

	interleaved_samples = calloc(1, 1000000);
	buffer = calloc(1, 1000000);
	
	mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, OUTPUT, 2, 
												   krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);		
		
	switch (krad_link->audio_codec) {
		case VORBIS:
			printf("Vorbis quality is %f\n", krad_link->vorbis_quality);
			krad_link->krad_vorbis = krad_vorbis_encoder_create(krad_link->audio_channels, 48000, krad_link->vorbis_quality);
			framecnt = 1024;
			break;
		case FLAC:
		//	krad_link->krad_flac = krad_flac_encoder_create(krad_link->audio_channels, krad_link->krad_audio->sample_rate, 24);
			framecnt = 4096;
			break;
		case OPUS:
			//krad_link->krad_opus = kradopus_encoder_create(krad_link->krad_audio->sample_rate, krad_link->audio_channels, 192000, OPUS_APPLICATION_AUDIO);
			framecnt = 960;
			break;
		default:
			printf("unknown audio codec\n");
			exit(1);
	}

	//kradaudio_set_process_callback(krad_link->krad_audio, krad_link_audio_callback, krad_link);
	
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
	
			usleep(5000);
	
			if (krad_link->encoding == 3) {
				break;
			}
	
		}
		
		if ((krad_link->encoding == 3) && (krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[1]) < framecnt * 4)) {
				break;
		}
		
	}

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, mixer_portgroup);
	
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

void *stream_output_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_stmout", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	unsigned char *packet;
	int packet_size;
	int keyframe;
	int frames;
	char keyframe_char[1];
	
	int video_frames_muxed;
	int audio_frames_muxed;
	int audio_frames_per_video_frame;
	
	audio_frames_per_video_frame = 0;

	dbg("Output/Muxing thread starting\n");
	
	audio_frames_muxed = 0;
	video_frames_muxed = 0;

	if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
	//	while (krad_link->krad_audio == NULL) {
	//		usleep(5000);
	//	}

		if (krad_link->audio_codec == OPUS) {
			audio_frames_per_video_frame = 48000 / krad_link->capture_fps;
		} else {
			audio_frames_per_video_frame = 48000 / krad_link->capture_fps;
	//		audio_frames_per_video_frame = krad_link->krad_audio->sample_rate / krad_link->capture_fps;
		}
	}

	packet = malloc(2000000);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad_ebml = krad_ebml_open_stream(krad_link->host, krad_link->tcp_port, krad_link->mount, krad_link->password);
	} else {
		printf("Outputing to file: %s\n", krad_link->output);
		krad_link->krad_ebml = krad_ebml_open_file(krad_link->output, KRAD_EBML_IO_WRITEONLY);
	}
	
	if (krad_link->audio_codec != NOCODEC) {
		while ( krad_link->audio_encoder_ready != 1) {
			usleep(10000);
		}
	}
	
	if (((krad_link->audio_codec == VORBIS) || (krad_link->audio_codec == NOCODEC)) && 
		((krad_link->video_codec == VP8) || (krad_link->video_codec == NOCODEC))) {
		
		krad_ebml_header(krad_link->krad_ebml, "webm", APPVERSION);
	} else {
		krad_ebml_header(krad_link->krad_ebml, "matroska", APPVERSION);
	}
	
	if (krad_link->video_source != NOVIDEO) {
	
		krad_link->video_track = krad_ebml_add_video_track(krad_link->krad_ebml, "V_VP8", 30000, 1001,
											 			 krad_link->encoding_width, krad_link->encoding_height);
	}	
	
	if (krad_link->audio_codec != NOCODEC) {
	
		switch (krad_link->audio_codec) {
			case VORBIS:
				krad_link->audio_track = krad_ebml_add_audio_track(krad_link->krad_ebml, "A_VORBIS", 48000, krad_link->audio_channels, krad_link->krad_vorbis->header, 
																 krad_link->krad_vorbis->headerpos);
				break;
			case FLAC:
				//krad_link->audio_track = krad_ebml_add_audio_track(krad_link->krad_ebml, "A_FLAC", krad_link->krad_audio->sample_rate, krad_link->audio_channels, (unsigned char *)krad_link->krad_flac->min_header, FLAC_MINIMAL_HEADER_SIZE);
				break;
			case OPUS:
				krad_link->audio_track = krad_ebml_add_audio_track(krad_link->krad_ebml, "A_OPUS", 48000, krad_link->audio_channels, krad_link->krad_opus->header_data, krad_link->krad_opus->header_data_size);
				break;
			default:
				printf("Unknown audio codec\n");
				exit(1);
		}
	
	}
	
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
	

				krad_ebml_add_video(krad_link->krad_ebml, krad_link->video_track, packet, packet_size, keyframe);

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

				krad_ebml_add_audio(krad_link->krad_ebml, krad_link->audio_track, packet, packet_size, frames);

				audio_frames_muxed += frames;
				
				//printf("ebml muxed audio frames: %d\n\n", audio_frames_muxed);
				
			} else {
			
			
				//printf("ebml yasdsad rames: %d\n", krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer));
			
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
		
	
		//krad_ebml_write_tag (krad_link->krad_ebml, "test tag 1", "monkey 123");
	
	}

	krad_ebml_destroy(krad_link->krad_ebml);
	
	free(packet);

	dbg("Output/Muxing thread exiting\n");
	
	return NULL;
	
}


void *udp_output_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_udpout", 0, 0, 0);

	dbg("UDP Output thread starting\n");

	krad_link_t *krad_link = (krad_link_t *)arg;

	unsigned char *buffer;
	int count;
	int packet_size;
	int frames;
	
	
	count = 0;
	
	buffer = malloc(250000);
	
	krad_link->krad_slicer = krad_slicer_create ();
	
	while ( krad_link->encoding ) {
	
		if (krad_link->audio_codec != NOCODEC) {
		
			if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) >= 4)) {

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding != 4)) {
					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding == 4)) {
					break;
				}
			
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)buffer, packet_size);

				krad_slicer_sendto (krad_link->krad_slicer, buffer, packet_size, 1, krad_link->host, krad_link->udp_send_port);
				count++;
				
			} else {
				if (krad_link->encoding == 4) {
					break;
				}
				usleep(5000);
			}
		}
	}
	
	krad_slicer_destroy (krad_link->krad_slicer);
	
	free (buffer);

	dbg("UDP Output thread exiting\n");
	
	return NULL;

}

void krad_link_display(krad_link_t *krad_link) {

	if ((krad_link->composite_width == krad_link->display_width) && (krad_link->composite_height == krad_link->display_height)) {
		memcpy ( krad_link->krad_x11->pixels, krad_link->krad_gui->data, krad_link->krad_gui->bytes );
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

		dst[0] = krad_link->krad_x11->pixels;
		
		sws_scale(krad_link->display_frame_converter, rgb_arr, rgb1_stride_arr, 0, krad_link->display_height, dst, rgb_stride_arr);

	}

	krad_x11_glx_render (krad_link->krad_x11);

}

void *krad_link_run_thread (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_link_activate ( krad_link );

	if (krad_link->operation_mode == RECEIVE) {
	
		while (!krad_link->destroy) {
			usleep(40000);
		}
	
	} else {

		while (!krad_link->destroy) {

			if (krad_link->capturing) {
				while (krad_ringbuffer_read_space (krad_link->captured_frames_buffer) < krad_link->composited_frame_byte_size) {
					usleep(5000);
				}
			}

			if (krad_link->operation_mode == CAPTURE) {
				krad_compositor_process (krad_link->krad_linker->krad_radio->krad_compositor);
			}

			if ((!krad_link->capturing) || (krad_link->video_source == X11)) {

				//FIXME this needs to be calculated with frame duration in nanoseconds
			
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

	return NULL;

}

void krad_link_run (krad_link_t *krad_link) {

	pthread_create (&krad_link->main_thread, NULL, krad_link_run_thread, (void *)krad_link);
	pthread_detach (krad_link->main_thread);
}

void *stream_input_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_stmin", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("Input/Demuxing thread starting\n");

	unsigned char *buffer;
	unsigned char *header_buffer;
	int codec_bytes;
	int video_packets;
	int audio_packets;
	int current_track;
	krad_codec_t track_codecs[10];
	krad_codec_t nocodec;	
	int packet_size;
	uint64_t packet_timecode;
	int header_size;
	int h;
	int total_header_size;
	int writeheaders;
	
	nocodec = NOCODEC;
	packet_size = 0;
	codec_bytes = 0;	
	header_size = 0;
	total_header_size = 0;	
	writeheaders = 0;
	video_packets = 0;
	audio_packets = 0;
	current_track = -1;

	header_buffer = malloc(4096 * 512);
	buffer = malloc(4096 * 512);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad_container = krad_container_open_stream(krad_link->host, krad_link->tcp_port, krad_link->mount, NULL);
	} else {
		krad_link->krad_container = krad_container_open_file(krad_link->input, KRAD_IO_READONLY);
	}
	
	while (!krad_link->destroy) {
		
		writeheaders = 0;
		total_header_size = 0;
		header_size = 0;

		packet_size = krad_container_read_packet ( krad_link->krad_container, &current_track, &packet_timecode, buffer);
		//printf("packet track %d timecode: %zu\n", current_track, packet_timecode);
		if ((packet_size <= 0) && (packet_timecode == 0) && ((video_packets + audio_packets) > 20))  {
			printf("\nstream input thread packet size was: %d\n", packet_size);
			break;
		}
		
		if (krad_container_track_changed(krad_link->krad_container, current_track)) {
			printf("track %d changed! status is %d header count is %d\n", current_track, krad_container_track_active(krad_link->krad_container, current_track), krad_container_track_header_count(krad_link->krad_container, current_track));
			
			track_codecs[current_track] = krad_container_track_codec(krad_link->krad_container, current_track);
			
			if (track_codecs[current_track] == NOCODEC) {
				continue;
			}
			
			writeheaders = 1;
			
			for (h = 0; h < krad_container_track_header_count(krad_link->krad_container, current_track); h++) {
				printf("header %d is %d bytes\n", h, krad_container_track_header_size(krad_link->krad_container, current_track, h));
				total_header_size += krad_container_track_header_size(krad_link->krad_container, current_track, h);
			}

			
		}
		
		if ((track_codecs[current_track] == VP8) || (track_codecs[current_track] == THEORA) || (track_codecs[current_track] == DIRAC)) {

			video_packets++;

			if (krad_link->interface_mode == WINDOW) {

				while ((krad_ringbuffer_write_space(krad_link->encoded_video_ringbuffer) < packet_size + 4 + total_header_size + 4 + 4 + 8) && (!krad_link->destroy)) {
					usleep(10000);
				}
				
				if (writeheaders == 1) {
					krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&nocodec, 4);
					krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&track_codecs[current_track], 4);
					for (h = 0; h < krad_container_track_header_count(krad_link->krad_container, current_track); h++) {
						header_size = krad_container_track_header_size(krad_link->krad_container, current_track, h);
						krad_container_read_track_header(krad_link->krad_container, header_buffer, current_track, h);
						krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&header_size, 4);
						krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)header_buffer, header_size);
						codec_bytes += packet_size;
					}
				} else {
					krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&track_codecs[current_track], 4);
				}
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_timecode, 8);
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)buffer, packet_size);
				codec_bytes += packet_size;
			}
		}
		
		if ((track_codecs[current_track] == VORBIS) || (track_codecs[current_track] == OPUS) || (track_codecs[current_track] == FLAC)) {

			audio_packets++;

			if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			
				while ((krad_ringbuffer_write_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4 + total_header_size + 4 + 4) && (!krad_link->destroy)) {
					usleep(10000);
				}
				
				if (writeheaders == 1) {
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&nocodec, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&track_codecs[current_track], 4);
					for (h = 0; h < krad_container_track_header_count(krad_link->krad_container, current_track); h++) {
						header_size = krad_container_track_header_size(krad_link->krad_container, current_track, h);
						krad_container_read_track_header(krad_link->krad_container, header_buffer, current_track, h);
						krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&header_size, 4);
						krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)header_buffer, header_size);
						codec_bytes += packet_size;
					}
				} else {
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&track_codecs[current_track], 4);
				}
				

				krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)buffer, packet_size);
				codec_bytes += packet_size;
			}
			
		}

		//if (krad_link->krad_audio != NULL) {
		//	dbg("Input ACodec %s :: Video Packets %d Audio Packets: %d Codec bytes Read %d\r", krad_codec_to_string(track_codecs[current_track]), video_packets, audio_packets, codec_bytes);
		//	//dbg("Input Timecode: %6.1f :: Video Packets %d Audio Packets: %d Codec bytes Read %d\r", krad_link->krad_ebml->current_timecode / 1000.0, video_packets, audio_packets, codec_bytes);
		//	fflush(stdout);
		//}

	}


	
	printf("\n");
	dbg("Input/Demuxing thread exiting\n");
	
	krad_container_destroy(krad_link->krad_container);
	
	free(buffer);
	free(header_buffer);
	return NULL;

}

void *udp_input_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_udpin", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("UDP Input thread starting\n");

	int sd;
	int ret;
	int rsize;
	unsigned char *buffer;
	unsigned char *packet_buffer;
	struct sockaddr_in local_address;
	struct sockaddr_in remote_address;
	int nocodec;
	int opus_codec;
	int packets;
	
	packets = 0;
	rsize = sizeof(remote_address);
	opus_codec = OPUS;
	nocodec = NOCODEC;
	buffer = calloc (1, 2000);
	packet_buffer = calloc (1, 500000);
	sd = socket (AF_INET, SOCK_DGRAM, 0);

	krad_link->krad_rebuilder = krad_rebuilder_create ();

	memset((char *) &local_address, 0, sizeof(local_address));
	local_address.sin_family = AF_INET;
	local_address.sin_port = htons (krad_link->udp_recv_port);
	local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1 ) {
		printf("bind error\n");
		exit(1);
	}
	
	//kludge to get header
	krad_opus_t *opus_temp;
	unsigned char opus_header[256];
	int opus_header_size;
	
	opus_temp = kradopus_encoder_create(44100.0, 2, 192000, OPUS_APPLICATION_AUDIO);
	opus_header_size = opus_temp->header_data_size;
	memcpy (opus_header, opus_temp->header_data, opus_header_size);
	kradopus_encoder_destroy(opus_temp);
	
	printf("placing opus header size is %d\n", opus_header_size);
	
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&nocodec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_codec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_header_size, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)opus_header, opus_header_size);

	while (!krad_link->destroy) {
	
		ret = recvfrom(sd, buffer, 2000, 0, (struct sockaddr *)&remote_address, (socklen_t *)&rsize);
		
		if (ret == -1) {
			printf("failed recvin udp\n");
			krad_link->destroy = 1;
			continue;
		}
		
		//printf("Received packet from %s:%d\n", 
		//		inet_ntoa(remote_address.sin_addr), ntohs(remote_address.sin_port));


		krad_rebuilder_write (krad_link->krad_rebuilder, buffer, ret);

		ret = krad_rebuilder_read_packet (krad_link->krad_rebuilder, packet_buffer, 1);
		
		if (ret != 0) {
			//printf("read a packet with %d bytes\n", ret);

			if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			
				while ((krad_ringbuffer_write_space(krad_link->encoded_audio_ringbuffer) < ret + 4 + 4) && (!krad_link->destroy)) {
					usleep(10000);
				}
				
				if (packets > 0) {
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_codec, 4);
				}
				krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&ret, 4);
				krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)packet_buffer, ret);
				packets++;
			}

		}

	}

	krad_rebuilder_destroy (krad_link->krad_rebuilder);
	close (sd);
	free (buffer);
	free (packet_buffer);
	dbg("UDP Input thread exiting\n");
	
	return NULL;

}

void krad_link_yuv_to_rgb(krad_link_t *krad_link, unsigned char *output, unsigned char *y, int ys, unsigned char *u, int us, unsigned char *v, int vs, int height) {

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

	sws_scale(krad_link->decoded_frame_converter, yv12_arr, yv12_str_arr, 0, height, dst, rgb_stride_arr);

}

void *video_decoding_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_viddec", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("Video decoding thread starting\n");

	int bytes;
	unsigned char *buffer;	
	unsigned char *converted_buffer;
	int h;
	unsigned char *header[3];
	int header_len[3];
	uint64_t timecode;
	uint64_t timecode2;

	for (h = 0; h < 3; h++) {
		header[h] = malloc(100000);
		header_len[h] = 0;
	}

	bytes = 0;
	buffer = malloc(3000000);
	converted_buffer = malloc(12000000);
	
	krad_link->last_video_codec = NOCODEC;
	krad_link->video_codec = NOCODEC;
	
	while (!krad_link->destroy) {


		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(10000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&krad_link->video_codec, 4);
		if ((krad_link->last_video_codec != krad_link->video_codec) || (krad_link->video_codec == NOCODEC)) {
			dbg("\nvideo codec is %d\n", krad_link->video_codec);
			if (krad_link->last_video_codec != NOCODEC)	{
				if (krad_link->last_video_codec == VP8) {
					krad_vpx_decoder_destroy (krad_link->krad_vpx_decoder);
					krad_link->krad_vpx_decoder = NULL;
				}
				if (krad_link->last_video_codec == THEORA) {
					krad_theora_decoder_destroy (krad_link->krad_theora_decoder);
					krad_link->krad_theora_decoder = NULL;
				}			
				if (krad_link->last_video_codec == DIRAC) {
					krad_dirac_decoder_destroy(krad_link->krad_dirac);
					krad_link->krad_dirac = NULL;
				}
				
				if (krad_link->decoded_frame_converter != NULL) {
					sws_freeContext ( krad_link->decoded_frame_converter );
				}
				
			}
	
			if (krad_link->video_codec == NOCODEC) {
				krad_link->last_video_codec = krad_link->video_codec;
				continue;
			}
	
			if (krad_link->video_codec == VP8) {
				krad_link->krad_vpx_decoder = krad_vpx_decoder_create();
			}
	
			if (krad_link->video_codec == THEORA) {
				
				for (h = 0; h < 3; h++) {

					while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4) && (!krad_link->destroy)) {
						usleep(10000);
					}
	
					krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&header_len[h], 4);
		
					while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < header_len[h]) && (!krad_link->destroy)) {
						usleep(10000);
					}
		
					krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)header[h], header_len[h]);
				}

				dbg("\n\nTheora Header byte sizes: %d %d %d\n", header_len[0], header_len[1], header_len[2]);
				krad_link->krad_theora_decoder = krad_theora_decoder_create(header[0], header_len[0], header[1], header_len[1], header[2], header_len[2]);
			}
	
			if (krad_link->video_codec == DIRAC) {
				krad_link->krad_dirac = krad_dirac_decoder_create();
			}
		}

		krad_link->last_video_codec = krad_link->video_codec;
	
		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 12) && (!krad_link->destroy)) {
			usleep(10000);
		}
	
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&timecode, 8);
	
		//printf("\nframe timecode: %zu\n", timecode);
	
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < bytes) && (!krad_link->destroy)) {
			usleep(10000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)buffer, bytes);
		
		if (krad_link->video_codec == THEORA) {
		
			krad_theora_decoder_decode(krad_link->krad_theora_decoder, buffer, bytes);

			if (krad_link->decoded_frame_converter == NULL) {
				krad_link->decoded_frame_converter = sws_getContext ( krad_link->krad_theora_decoder->width, krad_link->krad_theora_decoder->height, PIX_FMT_YUV420P,
															  krad_link->display_width, krad_link->display_height, PIX_FMT_RGB32, 
															  SWS_BICUBIC, NULL, NULL, NULL);
		
			}

			krad_link_yuv_to_rgb(krad_link, converted_buffer, krad_link->krad_theora_decoder->ycbcr[0].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[0].stride), 
								 krad_link->krad_theora_decoder->ycbcr[0].stride, krad_link->krad_theora_decoder->ycbcr[1].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[1].stride), 
								 krad_link->krad_theora_decoder->ycbcr[1].stride, krad_link->krad_theora_decoder->ycbcr[2].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[2].stride), 
								 krad_link->krad_theora_decoder->ycbcr[2].stride, krad_link->krad_theora_decoder->height);

			while ((krad_ringbuffer_write_space(krad_link->decoded_frames_buffer) < krad_link->composited_frame_byte_size + 8) && (!krad_link->destroy)) {
				usleep(10000);
			}
			
			krad_theora_decoder_timecode(krad_link->krad_theora_decoder, &timecode2);
			
			///printf("\n\ntimecode1: %zu timecode2: %zu\n\n", timecode, timecode2);
			
			timecode = timecode2;
			
			krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)&timecode, 8);
			krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)converted_buffer, krad_link->composited_frame_byte_size);

		}
			
		if (krad_link->video_codec == VP8) {
			krad_vpx_decoder_decode(krad_link->krad_vpx_decoder, buffer, bytes);
				
			if (krad_link->krad_vpx_decoder->img != NULL) {

				if (krad_link->decoded_frame_converter == NULL) {

					krad_link->decoded_frame_converter = sws_getContext ( krad_link->krad_vpx_decoder->width, krad_link->krad_vpx_decoder->height, PIX_FMT_YUV420P,
																  krad_link->display_width, krad_link->display_height, PIX_FMT_RGB32, 
																  SWS_BICUBIC, NULL, NULL, NULL);

				}

				krad_link_yuv_to_rgb(krad_link, converted_buffer, krad_link->krad_vpx_decoder->img->planes[0], 
									 krad_link->krad_vpx_decoder->img->stride[0], krad_link->krad_vpx_decoder->img->planes[1], 
									 krad_link->krad_vpx_decoder->img->stride[1], krad_link->krad_vpx_decoder->img->planes[2], 
									 krad_link->krad_vpx_decoder->img->stride[2], krad_link->krad_vpx_decoder->height);

				while ((krad_ringbuffer_write_space(krad_link->decoded_frames_buffer) < krad_link->composited_frame_byte_size + 8) && (!krad_link->destroy)) {
					usleep(10000);
				}
				krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)&timecode, 8);
				krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)converted_buffer, krad_link->composited_frame_byte_size);
			}
		}
			

		if (krad_link->video_codec == DIRAC) {

			krad_dirac_decode(krad_link->krad_dirac, buffer, bytes);

			if ((krad_link->krad_dirac->format != NULL) && (krad_link->krad_dirac->format_set == false)) {

				switch (krad_link->krad_dirac->format->chroma_format) {
					case SCHRO_CHROMA_444:
						//krad_sdl_opengl_display_set_input_format(krad_link->krad_opengl_display, PIX_FMT_YUV444P);
					case SCHRO_CHROMA_422:
						//krad_sdl_opengl_display_set_input_format(krad_link->krad_opengl_display, PIX_FMT_YUV422P);
					case SCHRO_CHROMA_420:
						// default
						//krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display, PIX_FMT_YUV420P);
						break;
				}

				krad_link->krad_dirac->format_set = true;				
			}

			if (krad_link->krad_dirac->frame != NULL) {
			
				if (krad_link->decoded_frame_converter == NULL) {

					krad_link->decoded_frame_converter = sws_getContext ( krad_link->krad_dirac->frame->width, krad_link->krad_dirac->frame->height, PIX_FMT_YUV420P,
																  krad_link->display_width, krad_link->display_height, PIX_FMT_RGB32, 
																  SWS_BICUBIC, NULL, NULL, NULL);

				}

				krad_link_yuv_to_rgb(krad_link, converted_buffer, krad_link->krad_dirac->frame->components[0].data, 
									 krad_link->krad_dirac->frame->components[0].stride, krad_link->krad_dirac->frame->components[1].data, 
									 krad_link->krad_dirac->frame->components[1].stride, krad_link->krad_dirac->frame->components[2].data,
									 krad_link->krad_dirac->frame->components[2].stride, krad_link->krad_dirac->frame->height);

				while ((krad_ringbuffer_write_space(krad_link->decoded_frames_buffer) < krad_link->composited_frame_byte_size + 8) && (!krad_link->destroy)) {
					usleep(10000);
				}
				krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)&timecode, 8);
				krad_ringbuffer_write(krad_link->decoded_frames_buffer, (char *)converted_buffer, krad_link->composited_frame_byte_size);


			}
		}
	}


	free (converted_buffer);
	free (buffer);
	for (h = 0; h < 3; h++) {
		free(header[h]);
	}
	dbg("Video decoding thread exiting\n");

	return NULL;

}

void *audio_decoding_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_auddec", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	dbg("Audio decoding thread starting\n");

	int c;
	int h;
	int bytes;
	unsigned char *buffer;
	unsigned char *codec2_buffer;
	int codec2_bytes;
	unsigned char *header[3];
	int header_len[3];
	float *audio;
	float *samples[KRAD_MIXER_MAX_CHANNELS];
	int audio_frames;
	
	krad_mixer_portgroup_t *mixer_portgroup;	
	
	krad_link->audio_channels = 2;

	for (h = 0; h < 3; h++) {
		header[h] = malloc(100000);
		header_len[h] = 0;
	}

	for (c = 0; c < krad_link->audio_channels; c++) {
		krad_link->audio_output_ringbuffer[c] = krad_ringbuffer_create (4000000);	
		samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}
	
	buffer = malloc(2000000);
	audio = calloc(1, 8192 * 4 * 4);

	codec2_bytes = 0;
	codec2_buffer = calloc(1, 8192 * 4 * 4);
	
	krad_link->last_audio_codec = NOCODEC;
	krad_link->audio_codec = NOCODEC;
	
	
	mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, INPUT, 2, 
												   krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);
	
	
	if (krad_link->udp_mode == 1) {
		//usleep(150000);
	}
	
	while (!krad_link->destroy) {


		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(5000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&krad_link->audio_codec, 4);
		if ((krad_link->last_audio_codec != krad_link->audio_codec) || (krad_link->audio_codec == NOCODEC)) {
			dbg("\n\naudio codec is %d\n", krad_link->audio_codec);
			if (krad_link->last_audio_codec != NOCODEC)	{
				if (krad_link->last_audio_codec == FLAC) {
					krad_flac_decoder_destroy (krad_link->krad_flac);
					krad_link->krad_flac = NULL;
				}
				if (krad_link->last_audio_codec == VORBIS) {
					krad_vorbis_decoder_destroy (krad_link->krad_vorbis);
					krad_link->krad_vorbis = NULL;
				}			
				if (krad_link->last_audio_codec == OPUS) {
					kradopus_decoder_destroy(krad_link->krad_opus);
					krad_link->krad_opus = NULL;
				}
			}
	
			if (krad_link->audio_codec == NOCODEC) {
				krad_link->last_audio_codec = krad_link->audio_codec;
				continue;
			}
	
			if (krad_link->audio_codec == FLAC) {
				krad_link->krad_flac = krad_flac_decoder_create();
				for (h = 0; h < 1; h++) {

					while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
						usleep(10000);
					}
	
					krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&header_len[h], 4);
		
					while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < header_len[h]) && (!krad_link->destroy)) {
						usleep(10000);
					}
		
					krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)header[h], header_len[h]);
				}
				krad_flac_decode(krad_link->krad_flac, header[0], header_len[0], NULL);
			}
	
			if (krad_link->audio_codec == VORBIS) {
				
				for (h = 0; h < 3; h++) {

					while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
						usleep(10000);
					}
	
					krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&header_len[h], 4);
		
					while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < header_len[h]) && (!krad_link->destroy)) {
						usleep(10000);
					}
		
					krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)header[h], header_len[h]);
				}

				dbg("\n\nVorbis Header byte sizes: %d %d %d\n", header_len[0], header_len[1], header_len[2]);
				krad_link->krad_vorbis = krad_vorbis_decoder_create(header[0], header_len[0], header[1], header_len[1], header[2], header_len[2]);



				//kradaudio_set_process_callback(krad_link->krad_audio, krad_link_audio_callback, krad_link);
			}
	
			if (krad_link->audio_codec == OPUS) {
			
				for (h = 0; h < 1; h++) {

					while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
						usleep(5000);
					}
	
					krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&header_len[h], 4);
		
					while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < header_len[h]) && (!krad_link->destroy)) {
						usleep(5000);
					}
		
					krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)header[h], header_len[h]);
				}
			
				dbg("Opus Header size: %d\n", header_len[0]);
				//krad_link->krad_opus = kradopus_decoder_create(header[0], header_len[0], krad_link->krad_audio->sample_rate);
			}
		}

		krad_link->last_audio_codec = krad_link->audio_codec;
	
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(5000);
		}
	
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < bytes) && (!krad_link->destroy)) {
			usleep(5000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
		
		if (krad_link->audio_codec == VORBIS) {
			krad_vorbis_decoder_decode(krad_link->krad_vorbis, buffer, bytes);
			
			int len = 1;
			
			while (len ) {
			
				len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 0, (char *)samples[0], 512);

				if (len) {
				
					while (krad_ringbuffer_write_space(krad_link->audio_output_ringbuffer[0]) < len) {
						//printf("wait!\n");
						usleep(25000);
					}
				
					krad_ringbuffer_write (krad_link->audio_output_ringbuffer[0], (char *)samples[0], len);
				}

				len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 1, (char *)samples[1], 512);

				if (len) {

					while (krad_ringbuffer_write_space(krad_link->audio_output_ringbuffer[1]) < len) {
						//printf("wait!\n");
						usleep(25000);
					}

					krad_ringbuffer_write (krad_link->audio_output_ringbuffer[1], (char *)samples[1], len);
				}
			
			}
		}
			
		if (krad_link->audio_codec == FLAC) {

			audio_frames = krad_flac_decode(krad_link->krad_flac, buffer, bytes, samples);
			
			for (c = 0; c < krad_link->audio_channels; c++) {
				//kradaudio_write (krad_link->krad_audio, c, (char *)samples[c], audio_frames * 4 );
			}
		}
			
		if (krad_link->audio_codec == OPUS) {
		
			//kradopus_decoder_set_speed(krad_link->krad_opus, 100);
			kradopus_write_opus(krad_link->krad_opus, buffer, bytes);

			int c;
			int returned_samples = 0;
			int codec2_test = 0;
			bytes = -1;			
			while (bytes != 0) {
				for (c = 0; c < 2; c++) {
					bytes = kradopus_read_audio(krad_link->krad_opus, c + 1, (char *)audio, 960 * 4);
					if (bytes) {
						//printf("\nAudio data! %d samplers\n", bytes / 4);
						
						if ((bytes / 4) != 960) {
							printf("uh oh crazyto\n");
							exit(1);
						}
						if (codec2_test == 1) {
							//krad_smash (audio, 960);
							if (c == 0) {
								codec2_bytes = krad_codec2_encode(krad_link->krad_codec2_encoder, audio, 960, codec2_buffer);
								memset(audio, '0', 960 * 4);
								returned_samples = krad_codec2_decode(krad_link->krad_codec2_decoder, codec2_buffer, codec2_bytes, audio);
								//kradaudio_write (krad_link->krad_audio, c, (char *)audio, returned_samples * 4 );
							} else {
								memset(audio, '0', 960 * 4);
								//kradaudio_write (krad_link->krad_audio, c, (char *)audio, returned_samples * 4 );
							}
						} else {
							//kradaudio_write (krad_link->krad_audio, c, (char *)audio, bytes );
						}
					}
				}
			}
		}
	}
	
	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, mixer_portgroup);

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
	free(codec2_buffer);
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		free(krad_link->samples[c]);
		free(samples[c]);
		krad_ringbuffer_free ( krad_link->audio_output_ringbuffer[c] );		
	}	

	for (h = 0; h < 3; h++) {
		free(header[h]);
	}

	dbg("Audio decoding thread exiting\n");

	return NULL;

}

int krad_link_decklink_video_callback (void *arg, void *buffer, int length) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	//printf("krad link decklink frame received %d bytes\n", length);




		int rgb_stride_arr[3] = {4*krad_link->composite_width, 0, 0};

		const uint8_t *yuv_arr[4];
		int yuv_stride_arr[4];
	
		yuv_arr[0] = buffer;

		yuv_stride_arr[0] = krad_link->capture_width + (krad_link->capture_width/2) * 2;
		yuv_stride_arr[1] = 0;
		yuv_stride_arr[2] = 0;
		yuv_stride_arr[3] = 0;

		unsigned char *dst[4];

		dst[0] = krad_link->krad_decklink->captured_frame_rgb;

		sws_scale(krad_link->captured_frame_converter, yuv_arr, yuv_stride_arr, 0, krad_link->capture_height, dst, rgb_stride_arr);
		



	if (krad_ringbuffer_write_space(krad_link->captured_frames_buffer) >= krad_link->composited_frame_byte_size) {

		krad_ringbuffer_write(krad_link->captured_frames_buffer, 
							 (char *)krad_link->krad_decklink->captured_frame_rgb, 
							 krad_link->composited_frame_byte_size );

	} else {
	
		printf("oh no! captured frames ringbuffer overflow!\n");
	
	}


	return 0;

}


#define SAMPLE_16BIT_SCALING  32767.0f

void krad_link_int16_to_float (float *dst, char *src, unsigned long nsamples, unsigned long src_skip) {

	const float scaling = 1.0/SAMPLE_16BIT_SCALING;
	while (nsamples--) {
		*dst = (*((short *) src)) * scaling;
		dst++;
		src += src_skip;
	}
}

int krad_link_decklink_audio_callback (void *arg, void *buffer, int frames) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	int c;

	//printf("krad link decklink audio %d frames\n", frames);

	for (c = 0; c < 2; c++) {
		krad_link_int16_to_float ( krad_link->krad_decklink->samples[c], (char *)buffer + (c * 2), frames, 4);
		krad_ringbuffer_write (krad_link->audio_capture_ringbuffer[c], (char *)krad_link->krad_decklink->samples[c], frames * 4);
		//compute_peak(krad_link->krad_audio, KINPUT, krad_link->krad_decklink->samples[c], c, frames, 0);
	}

	krad_link->audio_frames_captured += frames;
	
	krad_mixer_process (frames, krad_link->krad_radio->krad_mixer);
	
	return 0;

}

void krad_link_start_decklink_capture (krad_link_t *krad_link) {

	krad_link->krad_decklink = krad_decklink_create ();
	krad_decklink_info ( krad_link->krad_decklink );
	
	krad_link->decklink_mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, "DecklinkIn", INPUT, 2, 
														  krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);	
	
	krad_link->krad_decklink->callback_pointer = krad_link;
	krad_link->krad_decklink->audio_frames_callback = krad_link_decklink_audio_callback;
	krad_link->krad_decklink->video_frame_callback = krad_link_decklink_video_callback;
	
	krad_decklink_set_verbose(krad_link->krad_decklink, verbose);
	
	krad_decklink_start (krad_link->krad_decklink);
}

void krad_link_stop_decklink_capture (krad_link_t *krad_link) {

	if (krad_link->krad_decklink != NULL) {
		krad_decklink_destroy ( krad_link->krad_decklink );
		krad_link->krad_decklink = NULL;
	}
	
	krad_link->capture_audio = 3;
	krad_link->encoding = 2;

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, krad_link->decklink_mixer_portgroup);

}


void krad_link_destroy (krad_link_t *krad_link) {

	int c;

	dbg ("Link shutting down\n");
	
	krad_link->destroy = 1;	
	
	if (krad_link->capturing) {
		krad_link->capturing = 0;
		if (krad_link->video_source == V4L2) {
			pthread_join(krad_link->video_capture_thread, NULL);
		}
		if (krad_link->video_source == DECKLINK) {
			if (krad_link->krad_decklink != NULL) {
				krad_link_stop_decklink_capture (krad_link);
			}
		}		
	} else {
		krad_link->capture_audio = 2;
		krad_link->encoding = 2;
	}
	
	if (krad_link->video_source != NOVIDEO) {
		pthread_join(krad_link->video_encoding_thread, NULL);
	} else {
		krad_link->encoding = 3;
	}
	
	if (krad_link->audio_codec != NOCODEC) {
		pthread_join(krad_link->audio_encoding_thread, NULL);
	}
	
	pthread_join(krad_link->stream_output_thread, NULL);

	if (krad_link->operation_mode == RECEIVE) {


		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_join(krad_link->video_decoding_thread, NULL);
		}
		
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_join(krad_link->audio_decoding_thread, NULL);
		}
		
		pthread_join(krad_link->stream_input_thread, NULL);
		
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		for (c = 0; c < krad_link->audio_channels; c++) {
			krad_ringbuffer_free ( krad_link->audio_capture_ringbuffer[c] );
		}
	}
	
	if (krad_link->krad_vpx_decoder) {
		krad_vpx_decoder_destroy(krad_link->krad_vpx_decoder);
	}

	kradgui_destroy(krad_link->krad_gui);

	if (krad_link->decoded_frame_converter != NULL) {
		sws_freeContext ( krad_link->decoded_frame_converter );
	}
	
	if (krad_link->captured_frame_converter != NULL) {
		sws_freeContext ( krad_link->captured_frame_converter );
	}
	
	if (krad_link->encoding_frame_converter != NULL) {
		sws_freeContext ( krad_link->encoding_frame_converter );
	}
	
	if (krad_link->display_frame_converter != NULL) {
		sws_freeContext ( krad_link->display_frame_converter );
	}

	// must be before vorbis
	//if (krad_link->krad_audio != NULL) {
	//	kradaudio_destroy (krad_link->krad_audio);
	//}
	
	if (krad_link->krad_vorbis != NULL) {
		krad_vorbis_encoder_destroy (krad_link->krad_vorbis);
	}
	
	if (krad_link->krad_flac != NULL) {
		krad_flac_encoder_destroy (krad_link->krad_flac);
	}

	if (krad_link->krad_opus != NULL) {
		kradopus_encoder_destroy(krad_link->krad_opus);
	}
	
	if (krad_link->interface_mode == WINDOW) {
		//krad_sdl_opengl_display_destroy(krad_link->krad_opengl_display);
		krad_x11_destroy (krad_link->krad_x11);
	}
	
	if (krad_link->decoded_frames_buffer != NULL) {
		krad_ringbuffer_free ( krad_link->decoded_frames_buffer );
	}
	
	krad_ringbuffer_free ( krad_link->captured_frames_buffer );
	krad_ringbuffer_free ( krad_link->encoded_audio_ringbuffer );
	krad_ringbuffer_free ( krad_link->encoded_video_ringbuffer );

	free(krad_link->current_encoding_frame);
	//free(krad_link->current_frame);

	if (krad_link->video_source == X11) {
		krad_x11_destroy (krad_link->krad_x11);
	}
	
	if (krad_link->krad_codec2_decoder != NULL) {
		krad_codec2_decoder_destroy(krad_link->krad_codec2_decoder);
	}
	
	if (krad_link->krad_codec2_encoder != NULL) {
		krad_codec2_encoder_destroy(krad_link->krad_codec2_encoder);
	}
	
	dbg ("Link shutoff cleanly.\n", APPVERSION);
	
	free (krad_link);
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

	krad_link->udp_recv_port = KRAD_LINK_DEFAULT_UDP_PORT;
	krad_link->udp_send_port = KRAD_LINK_DEFAULT_UDP_PORT;
	krad_link->tcp_port = KRAD_LINK_DEFAULT_TCP_PORT;

	krad_link->operation_mode = CAPTURE;
	krad_link->interface_mode = WINDOW;
	krad_link->video_codec = VP8;
	krad_link->audio_codec = VORBIS;
	krad_link->vorbis_quality = DEFAULT_VORBIS_QUALITY;	
	krad_link->video_source = V4L2;
	krad_link->transport_mode = TCP;



	//strncpy(krad_link->tone_preset, DEFAULT_TONE_PRESET, sizeof(krad_link->tone_preset));

	//FIXME wrong place for this vp8 stuff
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

void krad_link_activate (krad_link_t *krad_link) {

	int c;

	krad_link->composite_fps = krad_link->capture_fps;
	krad_link->encoding_fps = krad_link->capture_fps;
	
	if (krad_link->video_source == DECKLINK) {
		krad_link->composite_width = 1280;
		krad_link->composite_height = 720;
		krad_link->encoding_width = 1280;
		krad_link->encoding_height = 720;
	} else {
		krad_link->encoding_width = krad_link->capture_width;
		krad_link->encoding_height = krad_link->capture_height;
	}

	krad_link->display_width = krad_link->capture_width;
	krad_link->display_height = krad_link->capture_height;

	if (krad_link->interface_mode == WINDOW) {

		krad_link->krad_x11 = krad_x11_create_glx_window (krad_link->krad_x11, APPVERSION, krad_link->display_width, krad_link->display_height, 
														  &krad_link->destroy);

	}
	
	krad_link->composited_frame_byte_size = krad_link->composite_width * krad_link->composite_height * 4;
	//krad_link->current_frame = calloc(1, krad_link->composited_frame_byte_size);
	krad_link->current_encoding_frame = calloc(1, krad_link->composited_frame_byte_size);

	if (krad_link->video_source == V4L2) {
		krad_link->captured_frame_converter = sws_getContext ( krad_link->capture_width, krad_link->capture_height, PIX_FMT_YUYV422, 
															  krad_link->composite_width, krad_link->composite_height, PIX_FMT_RGB32, 
															  SWS_BICUBIC, NULL, NULL, NULL);
	}

	if (krad_link->video_source == DECKLINK) {
		krad_link->captured_frame_converter = sws_getContext ( krad_link->capture_width, krad_link->capture_height, PIX_FMT_UYVY422, 
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
	krad_link->composited_frames_buffer = krad_link->krad_linker->krad_radio->krad_compositor->composited_frames_buffer;
	krad_link->encoded_audio_ringbuffer = krad_ringbuffer_create (2000000);
	krad_link->encoded_video_ringbuffer = krad_ringbuffer_create (2000000);

	krad_link->krad_gui = kradgui_create_with_internal_surface(krad_link->composite_width, krad_link->composite_height);

	if (krad_link->operation_mode == CAPTURE) {

		//FIXME temp kludge
		krad_link->krad_linker->krad_radio->krad_compositor->incoming_frames_buffer = krad_link->captured_frames_buffer;

		krad_link->audio_channels = 2;

		for (c = 0; c < krad_link->audio_channels; c++) {
			krad_link->audio_capture_ringbuffer[c] = krad_ringbuffer_create (2000000);		
		}

		if ((krad_link->video_source == V4L2) || (krad_link->video_source == DECKLINK)) {
			krad_link->krad_gui->clear = 0;
		}

		if (krad_link->video_source == X11) {
			if (krad_link->krad_x11 == NULL) {
				krad_link->krad_x11 = krad_x11_create();
			}
			krad_x11_enable_capture(krad_link->krad_x11, krad_link->krad_x11->screen_width, krad_link->krad_x11->screen_height);
		}
		
		if (krad_link->video_source == V4L2) {

			krad_link->capturing = 1;
			pthread_create(&krad_link->video_capture_thread, NULL, video_capture_thread, (void *)krad_link);
		}
	
		if (krad_link->interface_mode == WINDOW) {
			krad_link->render_meters = 0;
		}
		
		if (krad_link->video_source == DECKLINK) {
			krad_link->capturing = 1;
			// delay slightly so audio encoder ringbuffer ready?
			usleep (150000);
			krad_link_start_decklink_capture(krad_link);
		}

	}
	
	if (krad_link->operation_mode == RECEIVE) {

		krad_link->decoded_frames_buffer = krad_ringbuffer_create (krad_link->composited_frame_byte_size * krad_link->encoding_buffer_frames);
		
		if (krad_link->udp_mode) {
			//FIXME move codec2 experimental stuff
			krad_link->krad_codec2_decoder = krad_codec2_decoder_create(1, 48000);
			krad_link->krad_codec2_encoder = krad_codec2_encoder_create(1, 48000);
			pthread_create(&krad_link->udp_input_thread, NULL, udp_input_thread, (void *)krad_link);	
		} else {
			pthread_create(&krad_link->stream_input_thread, NULL, stream_input_thread, (void *)krad_link);	
		}
		
		if (krad_link->interface_mode == WINDOW) {
			pthread_create(&krad_link->video_decoding_thread, NULL, video_decoding_thread, (void *)krad_link);
		}
	
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_create(&krad_link->audio_decoding_thread, NULL, audio_decoding_thread, (void *)krad_link);
		}

	}
	
	if (krad_link->operation_mode == TRANSMIT) {

		krad_link->encoding = 1;

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_create (&krad_link->video_encoding_thread, NULL, video_encoding_thread, (void *)krad_link);
		}
	
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_create (&krad_link->audio_encoding_thread, NULL, audio_encoding_thread, (void *)krad_link);
		}
	
		if (krad_link->udp_mode) {
			pthread_create (&krad_link->udp_output_thread, NULL, udp_output_thread, (void *)krad_link);	
		} else {
			pthread_create (&krad_link->stream_output_thread, NULL, stream_output_thread, (void *)krad_link);	
		}

	}
}

void krad_linker_ebml_to_link ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;

	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK) {
		printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
	
	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->host, ebml_data_size);
	
	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
		printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_link->tcp_port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->mount, ebml_data_size);

}

void krad_linker_link_to_ebml ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link) {

	uint64_t link;

	krad_ebml_start_element (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK, &link);	

	krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
	krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->tcp_port);
	krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
	
	krad_ebml_finish_element (krad_ipc_server->current_client->krad_ebml2, link);

}

int krad_linker_handler ( krad_linker_t *krad_linker, krad_ipc_server_t *krad_ipc ) {

	krad_link_t *krad_link;

	uint32_t command;
	uint64_t ebml_data_size;

	uint64_t element;
	uint64_t response;
	
	char linkname[64];	
	float floatval;
	
	uint64_t bigint;
	uint8_t tinyint;
	int k;
	
	bigint = 0;
	k = 0;
	floatval = 0;

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

		case EBML_ID_KRAD_LINK_CMD_LIST_LINKS:
			printf("krad linker handler! LIST_LINKS\n");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					printf("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
		case EBML_ID_KRAD_LINK_CMD_CREATE_LINK:
			printf("krad linker handler! CREATE_LINK\n");
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] == NULL) {
					verbose = 1;
					krad_linker->krad_link[k] = krad_link_create ();
					krad_link = krad_linker->krad_link[k];
					krad_link->krad_radio = krad_linker->krad_radio;
					krad_link->krad_linker = krad_linker;
					
					
					krad_link->interface_mode = COMMAND;

					krad_link->audio_codec = VORBIS;
					krad_link->video_source = DECKLINK;
					krad_link->video_codec = VP8;
					krad_link->av_mode = AUDIO_AND_VIDEO;

					sprintf (krad_link->sysname, "link%d", k);

					if (k != 0) {
						krad_link->operation_mode = TRANSMIT;

						strcpy (krad_link->password, "firefox");
						krad_link->tcp_port = 8000;
						strcpy (krad_link->host, "ec2-50-112-77-171.us-west-2.compute.amazonaws.com");

						sprintf (krad_link->mount, "/%s_%d.webm", krad_link->krad_radio->sysname, k);
						
						krad_linker_ebml_to_link ( krad_ipc, krad_link);
						
					} else {
						krad_link->operation_mode = CAPTURE;
						krad_link->av_mode = AUDIO_AND_VIDEO;
						krad_link->video_source = DECKLINK;
						krad_link->capture_fps = 30;
						krad_linker_ebml_to_link ( krad_ipc, krad_link);
					}
					
					krad_link_run (krad_link);
					break;
				}
			}
			break;
		case EBML_ID_KRAD_LINK_CMD_DESTROY_LINK:
			printf("krad linker handler! DESTROY_LINK\n");
			
			tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
			k = tinyint;
			printf("krad linker handler! DESTROY_LINK: %d %u\n", k, tinyint);
			
			if (krad_linker->krad_link[k] != NULL) {
				krad_link_destroy (krad_linker->krad_link[k]);
				krad_linker->krad_link[k] = NULL;
			}
			
			break;
		case EBML_ID_KRAD_LINK_CMD_UPDATE_LINK:
			printf("krad linker handler! UPDATE_LINK\n");		
			break;
	}

	return 0;
}

krad_linker_t *krad_linker_create (krad_radio_t *krad_radio) {

	krad_linker_t *krad_linker;
	
	krad_linker = calloc(1, sizeof(krad_linker_t));

	krad_linker->krad_radio = krad_radio;

	return krad_linker;

}

void krad_linker_destroy (krad_linker_t *krad_linker) {

	int l;
	
	for (l = 0; l < KRAD_LINKER_MAX_LINKS; l++) {
		if (krad_linker->krad_link[l] != NULL) {
			krad_link_destroy (krad_linker->krad_link[l]);
			krad_linker->krad_link[l] = NULL;
		}
	}

	free (krad_linker);

}


