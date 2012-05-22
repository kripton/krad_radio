#include "krad_link.h"

extern int verbose;

void krad_link_activate (krad_link_t *krad_link);

void *video_capture_thread (void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_vidcap", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	int first = 1;
	struct timespec start_time;
	struct timespec current_time;
	struct timespec elapsed_time;
	void *captured_frame = NULL;
	unsigned char *captured_frame_rgb = malloc(krad_link->composited_frame_byte_size); 
	krad_frame_t *krad_frame;
	
	printk ("Video capture thread started\n");
	
	krad_link->krad_v4l2 = kradv4l2_create();

	krad_link->krad_v4l2->mjpeg_mode = krad_link->mjpeg_mode;

	if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 1)) {
		krad_link->krad_compositor_port = krad_compositor_mjpeg_port_create (krad_link->krad_radio->krad_compositor, "V4L2MJPEGIn", INPUT);
	} else {
		krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor, "V4L2In", INPUT);
	}

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
				printk ("Frames Captured: %u Expected: %ld - %u fps * %ld seconds\n", krad_link->captured_frames, elapsed_time.tv_sec * krad_link->capture_fps, krad_link->capture_fps, elapsed_time.tv_sec);
	
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
	
		}
		
		if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 0)) {
			kradv4l2_mjpeg_to_rgb (krad_link->krad_v4l2, captured_frame_rgb, captured_frame, krad_link->krad_v4l2->jpeg_size);
		}
	
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

		if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 1)) {
			memcpy (krad_frame->pixels, captured_frame, krad_link->krad_v4l2->jpeg_size);
			krad_frame->mjpeg_size = krad_link->krad_v4l2->jpeg_size;
			//krad_frame->mjpeg_size = kradv4l2_mjpeg_to_jpeg (krad_link->krad_v4l2, krad_frame->pixels, captured_frame, krad_link->krad_v4l2->jpeg_size);
		} else {
			memcpy (krad_frame->pixels, captured_frame_rgb, krad_link->composited_frame_byte_size);
		}
		
		kradv4l2_frame_done (krad_link->krad_v4l2);
		
		krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);
		
		if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 1)) {
			krad_compositor_mjpeg_process (krad_link->krad_radio->krad_compositor);
		} else {
			krad_compositor_process (krad_link->krad_radio->krad_compositor);
		}
	
	}

	kradv4l2_stop_capturing (krad_link->krad_v4l2);
	kradv4l2_close(krad_link->krad_v4l2);
	kradv4l2_destroy(krad_link->krad_v4l2);

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	free(captured_frame_rgb);

	krad_link->capture_audio = 2;
	krad_link->encoding = 2;

	printk ("Video capture thread exited\n");
	
	return NULL;
	
}



void *x11_capture_thread (void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_x11cap", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	printk ("X11 capture thread begins\n");
	
	krad_frame_t *krad_frame;
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor, "X11In", INPUT);

	while (krad_link->capturing == 1) {
	
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

		krad_x11_capture (krad_link->krad_x11, (unsigned char *)krad_frame->pixels);
		
		krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		krad_compositor_process (krad_link->krad_radio->krad_compositor);

		usleep (30000);

	}

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	printk ("X11 capture thread exited\n");
	
	return NULL;
	
}


void *video_encoding_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_videnc", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	if (krad_link->verbose) {
		printk ("Video encoding thread started\n");
	}
	
	krad_frame_t *krad_frame;
	void *video_packet;
	int keyframe;
	int packet_size;
	char keyframe_char[1];

	krad_frame = NULL;

	if (krad_link->video_codec == VP8) {
		vpx_codec_enc_config_default(interface, &krad_link->vpx_encoder_config, 0);
	
		krad_link->vpx_encoder_config.rc_target_bitrate = DEFAULT_VPX_BITRATE;
		krad_link->vpx_encoder_config.kf_max_dist = krad_link->capture_fps * 3;	
		krad_link->vpx_encoder_config.g_w = krad_link->encoding_width;
		krad_link->vpx_encoder_config.g_h = krad_link->encoding_height;
		krad_link->vpx_encoder_config.g_threads = 4;
		krad_link->vpx_encoder_config.kf_mode = VPX_KF_AUTO;
		krad_link->vpx_encoder_config.rc_end_usage = VPX_VBR;

		krad_link->krad_vpx_encoder = krad_vpx_encoder_create(krad_link->encoding_width, krad_link->encoding_height, krad_link->vpx_encoder_config.rc_target_bitrate);

		krad_link->vpx_encoder_config.g_w = krad_link->encoding_width;
		krad_link->vpx_encoder_config.g_h = krad_link->encoding_height;

		krad_vpx_encoder_set(krad_link->krad_vpx_encoder, &krad_link->vpx_encoder_config);

		krad_link->krad_vpx_encoder->quality = (600 / krad_link->encoding_fps) * 1000;

		printk ("Video encoding quality set to %ld\n", krad_link->krad_vpx_encoder->quality);
	
	}
	
	if (krad_link->video_codec == THEORA) {
		krad_link->krad_theora_encoder = krad_theora_encoder_create (krad_link->encoding_width, krad_link->encoding_height, DEFAULT_THEORA_QUALITY);
	}
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor, "VIDEnc", OUTPUT);
	
	printk ("Encoding loop start\n");
	
	while (krad_link->encoding == 1) {

		krad_frame = krad_compositor_port_pull_frame (krad_link->krad_compositor_port);

		if (krad_frame != NULL) {
								 
				 

			int rgb_stride_arr[3] = {4*krad_link->composite_width, 0, 0};
			const uint8_t *rgb_arr[4];
	
			rgb_arr[0] = (unsigned char *)krad_frame->pixels;

			if (krad_link->video_codec == VP8) {
				sws_scale(krad_link->encoding_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_link->composite_height, 
						  krad_link->krad_vpx_encoder->image->planes, krad_link->krad_vpx_encoder->image->stride);
			}
			
			if (krad_link->video_codec == THEORA) {
				
				unsigned char *planes[3];
				int strides[3];
				
				planes[0] = krad_link->krad_theora_encoder->ycbcr[0].data;
				planes[1] = krad_link->krad_theora_encoder->ycbcr[1].data;
				planes[2] = krad_link->krad_theora_encoder->ycbcr[2].data;
				strides[0] = krad_link->krad_theora_encoder->ycbcr[0].stride;
				strides[1] = krad_link->krad_theora_encoder->ycbcr[1].stride;
				strides[2] = krad_link->krad_theora_encoder->ycbcr[2].stride;
				
				sws_scale(krad_link->encoding_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_link->composite_height, 
						  planes, strides);			
			
			}						
							
			krad_framepool_unref_frame (krad_frame);
									
			
			if (krad_link->video_codec == VP8) {		 
				packet_size = krad_vpx_encoder_write (krad_link->krad_vpx_encoder, (unsigned char **)&video_packet, &keyframe);
			}
			
			if (krad_link->video_codec == THEORA) {
				packet_size = krad_theora_encoder_write (krad_link->krad_theora_encoder, (unsigned char **)&video_packet, &keyframe);
			}
			
			if (packet_size) {

				keyframe_char[0] = keyframe;

				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)video_packet, packet_size);
				
				krad_link->video_frames_encoded++;

			}
	
		} else {
			
			usleep(5000);
		
		}
		
	}
	
	printk ("Encoding loop done\n");	
	
	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);	
	
	
	if (krad_link->video_codec == VP8) {	
		while (packet_size) {
			krad_vpx_encoder_finish(krad_link->krad_vpx_encoder);
			packet_size = krad_vpx_encoder_write(krad_link->krad_vpx_encoder, (unsigned char **)&video_packet, &keyframe);
		}

		krad_vpx_encoder_destroy (krad_link->krad_vpx_encoder);
	}
	
	if (krad_link->video_codec == THEORA) {
		krad_theora_encoder_destroy (krad_link->krad_theora_encoder);	
	}
	
	
	krad_link->encoding = 3;

	if (krad_link->audio_codec == NOCODEC) {
		krad_link->encoding = 4;
	}
	
	printk ("Video encoding thread exited\n");
	
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

	printk ("Audio encoding thread starting\n");
	
	krad_link->audio_channels = 2;	
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		krad_link->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}

	interleaved_samples = calloc(1, 1000000);
	buffer = calloc(1, 500000);
	
	mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, OUTPUT, 2, 
												   krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);		
		
	switch (krad_link->audio_codec) {
		case VORBIS:
			printk ("Vorbis quality is %f\n", krad_link->vorbis_quality);
			krad_link->krad_vorbis = krad_vorbis_encoder_create (krad_link->audio_channels, KRAD_LINK_DEFAULT_SAMPLE_RATE, krad_link->vorbis_quality);
			framecnt = 1024;
			break;
		case FLAC:
			krad_link->krad_flac = krad_flac_encoder_create (krad_link->audio_channels, KRAD_LINK_DEFAULT_SAMPLE_RATE, 24);
			framecnt = DEFAULT_FLAC_FRAME_SIZE;
			break;
		case OPUS:
			krad_link->krad_opus = kradopus_encoder_create (KRAD_LINK_DEFAULT_SAMPLE_RATE, krad_link->audio_channels, DEFAULT_OPUS_BITRATE, OPUS_APPLICATION_AUDIO);
			framecnt = MIN_OPUS_FRAME_SIZE;
			break;
		default:
			failfast ("unknown audio codec\n");
	}
	
	krad_link->audio_encoder_ready = 1;
	
	if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {
		op = NULL;
	}
	
	while (krad_link->encoding) {

		while ((krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[1]) >= framecnt * 4) || (op != NULL)) {
			
			
			if (krad_link->audio_codec == OPUS) {

				for (c = 0; c < krad_link->audio_channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)samples[c], (framecnt * 4) );
					kradopus_write_audio(krad_link->krad_opus, c + 1, (char *)samples[c], framecnt * 4);
				}

				bytes = kradopus_read_opus (krad_link->krad_opus, buffer);
			
			}
			
			
			if (krad_link->audio_codec == FLAC) {
				
				for (c = 0; c < krad_link->audio_channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)samples[c], (framecnt * 4) );
				}
			
				for (s = 0; s < framecnt; s++) {
					for (c = 0; c < krad_link->audio_channels; c++) {
						interleaved_samples[s * krad_link->audio_channels + c] = samples[c][s];
					}
				}
			
				bytes = krad_flac_encode(krad_link->krad_flac, interleaved_samples, framecnt, buffer);
	
			}
			
			
			if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {
	
				while (bytes > 0) {
					
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&framecnt, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
					
					krad_link->audio_frames_encoded += framecnt;
					bytes = 0;
					
					if (krad_link->audio_codec == OPUS) {
						bytes = kradopus_read_opus (krad_link->krad_opus, buffer);
					}
					
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
					
					if (frames < framecnt / 2) {
						op = NULL;
					}
				}
			}

			//printk ("audio_encoding_thread %zu !\n", krad_link->audio_frames_encoded);			

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
	
	printk ("Audio encoding thread exiting\n");
	
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
	krad_frame_t *krad_frame;

	krad_frame = NULL;
	audio_frames_per_video_frame = 0;
	audio_frames_muxed = 0;
	video_frames_muxed = 0;

	printk ("Output/Muxing thread starting\n");
	
	if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

		if (krad_link->audio_codec == OPUS) {
			audio_frames_per_video_frame = KRAD_LINK_DEFAULT_SAMPLE_RATE / krad_link->capture_fps;
		} else {
			audio_frames_per_video_frame = KRAD_LINK_DEFAULT_SAMPLE_RATE / krad_link->capture_fps;
			//audio_frames_per_video_frame = 1602;
		}
	}

	packet = malloc (2000000);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad_container = krad_container_open_stream (krad_link->host, krad_link->tcp_port, krad_link->mount, krad_link->password);
	} else {
		printk ("Outputing to file: %s\n", krad_link->output);
		krad_link->krad_container = krad_container_open_file (krad_link->output, KRAD_EBML_IO_WRITEONLY);
	}
	
	//FIXME needed?
	if (krad_link->av_mode != VIDEO_ONLY) {
		while ( krad_link->audio_encoder_ready != 1) {
			usleep(10000);
		}
	}
	
	if (krad_link->krad_container->container_type == EBML) {
		if (((krad_link->audio_codec == VORBIS) || (krad_link->audio_codec == NOCODEC)) && 
			((krad_link->video_codec == VP8) || (krad_link->video_codec == NOCODEC))) {
			krad_ebml_header (krad_link->krad_container->krad_ebml, "webm", APPVERSION);
		} else {
			krad_ebml_header (krad_link->krad_container->krad_ebml, "matroska", APPVERSION);
		}
	}
	
	if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		if (krad_link->mjpeg_passthru == 1) {
	
			krad_link->krad_compositor_port = krad_compositor_mjpeg_port_create (krad_link->krad_radio->krad_compositor, "StreamOut", OUTPUT);

		}
		
		if (krad_link->video_codec != THEORA) {
		
			krad_link->video_track = krad_container_add_video_track (krad_link->krad_container, krad_link->video_codec, DEFAULT_FPS_NUMERATOR, DEFAULT_FPS_DENOMINATOR,
															krad_link->encoding_width, krad_link->encoding_height);
		} else {
		
			usleep (50000);
		
			krad_link->video_track = krad_container_add_video_track_with_private_data (krad_link->krad_container, krad_link->video_codec, DEFAULT_FPS_NUMERATOR, DEFAULT_FPS_DENOMINATOR,
															krad_link->encoding_width, krad_link->encoding_height, krad_link->krad_theora_encoder->header_combined, krad_link->krad_theora_encoder->header_combined_size);
		
		}
	}
	
	if (krad_link->av_mode != VIDEO_ONLY) {
	
		switch (krad_link->audio_codec) {
			case VORBIS:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container, krad_link->audio_codec, KRAD_LINK_DEFAULT_SAMPLE_RATE, krad_link->audio_channels, 
																	krad_link->krad_vorbis->header, krad_link->krad_vorbis->headerpos);
				break;
			case FLAC:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container, krad_link->audio_codec,  KRAD_LINK_DEFAULT_SAMPLE_RATE, krad_link->audio_channels, 
																	(unsigned char *)krad_link->krad_flac->min_header, FLAC_MINIMAL_HEADER_SIZE);
				break;
			case OPUS:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container, krad_link->audio_codec, KRAD_LINK_DEFAULT_SAMPLE_RATE, krad_link->audio_channels, 
																	krad_link->krad_opus->header_data, krad_link->krad_opus->header_data_size);
				break;
			default:
				failfast ("Unknown audio codec\n");
		}
	
	}
	
	printk ("Output/Muxing thread waiting..\n");
		
	while ( krad_link->encoding ) {

		if ((krad_link->av_mode != AUDIO_ONLY) && (krad_link->mjpeg_passthru == 0)) {
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
	

				krad_container_add_video (krad_link->krad_container, krad_link->video_track, packet, packet_size, keyframe);

				video_frames_muxed++;
		
			}
		}
		
		if (krad_link->mjpeg_passthru == 1) {

			krad_frame = krad_compositor_port_pull_frame (krad_link->krad_compositor_port);

			if (krad_frame != NULL) {

				if (video_frames_muxed % 4 == 0) {
					keyframe = 1;
				} else {
					keyframe = 0;
				}
				
				krad_container_add_video (krad_link->krad_container, krad_link->video_track, (unsigned char *)krad_frame->pixels, krad_frame->mjpeg_size, keyframe);
							
				krad_framepool_unref_frame (krad_frame);
			
				video_frames_muxed++;

	

			}
			
			usleep(2000);
		
		}
		
		if (krad_link->av_mode != VIDEO_ONLY) {
		
			while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) >= 4) && 
				  ((krad_link->av_mode == AUDIO_ONLY) || ((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed))) {

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding != 4)) {
					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding == 4)) {
					break;
				}
			
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)packet, packet_size);

				krad_container_add_audio (krad_link->krad_container, krad_link->audio_track, packet, packet_size, frames);

				audio_frames_muxed += frames;
				
				//printk ("ebml muxed audio frames: %d\n\n", audio_frames_muxed);
				
			}

			if (krad_link->encoding == 4) {
				break;
			}
		}
		
		if (krad_link->av_mode == AUDIO_ONLY) {
		
			if (krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) {
				
				if (krad_link->encoding == 4) {
					break;
				}

				usleep (8000);
			
			}
		}
		
		if ((krad_link->av_mode == VIDEO_ONLY) && (krad_link->mjpeg_passthru == 0)) {
		
			if (krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) < 4) {
		
				if (krad_link->encoding == 4) {
					break;
				}

				usleep (8000);
			
			}
		}

		if (krad_link->av_mode == AUDIO_AND_VIDEO) {

			if (((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) || 
				((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed)) && 
			   (krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4)) {
		
				if (krad_link->encoding == 4) {
					break;
				}

				usleep (8000);
			
			}
		}
		
		//krad_ebml_write_tag (krad_link->krad_ebml, "test tag 1", "monkey 123");
	
	}

	krad_container_destroy (krad_link->krad_container);
	
	free (packet);
	
	if (krad_link->mjpeg_passthru == 1) {
		krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);
	}

	printk ("Output/Muxing thread exiting\n");
	
	return NULL;
	
}


void *udp_output_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_udpout", 0, 0, 0);

	printk ("UDP Output thread starting\n");

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

	printk ("UDP Output thread exiting\n");
	
	return NULL;

}

/*
void krad_link_display (krad_link_t *krad_link) {

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
*/

void *krad_link_run_thread (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_link_activate ( krad_link );
	
	while (!krad_link->destroy) {
		usleep(50000);
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

	printk ("Input/Demuxing thread starting\n");

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

	header_buffer = malloc (4096 * 512);
	buffer = malloc (4096 * 512);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad_container = krad_container_open_stream (krad_link->host, krad_link->tcp_port, krad_link->mount, NULL);
	} else {
		krad_link->krad_container = krad_container_open_file (krad_link->input, KRAD_IO_READONLY);
	}
	
	while (!krad_link->destroy) {
		
		writeheaders = 0;
		total_header_size = 0;
		header_size = 0;

		packet_size = krad_container_read_packet ( krad_link->krad_container, &current_track, &packet_timecode, buffer);
		//printk ("packet track %d timecode: %zu\n", current_track, packet_timecode);
		if ((packet_size <= 0) && (packet_timecode == 0) && ((video_packets + audio_packets) > 20))  {
			printk ("\nstream input thread packet size was: %d\n", packet_size);
			break;
		}
		
		if (krad_container_track_changed (krad_link->krad_container, current_track)) {
			printk ("track %d changed! status is %d header count is %d\n", current_track, krad_container_track_active(krad_link->krad_container, current_track), krad_container_track_header_count(krad_link->krad_container, current_track));
			
			track_codecs[current_track] = krad_container_track_codec (krad_link->krad_container, current_track);
			
			if (track_codecs[current_track] == NOCODEC) {
				continue;
			}
			
			writeheaders = 1;
			
			for (h = 0; h < krad_container_track_header_count (krad_link->krad_container, current_track); h++) {
				printk ("header %d is %d bytes\n", h, krad_container_track_header_size (krad_link->krad_container, current_track, h));
				total_header_size += krad_container_track_header_size (krad_link->krad_container, current_track, h);
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
			
				while ((krad_ringbuffer_write_space (krad_link->encoded_audio_ringbuffer) < packet_size + 4 + total_header_size + 4 + 4) && (!krad_link->destroy)) {
					usleep(10000);
				}
				
				if (writeheaders == 1) {
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&nocodec, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&track_codecs[current_track], 4);
					for (h = 0; h < krad_container_track_header_count (krad_link->krad_container, current_track); h++) {
						header_size = krad_container_track_header_size (krad_link->krad_container, current_track, h);
						krad_container_read_track_header (krad_link->krad_container, header_buffer, current_track, h);
						krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&header_size, 4);
						krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)header_buffer, header_size);
						codec_bytes += packet_size;
					}
				} else {
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&track_codecs[current_track], 4);
				}
				

				krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)buffer, packet_size);
				codec_bytes += packet_size;
			}
			
		}
	}
	
	printk ("\n");
	printk ("Input/Demuxing thread exiting\n");
	
	krad_container_destroy (krad_link->krad_container);
	
	free (buffer);
	free (header_buffer);
	return NULL;

}

void *udp_input_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_udpin", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("UDP Input thread starting\n");

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
		failfast ("UDP Input bind error\n");
	}
	
	//kludge to get header
	krad_opus_t *opus_temp;
	unsigned char opus_header[256];
	int opus_header_size;
	
	opus_temp = kradopus_encoder_create(KRAD_LINK_DEFAULT_SAMPLE_RATE, 2, 110000, OPUS_APPLICATION_AUDIO);
	opus_header_size = opus_temp->header_data_size;
	memcpy (opus_header, opus_temp->header_data, opus_header_size);
	kradopus_encoder_destroy(opus_temp);
	
	printk ("placing opus header size is %d\n", opus_header_size);
	
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&nocodec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_codec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_header_size, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)opus_header, opus_header_size);

	while (!krad_link->destroy) {
	
		ret = recvfrom(sd, buffer, 2000, 0, (struct sockaddr *)&remote_address, (socklen_t *)&rsize);
		
		if (ret == -1) {
			printk ("failed recvin udp\n");
			krad_link->destroy = 1;
			continue;
		}
		
		//printk ("Received packet from %s:%d\n", 
		//		inet_ntoa(remote_address.sin_addr), ntohs(remote_address.sin_port));


		krad_rebuilder_write (krad_link->krad_rebuilder, buffer, ret);

		ret = krad_rebuilder_read_packet (krad_link->krad_rebuilder, packet_buffer, 1);
		
		if (ret != 0) {
			//printk ("read a packet with %d bytes\n", ret);

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
	printk ("UDP Input thread exiting\n");
	
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

	printk ("Video decoding thread starting\n");

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
			printk ("\nvideo codec is %d\n", krad_link->video_codec);
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

				printk ("\n\nTheora Header byte sizes: %d %d %d\n", header_len[0], header_len[1], header_len[2]);
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
	
		//printk ("\nframe timecode: %zu\n", timecode);
	
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
			
			///printk ("\n\ntimecode1: %zu timecode2: %zu\n\n", timecode, timecode2);
			
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
	printk ("Video decoding thread exiting\n");

	return NULL;

}

void *audio_decoding_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_auddec", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("Audio decoding thread starting\n");

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
			printk ("\n\naudio codec is %d\n", krad_link->audio_codec);
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

				printk ("\n\nVorbis Header byte sizes: %d %d %d\n", header_len[0], header_len[1], header_len[2]);
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
			
				printk ("Opus Header size: %d\n", header_len[0]);
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
						//printk ("wait!\n");
						usleep(25000);
					}
				
					krad_ringbuffer_write (krad_link->audio_output_ringbuffer[0], (char *)samples[0], len);
				}

				len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 1, (char *)samples[1], 512);

				if (len) {

					while (krad_ringbuffer_write_space(krad_link->audio_output_ringbuffer[1]) < len) {
						//printk ("wait!\n");
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
						//printk ("\nAudio data! %d samplers\n", bytes / 4);
						
						if ((bytes / 4) != 960) {
							failfast ("uh oh crazyto\n");
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

	printk ("Audio decoding thread exiting\n");

	return NULL;

}

int krad_link_decklink_video_callback (void *arg, void *buffer, int length) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	//printk ("krad link decklink frame received %d bytes\n", length);

	krad_frame_t *krad_frame;


	int rgb_stride_arr[3] = {4*krad_link->composite_width, 0, 0};

	const uint8_t *yuv_arr[4];
	int yuv_stride_arr[4];

	yuv_arr[0] = buffer;

	yuv_stride_arr[0] = krad_link->capture_width + (krad_link->capture_width/2) * 2;
	yuv_stride_arr[1] = 0;
	yuv_stride_arr[2] = 0;
	yuv_stride_arr[3] = 0;

	unsigned char *dst[4];

	//dst[0] = krad_link->krad_decklink->captured_frame_rgb;

	krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

	dst[0] = (unsigned char *)krad_frame->pixels;

	sws_scale (krad_link->captured_frame_converter, yuv_arr, yuv_stride_arr, 0, krad_link->capture_height, dst, rgb_stride_arr);
	
	krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

	krad_framepool_unref_frame (krad_frame);

	krad_compositor_process (krad_link->krad_linker->krad_radio->krad_compositor);

	/*
	if (krad_ringbuffer_write_space(krad_link->captured_frames_buffer) >= krad_link->composited_frame_byte_size) {

		krad_ringbuffer_write (krad_link->captured_frames_buffer, 
							  (char *)krad_link->krad_decklink->captured_frame_rgb, 
							  krad_link->composited_frame_byte_size );

	} else {
	
		printk ("oh no! captured frames ringbuffer overflow!\n");
	
	}
	*/

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

	//printk ("krad link decklink audio %d frames\n", frames);

	for (c = 0; c < 2; c++) {
		krad_link_int16_to_float ( krad_link->krad_decklink->samples[c], (char *)buffer + (c * 2), frames, 4);
		krad_ringbuffer_write (krad_link->audio_capture_ringbuffer[c], (char *)krad_link->krad_decklink->samples[c], frames * 4);
		//compute_peak(krad_link->krad_audio, KINPUT, krad_link->krad_decklink->samples[c], c, frames, 0);
	}

	krad_link->audio_frames_captured += frames;
	
	float peakval[2];
	
	peakval[0] = krad_mixer_portgroup_read_channel_peak (krad_mixer_get_portgroup_from_sysname (krad_link->krad_radio->krad_mixer, "DecklinkIn"), 0);
	peakval[1] = krad_mixer_portgroup_read_channel_peak (krad_mixer_get_portgroup_from_sysname (krad_link->krad_radio->krad_mixer, "DecklinkIn"), 1);
	krad_compositor_set_peak (krad_link->krad_radio->krad_compositor, 0, krad_mixer_peak_scale(peakval[0]));
	krad_compositor_set_peak (krad_link->krad_radio->krad_compositor, 1, krad_mixer_peak_scale(peakval[1]));
	
	krad_mixer_process (frames, krad_link->krad_radio->krad_mixer);
	
	return 0;

}

void krad_link_start_decklink_capture (krad_link_t *krad_link) {

	krad_link->krad_decklink = krad_decklink_create ();
	//krad_decklink_info ( krad_link->krad_decklink );
	
	krad_link->krad_mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, "DecklinkIn", INPUT, 2, 
														  krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);	
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor, "DecklinkIn", INPUT);	
	
	krad_link->krad_decklink->callback_pointer = krad_link;
	krad_link->krad_decklink->audio_frames_callback = krad_link_decklink_audio_callback;
	krad_link->krad_decklink->video_frame_callback = krad_link_decklink_video_callback;
	
	krad_decklink_set_verbose (krad_link->krad_decklink, verbose);
	
	krad_decklink_start (krad_link->krad_decklink);
}

void krad_link_stop_decklink_capture (krad_link_t *krad_link) {

	if (krad_link->krad_decklink != NULL) {
		krad_decklink_destroy ( krad_link->krad_decklink );
		krad_link->krad_decklink = NULL;
	}
	
	krad_link->capture_audio = 3;
	krad_link->encoding = 2;

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, krad_link->krad_mixer_portgroup);

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

}


void krad_link_destroy (krad_link_t *krad_link) {

	int c;

	printk ("Link shutting down\n");
	
	krad_link->destroy = 1;	
	
	if (krad_link->capturing) {
		krad_link->capturing = 0;
		if ((krad_link->video_source == V4L2) || (krad_link->video_source == X11)) {
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
		
		if (krad_link->krad_framepool != NULL) {
			krad_framepool_destroy (krad_link->krad_framepool);
			krad_link->krad_framepool = NULL;
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

	free (krad_link->current_encoding_frame);


	if (krad_link->video_source == X11) {
		krad_x11_destroy (krad_link->krad_x11);
	}
	
	if (krad_link->krad_codec2_decoder != NULL) {
		krad_codec2_decoder_destroy(krad_link->krad_codec2_decoder);
	}
	
	if (krad_link->krad_codec2_encoder != NULL) {
		krad_codec2_encoder_destroy(krad_link->krad_codec2_encoder);
	}
	
	printk ("Link shutoff cleanly.\n", APPVERSION);
	
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
	sprintf(krad_link->output, "%s/Videos/krad_link_%"PRIuMAX".webm", getenv ("HOME"), (uintmax_t)time(NULL));

	krad_link->udp_recv_port = KRAD_LINK_DEFAULT_UDP_PORT;
	krad_link->udp_send_port = KRAD_LINK_DEFAULT_UDP_PORT;
	krad_link->tcp_port = KRAD_LINK_DEFAULT_TCP_PORT;

	krad_link->operation_mode = CAPTURE;
	krad_link->interface_mode = COMMAND;
	krad_link->video_codec = KRAD_LINK_DEFAULT_VIDEO_CODEC;
	krad_link->audio_codec = KRAD_LINK_DEFAULT_AUDIO_CODEC;
	krad_link->vorbis_quality = DEFAULT_VORBIS_QUALITY;	
	krad_link->video_source = NOVIDEO;
	krad_link->transport_mode = TCP;

	return krad_link;
}

void krad_link_activate (krad_link_t *krad_link) {

	int c;

	krad_link->composite_fps = krad_link->capture_fps;
	krad_link->encoding_fps = krad_link->capture_fps;
	
	if (krad_link->video_source == DECKLINK) {
		krad_link->composite_width = DEFAULT_WIDTH;
		krad_link->composite_height = DEFAULT_HEIGHT;
		krad_link->encoding_width = DEFAULT_WIDTH;
		krad_link->encoding_height = DEFAULT_HEIGHT;
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
		  
	if ((krad_link->video_codec != NOCODEC) && (krad_link->mjpeg_passthru == 0)) {
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
	krad_link->encoded_audio_ringbuffer = krad_ringbuffer_create (3000000);
	krad_link->encoded_video_ringbuffer = krad_ringbuffer_create (7000000);

	krad_link->krad_gui = kradgui_create_with_internal_surface(krad_link->composite_width, krad_link->composite_height);

	if (krad_link->operation_mode == CAPTURE) {


		krad_link->krad_framepool = krad_framepool_create ( DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_CAPTURE_BUFFER_FRAMES);

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
			krad_x11_enable_capture (krad_link->krad_x11, krad_link->krad_x11->screen_width, krad_link->krad_x11->screen_height);
			krad_link->capturing = 1;
			pthread_create(&krad_link->video_capture_thread, NULL, x11_capture_thread, (void *)krad_link);
		}
		
		if (krad_link->video_source == V4L2) {

			krad_link->capturing = 1;
			pthread_create(&krad_link->video_capture_thread, NULL, video_capture_thread, (void *)krad_link);
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
			krad_link->krad_codec2_decoder = krad_codec2_decoder_create(1, KRAD_LINK_DEFAULT_SAMPLE_RATE);
			krad_link->krad_codec2_encoder = krad_codec2_encoder_create(1, KRAD_LINK_DEFAULT_SAMPLE_RATE);
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

		
		if ((krad_link->mjpeg_passthru == 0) && ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO))) {
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

void krad_linker_ebml_to_link ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;

	char string[512];
	
	memset (string, '\0', 512);

	//FIXME default
	krad_link->av_mode = AUDIO_AND_VIDEO;

	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK) {
		printk ("hrm wtf\n");
	} else {
		//printk ("tag size %zu\n", ebml_data_size);
	}
	
	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK_OPERATION_MODE) {
		printk ("hrm wtf\n");
	} else {
		//printk ("tag size %zu\n", ebml_data_size);
	}
	
	
	krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
	krad_link->operation_mode = krad_link_string_to_operation_mode (string);
	
	if (krad_link->operation_mode == CAPTURE) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE) {
			printk ("hrm wtf\n");
		} else {
			//printk ("tag size %zu\n", ebml_data_size);
		}
	
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
		krad_link->video_source = krad_link_string_to_video_source (string);
		
		if (krad_link->video_source == DECKLINK) {
			krad_link->av_mode = AUDIO_AND_VIDEO;
		}
		
		if ((krad_link->video_source == V4L2) || (krad_link->video_source == X11) || (krad_link->video_source == TEST)) {
			krad_link->av_mode = VIDEO_ONLY;
		}
	
	}
	
	if (krad_link->operation_mode == TRANSMIT) {


		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_AV_MODE) {
			printk ("hrm wtf\n");
		} else {
			//printk ("tag size %zu\n", ebml_data_size);
		}
	
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
		krad_link->av_mode = krad_link_string_to_av_mode (string);

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC) {
				printk ("hrm wtf2v\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
			
			krad_link->video_codec = krad_string_to_codec (string);
			
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC) {
				printk ("hrm wtf2a\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
			
			krad_link->audio_codec = krad_string_to_codec (string);
		}

	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->host, ebml_data_size);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
			printk ("hrm wtf3\n");
		} else {
			//printk ("tag value size %zu\n", ebml_data_size);
		}

		krad_link->tcp_port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->mount, ebml_data_size);

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PASSWORD) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->password, ebml_data_size);

	}

}

void krad_linker_link_to_ebml ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link) {

	uint64_t link;

	krad_ebml_start_element (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK, &link);	


	switch ( krad_link->av_mode ) {

		case AUDIO_ONLY:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "audio only");
			break;
		case VIDEO_ONLY:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "video only");
			break;
		case AUDIO_AND_VIDEO:		
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "audio and video");
			break;
	}
	
	
	switch ( krad_link->operation_mode ) {

		case TRANSMIT:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "transmit");
			break;
		case RECORD:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "record");
			break;
		case PLAYBACK:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "playback");
			break;
		case RECEIVE:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "receive");
			break;
		case CAPTURE:
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "capture");
			break;
		default:		
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "Other/Unknown");
			break;
	}
	
	if (krad_link->operation_mode == TRANSMIT) {
		switch ( krad_link->av_mode ) {

			case AUDIO_ONLY:
				krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (krad_link->audio_codec));
				break;
			case VIDEO_ONLY:
				krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (krad_link->video_codec));
				break;
			case AUDIO_AND_VIDEO:		
				krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (krad_link->video_codec));				
				krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (krad_link->audio_codec));
				break;
		}	
	
		krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
		krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->tcp_port);
		krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
	}
	
	krad_ebml_finish_element (krad_ipc_server->current_client->krad_ebml2, link);

}

int krad_linker_handler ( krad_linker_t *krad_linker, krad_ipc_server_t *krad_ipc ) {

	krad_link_t *krad_link;

	uint32_t ebml_id;

	uint32_t command;
	uint64_t ebml_data_size;

	uint64_t element;
	uint64_t response;
	
	char linkname[64];	
	float floatval;
	
	char string[128];	
	
	uint64_t bigint;
	uint8_t tinyint;
	int k;
	
	string[0] = '\0';
	bigint = 0;
	k = 0;
	floatval = 0;

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

		case EBML_ID_KRAD_LINK_CMD_LIST_LINKS:
			printk ("krad linker handler! LIST_LINKS\n");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					printk ("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
		case EBML_ID_KRAD_LINK_CMD_CREATE_LINK:
			printk ("krad linker handler! CREATE_LINK\n");
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] == NULL) {
					verbose = 1;
					krad_linker->krad_link[k] = krad_link_create ();
					krad_link = krad_linker->krad_link[k];
					krad_link->krad_radio = krad_linker->krad_radio;
					krad_link->krad_linker = krad_linker;
					
					sprintf (krad_link->sysname, "link%d", k);
					krad_link->verbose = 1;
					
					if (strstr(krad_link->mount, "flac") != NULL) {
						krad_link->audio_codec = FLAC;
					}
						
					if (strstr(krad_link->mount, "opus") != NULL) {
						krad_link->audio_codec = OPUS;
					}

					krad_linker_ebml_to_link ( krad_ipc, krad_link );
					
					krad_link_run (krad_link);

					break;
				}
			}
			break;
		case EBML_ID_KRAD_LINK_CMD_DESTROY_LINK:
			printk ("krad linker handler! DESTROY_LINK\n");
			
			tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
			k = tinyint;
			printk ("krad linker handler! DESTROY_LINK: %d %u\n", k, tinyint);
			
			if (krad_linker->krad_link[k] != NULL) {
				krad_link_destroy (krad_linker->krad_link[k]);
				krad_linker->krad_link[k] = NULL;
			}
			
			break;
		case EBML_ID_KRAD_LINK_CMD_UPDATE_LINK:
			printk ("krad linker handler! UPDATE_LINK\n");
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	
			
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_NUMBER) {
			
				tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
				k = tinyint;
				printk ("krad linker handler! UPDATE_LINK: %d %u\n", k, tinyint);
			
				if (krad_linker->krad_link[k] != NULL) {

					krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

					if (krad_linker->krad_link[k]->audio_codec == OPUS) {

						/*
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_APPLICATION) {
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							if (strncmp(string, "OPUS_APPLICATION_VOIP", 21) == 0) {
								kradopus_set_application (krad_linker->krad_link[k]->krad_opus, OPUS_APPLICATION_VOIP);
							}
							if (strncmp(string, "OPUS_APPLICATION_AUDIO", 22) == 0) {
								kradopus_set_application (krad_linker->krad_link[k]->krad_opus, OPUS_APPLICATION_AUDIO);
							}
							if (strncmp(string, "OPUS_APPLICATION_RESTRICTED_LOWDELAY", 36) == 0) {
								kradopus_set_application (krad_linker->krad_link[k]->krad_opus, OPUS_APPLICATION_RESTRICTED_LOWDELAY);
							}
						}
						*/
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL) {
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							if (strncmp(string, "OPUS_SIGNAL_AUTO", 16) == 0) {
								kradopus_set_signal (krad_linker->krad_link[k]->krad_opus, OPUS_AUTO);
							}
							if (strncmp(string, "OPUS_SIGNAL_VOICE", 17) == 0) {
								kradopus_set_signal (krad_linker->krad_link[k]->krad_opus, OPUS_SIGNAL_VOICE);
							}
							if (strncmp(string, "OPUS_SIGNAL_MUSIC", 17) == 0) {
								kradopus_set_signal (krad_linker->krad_link[k]->krad_opus, OPUS_SIGNAL_MUSIC);
							}						
						}

						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if ((bigint >= 500) && (bigint <= 512000)) {
								kradopus_set_bitrate (krad_linker->krad_link[k]->krad_opus, bigint);
							}
						}
						
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if ((bigint >= 0) && (bigint <= 10)) {
								kradopus_set_complexity (krad_linker->krad_link[k]->krad_opus, bigint);
							}
						}						
				
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							if ((bigint == 120) || (bigint == 240) || (bigint == 480) || (bigint == 960) || (bigint == 1920) || (bigint == 2880)) {
								kradopus_set_frame_size (krad_linker->krad_link[k]->krad_opus, bigint);
							}
						}
						
						//FIXME verify ogg container
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							if ((bigint > 0) && (bigint < 200)) {					
								krad_ogg_set_max_packets_per_page (krad_linker->krad_link[k]->krad_container->krad_ogg, bigint);
							}
						}
				
					}


				}
			}
				
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


