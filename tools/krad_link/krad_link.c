#include "krad_link.h"

extern int verbose;

void krad_link_activate (krad_link_t *krad_link);

static void *krad_linker_listening_thread (void *arg);
static void krad_linker_listen_destroy_client (krad_linker_listen_client_t *krad_linker_listen_client);
static void krad_linker_listen_create_client (krad_linker_t *krad_linker, int sd);
static void *krad_linker_listen_client_thread (void *arg);

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
	
	printk ("Video capture thread started");
	
	krad_link->krad_v4l2 = kradv4l2_create();

	krad_link->krad_v4l2->mjpeg_mode = krad_link->mjpeg_mode;

	if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 1)) {
		krad_link->krad_compositor_port = 
		krad_compositor_mjpeg_port_create (krad_link->krad_radio->krad_compositor, "V4L2MJPEGIn", INPUT);
	} else {
		krad_link->krad_compositor_port = 
		krad_compositor_port_create (krad_link->krad_radio->krad_compositor, "V4L2In", INPUT);
	}

	kradv4l2_open(krad_link->krad_v4l2, krad_link->device, krad_link->capture_width, 
				  krad_link->capture_height, krad_link->capture_fps);
	
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
				printk ("Frames Captured: %u Expected: %ld - %u fps * %ld seconds", 
						krad_link->captured_frames, elapsed_time.tv_sec * krad_link->capture_fps, 
						krad_link->capture_fps, elapsed_time.tv_sec);
	
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

			sws_scale(krad_link->captured_frame_converter, yuv_arr, yuv_stride_arr, 0, 
					  krad_link->capture_height, dst, rgb_stride_arr);
	
		}
		
		if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 0)) {
			kradv4l2_mjpeg_to_rgb (krad_link->krad_v4l2, captured_frame_rgb,
								   captured_frame, krad_link->krad_v4l2->jpeg_size);
		}
	
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

		if ((krad_link->mjpeg_mode == 1) && (krad_link->mjpeg_passthru == 1)) {
			memcpy (krad_frame->pixels, captured_frame, krad_link->krad_v4l2->jpeg_size);
			krad_frame->mjpeg_size = krad_link->krad_v4l2->jpeg_size;
			//krad_frame->mjpeg_size = kradv4l2_mjpeg_to_jpeg (krad_link->krad_v4l2, krad_frame->pixels, 
			//captured_frame, krad_link->krad_v4l2->jpeg_size);
		} else {
			memcpy (krad_frame->pixels, captured_frame_rgb, krad_link->composited_frame_byte_size);
		}
		
		kradv4l2_frame_done (krad_link->krad_v4l2);
		
		krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);


		float peakval[2];
	
		if (krad_mixer_get_portgroup_from_sysname (krad_link->krad_radio->krad_mixer, "Music1") != NULL) {
	
			peakval[0] = krad_mixer_portgroup_read_channel_peak (krad_mixer_get_portgroup_from_sysname 
																(krad_link->krad_radio->krad_mixer, "Music1"), 0);
			peakval[1] = krad_mixer_portgroup_read_channel_peak (krad_mixer_get_portgroup_from_sysname 
																(krad_link->krad_radio->krad_mixer, "Music1"), 1);
			krad_compositor_set_peak (krad_link->krad_radio->krad_compositor, 0, krad_mixer_peak_scale(peakval[0]));
			krad_compositor_set_peak (krad_link->krad_radio->krad_compositor, 1, krad_mixer_peak_scale(peakval[1]));
		}
		
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

	printk ("Video capture thread exited");
	
	return NULL;
	
}



void *x11_capture_thread (void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_x11cap", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	printk ("X11 capture thread begins");
	
	krad_frame_t *krad_frame;
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "X11In",
																   INPUT);

	while (krad_link->capturing == 1) {
	
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

		krad_x11_capture (krad_link->krad_x11, (unsigned char *)krad_frame->pixels);
		
		krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		krad_compositor_process (krad_link->krad_radio->krad_compositor);

		usleep (33000);

	}

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	printk ("X11 capture thread exited");
	
	return NULL;
	
}


void *video_encoding_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_videnc", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("Video encoding thread started");
	
	krad_frame_t *krad_frame;
	void *video_packet;
	int keyframe;
	int packet_size;
	char keyframe_char[1];

	keyframe = 0;
	krad_frame = NULL;
	
	/* RGB -> YUV CONVERTER AND SCALER SETUP */	
	
	krad_compositor_get_info (krad_link->krad_radio->krad_compositor,
							  &krad_link->composite_width,
							  &krad_link->composite_height);
	
	krad_link->encoder_frame_converter = sws_getContext ( krad_link->composite_width, krad_link->composite_height,
														   PIX_FMT_RGB32,
														   krad_link->encoding_width, krad_link->encoding_height,
														   PIX_FMT_YUV420P, 
														   SWS_BICUBIC, NULL, NULL, NULL);
	
	/* CODEC SETUP */

	if (krad_link->video_codec == VP8) {

		krad_link->krad_vpx_encoder = krad_vpx_encoder_create (krad_link->encoding_width, krad_link->encoding_height);

		krad_vpx_encoder_bitrate_set (krad_link->krad_vpx_encoder, krad_link->vp8_bitrate);

		krad_vpx_encoder_config_set (krad_link->krad_vpx_encoder, &krad_link->krad_vpx_encoder->cfg);

		krad_vpx_encoder_quality_set (krad_link->krad_vpx_encoder, (((1000 / krad_link->encoding_fps) / 2) * 1000));

		printk ("Video encoding quality set to %ld", krad_link->krad_vpx_encoder->quality);
	
	}
	
	if (krad_link->video_codec == THEORA) {
		krad_link->krad_theora_encoder = krad_theora_encoder_create (krad_link->encoding_width, 
																	 krad_link->encoding_height,
																	 DEFAULT_THEORA_QUALITY);
	}
	
	/* COMPOSITOR CONNECTION */
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "VIDEnc",
																   OUTPUT);
	
	printk ("Encoding loop start");
	
	while (krad_link->encoding == 1) {

		krad_frame = krad_compositor_port_pull_frame (krad_link->krad_compositor_port);

		if (krad_frame != NULL) {

			/* CONVERT FRAME RGB -> YUV */

			int rgb_stride_arr[3] = {4*krad_link->composite_width, 0, 0};
			const uint8_t *rgb_arr[4];
			rgb_arr[0] = (unsigned char *)krad_frame->pixels;

			if (krad_link->video_codec == VP8) {
				sws_scale(krad_link->encoder_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_link->composite_height, 
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
				
				sws_scale(krad_link->encoder_frame_converter, rgb_arr, rgb_stride_arr, 0, krad_link->composite_height, 
						  planes, strides);			
			
			}						
							
			krad_framepool_unref_frame (krad_frame);
			
			/* ENCODE FRAME */
			
			if (krad_link->video_codec == VP8) {
			
				if (krad_vpx_encoder_quality_get(krad_link->krad_vpx_encoder) > 1) {	
					if (krad_compositor_port_frames_avail (krad_link->krad_compositor_port) > 25) {
						krad_vpx_encoder_quality_set (krad_link->krad_vpx_encoder, 1);
						printk ("Alert! Reduced VP8 quality due to frames avail > 25");
					}
				}
				if (krad_vpx_encoder_quality_get(krad_link->krad_vpx_encoder) == 1) {
					if (krad_compositor_port_frames_avail(krad_link->krad_compositor_port) < 1) {
						krad_vpx_encoder_quality_set (krad_link->krad_vpx_encoder,
												  (((1000 / krad_link->encoding_fps) / 2) * 1000));
						printk ("Alert! Increased VP8 quality");
					}				
				}			
				packet_size = krad_vpx_encoder_write (krad_link->krad_vpx_encoder,
								    (unsigned char **)&video_packet,
								    				  &keyframe);
			}
			
			if (krad_link->video_codec == THEORA) {
				packet_size = krad_theora_encoder_write (krad_link->krad_theora_encoder,
									   (unsigned char **)&video_packet,
									   					 &keyframe);
			}
			
			if ((packet_size) || (krad_link->video_codec == THEORA)) {
				
				//FIXME un needed memcpy
				
				keyframe_char[0] = keyframe;

				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)video_packet, packet_size);
				
				krad_link->video_frames_encoded++;

			}
	
		} else {
			// FIXME signal
			usleep (3000);
		}
	}
	
	printk ("Encoding loop done");	

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);
	
	if (krad_link->encoder_frame_converter != NULL) {
		sws_freeContext ( krad_link->encoder_frame_converter );
	}	
		
	if (krad_link->video_codec == VP8) {
		krad_vpx_encoder_finish (krad_link->krad_vpx_encoder);
		do {
			packet_size = krad_vpx_encoder_write (krad_link->krad_vpx_encoder,
							    (unsigned char **)&video_packet,
							   					  &keyframe);
			if (packet_size) {
				//FIXME goes with un needed memcpy above
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)video_packet, packet_size);
			}
							   					  
		} while (packet_size);

		krad_vpx_encoder_destroy (krad_link->krad_vpx_encoder);
	}
	
	if (krad_link->video_codec == THEORA) {
		krad_theora_encoder_destroy (krad_link->krad_theora_encoder);	
	}
	
	// FIXME make shutdown sequence more pretty
	krad_link->encoding = 3;
	if (krad_link->audio_codec == NOCODEC) {
		krad_link->encoding = 4;
	}
	
	printk ("Video encoding thread exited");
	
	return NULL;
	
}


void krad_link_audio_samples_callback (int frames, void *userdata, float **samples) {

	krad_link_t *krad_link = (krad_link_t *)userdata;
	
	if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {
		if ((krad_ringbuffer_read_space (krad_link->audio_output_ringbuffer[0]) >= frames * 4) && 
			(krad_ringbuffer_read_space (krad_link->audio_output_ringbuffer[1]) >= frames * 4)) {
			krad_ringbuffer_read (krad_link->audio_output_ringbuffer[0], (char *)samples[0], frames * 4);
			krad_ringbuffer_read (krad_link->audio_output_ringbuffer[1], (char *)samples[1], frames * 4);
		} else {
			memset(samples[0], '0', frames * 4);
			memset(samples[1], '0', frames * 4);
		}
	}

	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
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

	printk ("Audio encoding thread starting");
	
	krad_link->audio_channels = 2;	
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		krad_link->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);
		samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}

	interleaved_samples = calloc(1, 1000000);
	buffer = calloc(1, 500000);
	
	mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, 
												   OUTPUT, 2, krad_link->krad_radio->krad_mixer->master_mix,
												   KRAD_LINK, krad_link, 0);		
		
	switch (krad_link->audio_codec) {
		case VORBIS:
			printk ("Vorbis quality is %f", krad_link->vorbis_quality);
			krad_link->krad_vorbis = krad_vorbis_encoder_create (krad_link->audio_channels,
																 krad_link->krad_radio->krad_mixer->sample_rate,
																 krad_link->vorbis_quality);
			framecnt = 1024;
			break;
		case FLAC:
			krad_link->krad_flac = krad_flac_encoder_create (krad_link->audio_channels,
															 krad_link->krad_radio->krad_mixer->sample_rate,
															 24);
			framecnt = DEFAULT_FLAC_FRAME_SIZE;
			break;
		case OPUS:
			krad_link->krad_opus = kradopus_encoder_create (krad_link->krad_radio->krad_mixer->sample_rate,
															krad_link->audio_channels,
															DEFAULT_OPUS_BITRATE,
															OPUS_APPLICATION_AUDIO);
			framecnt = MIN_OPUS_FRAME_SIZE;
			break;
		default:
			failfast ("unknown audio codec");
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
					kradopus_write_audio (krad_link->krad_opus, c + 1, (char *)samples[c], framecnt * 4);
				}

				bytes = kradopus_read_opus (krad_link->krad_opus, buffer, &framecnt);
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
			
				bytes = krad_flac_encode (krad_link->krad_flac, interleaved_samples, framecnt, buffer);
			}
			
			if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {
	
				while (bytes > 0) {
					
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&framecnt, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
					
					krad_link->audio_frames_encoded += framecnt;
					bytes = 0;
					
					if (krad_link->audio_codec == OPUS) {
						bytes = kradopus_read_opus (krad_link->krad_opus, buffer, &framecnt);
					}
				}
			}
			
			if (krad_link->audio_codec == VORBIS) {
			
				op = krad_vorbis_encode (krad_link->krad_vorbis,
										 framecnt,
										 krad_link->audio_input_ringbuffer[0],
										 krad_link->audio_input_ringbuffer[1]);

				if (op != NULL) {
			
					frames = op->granulepos - krad_link->audio_frames_encoded;
				
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&op->bytes, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)op->packet, op->bytes);

					krad_link->audio_frames_encoded = op->granulepos;
					
					if (frames < framecnt / 2) {
						op = NULL;
					}
				}
			}

			//printk ("audio_encoding_thread %zu !", krad_link->audio_frames_encoded);
		}
	
		while (krad_ringbuffer_read_space (krad_link->audio_input_ringbuffer[1]) < framecnt * 4) {
			usleep (5000);
			if (krad_link->encoding == 3) {
				break;
			}
		}
		
		if ((krad_link->encoding == 3) && 
			(krad_ringbuffer_read_space (krad_link->audio_input_ringbuffer[1]) < framecnt * 4)) {
				break;
		}
	}

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, mixer_portgroup);
	
	printk ("Audio encoding thread exiting");
	
	krad_link->encoding = 4;
	
	while (krad_link->capture_audio != 3) {
		usleep(5000);
	}
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		free (krad_link->samples[c]);
		krad_ringbuffer_free ( krad_link->audio_input_ringbuffer[c] );
		free (samples[c]);	
	}	
	
	free (interleaved_samples);
	free (buffer);
	
	return NULL;
}

void *stream_output_thread (void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_stmout", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_transmission_t *krad_transmission;
	unsigned char *packet;
	int packet_size;
	int keyframe;
	int frames;
	char keyframe_char[1];
	int video_frames_muxed;
	int audio_frames_muxed;
	int audio_frames_per_video_frame;
	krad_frame_t *krad_frame;

	krad_transmission = NULL;
	krad_frame = NULL;
	audio_frames_per_video_frame = 0;
	audio_frames_muxed = 0;
	video_frames_muxed = 0;

	printk ("Output/Muxing thread starting");
	
	if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

		if (krad_link->audio_codec == OPUS) {
			audio_frames_per_video_frame = krad_link->krad_radio->krad_mixer->sample_rate / krad_link->capture_fps;
		} else {
			audio_frames_per_video_frame = krad_link->krad_radio->krad_mixer->sample_rate / krad_link->capture_fps;
			//audio_frames_per_video_frame = 1602;
		}
	}

	packet = malloc (2000000);
	
	if (krad_link->host[0] != '\0') {
	
		if ((strcmp(krad_link->host, "transmitter") == 0) &&
			(krad_link->krad_linker->krad_transmitter->listening == 1)) {
			
			krad_transmission = krad_transmitter_transmission_create (krad_link->krad_linker->krad_transmitter,
																	  krad_link->mount,
																	  "application/ogg");

			krad_link->krad_container = krad_container_open_transmission (krad_transmission);
	
		} else {
	
			krad_link->krad_container = krad_container_open_stream (krad_link->host,
																	krad_link->port,
																	krad_link->mount,
																	krad_link->password);
		}																	
	} else {
		printk ("Outputing to file: %s", krad_link->output);
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
			krad_link->krad_compositor_port = krad_compositor_mjpeg_port_create (krad_link->krad_radio->krad_compositor,
																				 "StreamOut",
																				 OUTPUT);
		}
		
		if (krad_link->video_codec != THEORA) {
		
			krad_link->video_track = krad_container_add_video_track (krad_link->krad_container,
																	 krad_link->video_codec,
																	 DEFAULT_FPS_NUMERATOR,
																	 DEFAULT_FPS_DENOMINATOR,
																	 krad_link->encoding_width,
																	 krad_link->encoding_height);
		} else {
		
			usleep (50000);
		
			krad_link->video_track = 
			krad_container_add_video_track_with_private_data (krad_link->krad_container, 
														      krad_link->video_codec,
														   	  DEFAULT_FPS_NUMERATOR,
															  DEFAULT_FPS_DENOMINATOR,
															  krad_link->encoding_width,
															  krad_link->encoding_height,
															  &krad_link->krad_theora_encoder->krad_codec_header);
		
		}
	}
	
	if (krad_link->av_mode != VIDEO_ONLY) {
	
		switch (krad_link->audio_codec) {
			case VORBIS:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container,
																		 krad_link->audio_codec,
																		 krad_link->krad_radio->krad_mixer->sample_rate,
																		 krad_link->audio_channels, 
																		 &krad_link->krad_vorbis->krad_codec_header);
				break;
			case FLAC:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container,
																		 krad_link->audio_codec,
																		 krad_link->krad_radio->krad_mixer->sample_rate,
																		 krad_link->audio_channels,
																		 &krad_link->krad_flac->krad_codec_header);
				break;
			case OPUS:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container,
																		 krad_link->audio_codec,
																		 krad_link->krad_radio->krad_mixer->sample_rate,
																		 krad_link->audio_channels, 
																		 &krad_link->krad_opus->krad_codec_header);
				break;
			default:
				failfast ("Unknown audio codec");
		}
	
	}
	
	printk ("Output/Muxing thread waiting..");
		
	while ( krad_link->encoding ) {

		if ((krad_link->av_mode != AUDIO_ONLY) && (krad_link->mjpeg_passthru == 0)) {
			if ((krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) >= 4) && (krad_link->encoding < 3)) {

				krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) < packet_size + 1) &&
					   (krad_link->encoding < 3)) {		

					usleep(10000);
				}
			
				if ((krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) < packet_size + 1) &&
					(krad_link->encoding > 2)) {
					
					continue;
				}
			
				krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)packet, packet_size);

				keyframe = keyframe_char[0];
	
				krad_container_add_video (krad_link->krad_container, 
										  krad_link->video_track,
										  packet,
										  packet_size,
										  keyframe);

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
				
				krad_container_add_video (krad_link->krad_container,
										  krad_link->video_track, 
						 (unsigned char *)krad_frame->pixels,
										  krad_frame->mjpeg_size,
										  keyframe);
				
				krad_framepool_unref_frame (krad_frame);
				video_frames_muxed++;
			}
			
			usleep(2000);
		}
		
		if (krad_link->av_mode != VIDEO_ONLY) {
		
			while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) >= 4) && 
				  ((krad_link->av_mode == AUDIO_ONLY) ||
				  ((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed))) {

				krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) &&
					   (krad_link->encoding != 4)) {
							usleep (5000);
				}
			
				if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) &&
					(krad_link->encoding == 4)) {
					break;
				}
			
				krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
				krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)packet, packet_size);

				krad_container_add_audio (krad_link->krad_container,
										  krad_link->audio_track,
										  packet,
										  packet_size,
										  frames);

				audio_frames_muxed += frames;

				//printk ("ebml muxed audio frames: %d", audio_frames_muxed);
			}

			if (krad_link->encoding == 4) {
				break;
			}
		}
		
		if (krad_link->av_mode == AUDIO_ONLY) {
		
			if (krad_ringbuffer_read_space (krad_link->encoded_audio_ringbuffer) < 4) {
				
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

			if (((krad_ringbuffer_read_space (krad_link->encoded_audio_ringbuffer) < 4) || 
				((video_frames_muxed * audio_frames_per_video_frame) > audio_frames_muxed)) && 
			     (krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) < 4)) {
		
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

	if (krad_transmission != NULL) {
		krad_transmitter_transmission_destroy (krad_transmission);
	}

	printk ("Output/Muxing thread exiting");
	
	return NULL;
	
}


void *udp_output_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_udpout", 0, 0, 0);

	printk ("UDP Output thread starting");

	krad_link_t *krad_link = (krad_link_t *)arg;

	unsigned char *buffer;
	int count;
	int packet_size;
	int frames;
	uint64_t frames_big;
	
	frames_big = 0;
	count = 0;
	
	buffer = malloc(250000);
	
	krad_link->krad_slicer = krad_slicer_create ();
	
	if (krad_link->audio_codec == OPUS) {	
	
		while ( krad_link->encoding ) {
		
			if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) >= 4)) {

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
		
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding != 4)) {
					usleep(4000);
				}
			
				if ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < packet_size + 4) && (krad_link->encoding == 4)) {
					break;
				}
			
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
				frames_big = frames;
				memcpy (buffer, &frames_big, 8);
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)buffer + 8, packet_size);

				krad_slicer_sendto (krad_link->krad_slicer, buffer, packet_size + 8, 1, krad_link->host, krad_link->port);
				count++;
				
			} else {
				if (krad_link->encoding == 4) {
					break;
				}
				usleep(4000);
			}
		}
	}
	
	krad_slicer_destroy (krad_link->krad_slicer);
	
	free (buffer);

	printk ("UDP Output thread exiting");
	
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

	printk ("Input/Demuxing thread starting");

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
		krad_link->krad_container = krad_container_open_stream (krad_link->host, krad_link->port, krad_link->mount, NULL);
	} else {
		krad_link->krad_container = krad_container_open_file (krad_link->input, KRAD_IO_READONLY);
	}
	
	while (!krad_link->destroy) {
		
		writeheaders = 0;
		total_header_size = 0;
		header_size = 0;

		packet_size = krad_container_read_packet ( krad_link->krad_container, &current_track, &packet_timecode, buffer);
		printk ("packet track %d timecode: %zu size %d", current_track, packet_timecode, packet_size);
		if ((packet_size <= 0) && (packet_timecode == 0) && ((video_packets + audio_packets) > 20))  {
			printk ("stream input thread packet size was: %d", packet_size);
			break;
		}
		
		if (krad_container_track_changed (krad_link->krad_container, current_track)) {
			printk ("track %d changed! status is %d header count is %d", current_track, krad_container_track_active(krad_link->krad_container, current_track), krad_container_track_header_count(krad_link->krad_container, current_track));
			
			track_codecs[current_track] = krad_container_track_codec (krad_link->krad_container, current_track);
			
			if (track_codecs[current_track] == NOCODEC) {
				continue;
			}
			
			writeheaders = 1;
			
			for (h = 0; h < krad_container_track_header_count (krad_link->krad_container, current_track); h++) {
				printk ("header %d is %d bytes", h, krad_container_track_header_size (krad_link->krad_container, current_track, h));
				total_header_size += krad_container_track_header_size (krad_link->krad_container, current_track, h);
			}

			
		}
		
		if ((track_codecs[current_track] == VP8) || (track_codecs[current_track] == THEORA) || (track_codecs[current_track] == DIRAC)) {

			video_packets++;

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
	
	printk ("");
	printk ("Input/Demuxing thread exiting");
	
	krad_container_destroy (krad_link->krad_container);
	
	free (buffer);
	free (header_buffer);
	return NULL;

}

void *udp_input_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_udpin", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("UDP Input thread starting");

	int sd;
	int ret;
	int rsize;
	unsigned char *buffer;
	unsigned char *packet_buffer;
	struct sockaddr_in local_address;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];	
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
	local_address.sin_port = htons (krad_link->port);
	local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1 ) {
		failfast ("UDP Input bind error");
	}
	
	//kludge to get header
	krad_opus_t *opus_temp;
	unsigned char opus_header[256];
	int opus_header_size;
	
	opus_temp = kradopus_encoder_create (krad_link->krad_radio->krad_mixer->sample_rate, 2, 110000, 
										 OPUS_APPLICATION_AUDIO);
										 
	opus_header_size = opus_temp->header_data_size;
	memcpy (opus_header, opus_temp->header_data, opus_header_size);
	kradopus_encoder_destroy(opus_temp);
	
	printk ("placing opus header size is %d", opus_header_size);
	
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&nocodec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_codec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_header_size, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)opus_header, opus_header_size);

	while (!krad_link->destroy) {
	
		sockets[0].fd = sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	
	
		if (ret < 0) {
			printk ("Krad Link UDP Poll Failure");
			krad_link->destroy = 1;
			continue;
		}
	
		if (ret > 0) {
		
			ret = recvfrom (sd, buffer, 2000, 0, (struct sockaddr *)&remote_address, (socklen_t *)&rsize);
		
			if (ret == -1) {
				printk ("Krad Link UDP Recv Failure");
				krad_link->destroy = 1;
				continue;
			}
		
			//printk ("Received packet from %s:%d", 
			//		inet_ntoa(remote_address.sin_addr), ntohs(remote_address.sin_port));


			krad_rebuilder_write (krad_link->krad_rebuilder, buffer, ret);

			ret = krad_rebuilder_read_packet (krad_link->krad_rebuilder, packet_buffer, 1);
		
			if (ret != 0) {
				//printk ("read a packet with %d bytes", ret);

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
	}

	krad_rebuilder_destroy (krad_link->krad_rebuilder);
	close (sd);
	free (buffer);
	free (packet_buffer);
	printk ("UDP Input thread exiting");
	
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

	printk ("Video decoding thread starting");

	int bytes;
	unsigned char *buffer;	
	int h;
	unsigned char *header[3];
	int header_len[3];
	uint64_t timecode;
	uint64_t timecode2;
	krad_frame_t *krad_frame;
	
	for (h = 0; h < 3; h++) {
		header[h] = malloc(100000);
		header_len[h] = 0;
	}

	bytes = 0;
	buffer = malloc(3000000);
	
	krad_link->last_video_codec = NOCODEC;
	krad_link->video_codec = NOCODEC;
	
	krad_link->krad_framepool = krad_framepool_create ( krad_link->composite_width,
														krad_link->composite_height,
														DEFAULT_CAPTURE_BUFFER_FRAMES);
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "VidDecIn",
																   INPUT);
	
	
	while (!krad_link->destroy) {


		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(10000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&krad_link->video_codec, 4);
		if ((krad_link->last_video_codec != krad_link->video_codec) || (krad_link->video_codec == NOCODEC)) {
			printk ("video codec is %d", krad_link->video_codec);
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

				printk ("Theora Header byte sizes: %d %d %d", header_len[0], header_len[1], header_len[2]);
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
	
		//printk ("frame timecode: %zu", timecode);
	
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

			krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
			while ((krad_frame == NULL) && (!krad_link->destroy)) {
				usleep(10000);
				krad_frame = krad_framepool_getframe (krad_link->krad_framepool);				
			}

			krad_link_yuv_to_rgb(krad_link, (unsigned char *)krad_frame->pixels, krad_link->krad_theora_decoder->ycbcr[0].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[0].stride), 
								 krad_link->krad_theora_decoder->ycbcr[0].stride, krad_link->krad_theora_decoder->ycbcr[1].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[1].stride), 
								 krad_link->krad_theora_decoder->ycbcr[1].stride, krad_link->krad_theora_decoder->ycbcr[2].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[2].stride), 
								 krad_link->krad_theora_decoder->ycbcr[2].stride, krad_link->krad_theora_decoder->height);

			
			krad_theora_decoder_timecode(krad_link->krad_theora_decoder, &timecode2);
			
			///printk ("timecode1: %zu timecode2: %zu", timecode, timecode2);
			
			timecode = timecode2;
			
			krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

			krad_framepool_unref_frame (krad_frame);

			krad_compositor_process (krad_link->krad_radio->krad_compositor);

		}
			
		if (krad_link->video_codec == VP8) {
			krad_vpx_decoder_decode(krad_link->krad_vpx_decoder, buffer, bytes);
				
			if (krad_link->krad_vpx_decoder->img != NULL) {

				if (krad_link->decoded_frame_converter == NULL) {

					krad_link->decoded_frame_converter = sws_getContext ( krad_link->krad_vpx_decoder->width, krad_link->krad_vpx_decoder->height, PIX_FMT_YUV420P,
																  krad_link->display_width, krad_link->display_height, PIX_FMT_RGB32, 
																  SWS_BICUBIC, NULL, NULL, NULL);

				}
				
				krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
				while ((krad_frame == NULL) && (!krad_link->destroy)) {
					usleep(10000);
					krad_frame = krad_framepool_getframe (krad_link->krad_framepool);				
				}				

				krad_link_yuv_to_rgb(krad_link, (unsigned char *)krad_frame->pixels, krad_link->krad_vpx_decoder->img->planes[0], 
									 krad_link->krad_vpx_decoder->img->stride[0], krad_link->krad_vpx_decoder->img->planes[1], 
									 krad_link->krad_vpx_decoder->img->stride[1], krad_link->krad_vpx_decoder->img->planes[2], 
									 krad_link->krad_vpx_decoder->img->stride[2], krad_link->krad_vpx_decoder->height);

				krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

				krad_framepool_unref_frame (krad_frame);

				krad_compositor_process (krad_link->krad_radio->krad_compositor);

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
				
				krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
				while ((krad_frame == NULL) && (!krad_link->destroy)) {
					usleep(10000);
					krad_frame = krad_framepool_getframe (krad_link->krad_framepool);				
				}				

				krad_link_yuv_to_rgb(krad_link, (unsigned char *)krad_frame->pixels, krad_link->krad_dirac->frame->components[0].data, 
									 krad_link->krad_dirac->frame->components[0].stride, krad_link->krad_dirac->frame->components[1].data, 
									 krad_link->krad_dirac->frame->components[1].stride, krad_link->krad_dirac->frame->components[2].data,
									 krad_link->krad_dirac->frame->components[2].stride, krad_link->krad_dirac->frame->height);

				krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

				krad_framepool_unref_frame (krad_frame);

				krad_compositor_process (krad_link->krad_radio->krad_compositor);

			}
		}
	}

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	free (buffer);
	for (h = 0; h < 3; h++) {
		free(header[h]);
	}
	printk ("Video decoding thread exiting");

	return NULL;

}

void *audio_decoding_thread(void *arg) {

	prctl (PR_SET_NAME, (unsigned long) "kradlink_auddec", 0, 0, 0);

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("Audio decoding thread starting");

	int c;
	int h;
	int bytes;
	int len;	
	unsigned char *buffer;
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
	
	krad_link->last_audio_codec = NOCODEC;
	krad_link->audio_codec = NOCODEC;
	
	
	mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, INPUT, 2, 
												   krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);
	
	while (!krad_link->destroy) {


		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(5000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&krad_link->audio_codec, 4);
		if ((krad_link->last_audio_codec != krad_link->audio_codec) || (krad_link->audio_codec == NOCODEC)) {
			printk ("audio codec is %d", krad_link->audio_codec);
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
					kradopus_decoder_destroy (krad_link->krad_opus);
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
				krad_flac_decode (krad_link->krad_flac, header[0], header_len[0], NULL);
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

				printk ("Vorbis Header byte sizes: %d %d %d", header_len[0], header_len[1], header_len[2]);

				krad_link->krad_vorbis = krad_vorbis_decoder_create (header[0], header_len[0], header[1], header_len[1], header[2], header_len[2]);
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
			
				printk ("Opus Header size: %d", header_len[0]);
				krad_link->krad_opus = kradopus_decoder_create (header[0], header_len[0], krad_link->krad_radio->krad_mixer->sample_rate);
			}
		}

		krad_link->last_audio_codec = krad_link->audio_codec;
	
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(4000);
		}
	
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < bytes) && (!krad_link->destroy)) {
			usleep(1000);
		}
		
		krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
		
		if (krad_link->audio_codec == VORBIS) {
			krad_vorbis_decoder_decode(krad_link->krad_vorbis, buffer, bytes);
			
			len = 1;
			
			while (len ) {
			
				len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 0, (char *)samples[0], 512);

				if (len) {
				
					while ((krad_ringbuffer_write_space(krad_link->audio_output_ringbuffer[0]) < len) && (!krad_link->destroy)) {
						//printk ("wait!");
						usleep(25000);
					}
				
					krad_ringbuffer_write (krad_link->audio_output_ringbuffer[0], (char *)samples[0], len);
				}

				len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 1, (char *)samples[1], 512);

				if (len) {

					while ((krad_ringbuffer_write_space(krad_link->audio_output_ringbuffer[1]) < len) && (!krad_link->destroy)) {
						//printk ("wait!");
						usleep(25000);
					}

					krad_ringbuffer_write (krad_link->audio_output_ringbuffer[1], (char *)samples[1], len);
				}
			
			}
		}
			
		if (krad_link->audio_codec == FLAC) {

			audio_frames = krad_flac_decode(krad_link->krad_flac, buffer, bytes, samples);
			
			for (c = 0; c < krad_link->audio_channels; c++) {
				krad_ringbuffer_write (krad_link->audio_output_ringbuffer[c], (char *)samples[c], audio_frames * 4);
			}
		}
			
		if (krad_link->audio_codec == OPUS) {

			kradopus_write_opus (krad_link->krad_opus, buffer, bytes);
			
			bytes = -1;

			while (bytes != 0) {
				for (c = 0; c < 2; c++) {
					bytes = kradopus_read_audio (krad_link->krad_opus, c + 1, (char *)audio, 120 * 4);
					if (bytes) {
						if ((bytes / 4) != 120) {
							failfast ("uh oh crazyto");
						}

						while ((krad_ringbuffer_write_space(krad_link->audio_output_ringbuffer[c]) < bytes) &&
							   (!krad_link->destroy)) {
								usleep(20000);
						}

						krad_ringbuffer_write (krad_link->audio_output_ringbuffer[c], (char *)audio, bytes);

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

	free (buffer);
	free (audio);
	
	for (c = 0; c < krad_link->audio_channels; c++) {
		free(krad_link->samples[c]);
		free(samples[c]);
		krad_ringbuffer_free ( krad_link->audio_output_ringbuffer[c] );		
	}	

	for (h = 0; h < 3; h++) {
		free(header[h]);
	}

	printk ("Audio decoding thread exiting");

	return NULL;

}

int krad_link_decklink_video_callback (void *arg, void *buffer, int length) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	//printk ("krad link decklink frame received %d bytes", length);

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

	if (krad_frame != NULL) {
	
		dst[0] = (unsigned char *)krad_frame->pixels;

		sws_scale (krad_link->captured_frame_converter, yuv_arr, yuv_stride_arr, 0, krad_link->capture_height, dst, rgb_stride_arr);
	
		krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		krad_compositor_process (krad_link->krad_linker->krad_radio->krad_compositor);

	} else {
	
		failfast ("Krad Decklink underflow");
	
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

	//printk ("krad link decklink audio %d frames", frames);

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
	
	if (krad_mixer_get_pusher(krad_link->krad_radio->krad_mixer) == DECKLINKAUDIO) {
		krad_mixer_process (frames, krad_link->krad_radio->krad_mixer);
	}
	
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
	
	if (krad_mixer_has_pusher(krad_link->krad_radio->krad_mixer) == 0) {
		krad_mixer_set_pusher (krad_link->krad_radio->krad_mixer, DECKLINKAUDIO);
	}
	
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

	krad_mixer_unset_pusher (krad_link->krad_radio->krad_mixer);

}


krad_tags_t *krad_link_get_tags (krad_link_t *krad_link) {
	return krad_link->krad_tags;
}


void krad_link_destroy (krad_link_t *krad_link) {

	int c;

	printk ("Link shutting down");
	
	krad_link->destroy = 1;	
	
	if (krad_link->capturing) {
		krad_link->capturing = 0;
		if ((krad_link->video_source == V4L2) || (krad_link->video_source == X11)) {
			pthread_join (krad_link->video_capture_thread, NULL);
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
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
		if (krad_link->video_source != NOVIDEO) {
			pthread_join (krad_link->video_encoding_thread, NULL);
		} else {
			krad_link->encoding = 3;
		}
	
		if (krad_link->audio_codec != NOCODEC) {
			pthread_join (krad_link->audio_encoding_thread, NULL);
		}
	}
	
	if (((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == TCP)) ||
	     (krad_link->operation_mode == RECORD)) {
			pthread_join (krad_link->stream_output_thread, NULL);
	}

	if ((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == UDP)) {
		pthread_join (krad_link->udp_output_thread, NULL);
	}

	if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {


		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_join(krad_link->video_decoding_thread, NULL);
		}
		
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_join(krad_link->audio_decoding_thread, NULL);
		}
		
		if ((krad_link->transport_mode == TCP) || (krad_link->transport_mode == FILESYSTEM)) {
			pthread_join (krad_link->stream_input_thread, NULL);
		}
		
		if (krad_link->transport_mode == UDP) {
			pthread_join (krad_link->udp_input_thread, NULL);
		}		
			
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		for (c = 0; c < krad_link->audio_channels; c++) {
			krad_ringbuffer_free ( krad_link->audio_capture_ringbuffer[c] );
		}
	}
	
	if (krad_link->krad_framepool != NULL) {
		krad_framepool_destroy (krad_link->krad_framepool);
		krad_link->krad_framepool = NULL;
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
	
	krad_ringbuffer_free ( krad_link->encoded_audio_ringbuffer );
	krad_ringbuffer_free ( krad_link->encoded_video_ringbuffer );

	free (krad_link->current_encoding_frame);

	if (krad_link->video_source == X11) {
		krad_x11_destroy (krad_link->krad_x11);
	}
	
	krad_tags_destroy (krad_link->krad_tags);	
	
	printk ("Krad Link Closed Clean");
	
	free (krad_link);
}

krad_link_t *krad_link_create() {

	krad_link_t *krad_link;
	
	krad_link = calloc(1, sizeof(krad_link_t));
		
	krad_link->capture_buffer_frames = DEFAULT_CAPTURE_BUFFER_FRAMES;
	
	krad_link->capture_width = DEFAULT_CAPTURE_WIDTH;
	krad_link->capture_height = DEFAULT_CAPTURE_HEIGHT;
	krad_link->capture_fps = DEFAULT_FPS;

	krad_link->composite_fps = krad_link->capture_fps;
	krad_link->encoding_fps = krad_link->capture_fps;
	krad_link->encoding_width = DEFAULT_ENCODER_WIDTH;
	krad_link->encoding_height = DEFAULT_ENCODER_HEIGHT;
	krad_link->display_width = krad_link->capture_width;
	krad_link->display_height = krad_link->capture_height;
	
	krad_link->vp8_bitrate = DEFAULT_VPX_BITRATE;
	
	strncpy(krad_link->device, DEFAULT_V4L2_DEVICE, sizeof(krad_link->device));
	strncpy(krad_link->alsa_capture_device, DEFAULT_ALSA_CAPTURE_DEVICE, sizeof(krad_link->alsa_capture_device));
	strncpy(krad_link->alsa_playback_device, DEFAULT_ALSA_PLAYBACK_DEVICE, sizeof(krad_link->alsa_playback_device));
	sprintf(krad_link->output, "%s/Videos/krad_link_%"PRIuMAX".webm", getenv ("HOME"), (uintmax_t)time(NULL));
	krad_link->port = 0;
	krad_link->operation_mode = CAPTURE;
	krad_link->video_codec = KRAD_LINK_DEFAULT_VIDEO_CODEC;
	krad_link->audio_codec = KRAD_LINK_DEFAULT_AUDIO_CODEC;
	krad_link->vorbis_quality = DEFAULT_VORBIS_QUALITY;	
	krad_link->video_source = NOVIDEO;
	krad_link->transport_mode = TCP;

	krad_link->krad_tags = krad_tags_create ();

	return krad_link;
}

void krad_link_activate (krad_link_t *krad_link) {

	int c;

	
	krad_compositor_get_info (krad_link->krad_radio->krad_compositor,
							  &krad_link->composite_width,
							  &krad_link->composite_height);
							  
	krad_link->display_width = krad_link->composite_width;
	krad_link->display_height = krad_link->composite_height;							  

	krad_link->encoding_fps = krad_link->capture_fps;
	
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

	krad_link->encoded_audio_ringbuffer = krad_ringbuffer_create (2000000);
	krad_link->encoded_video_ringbuffer = krad_ringbuffer_create (6000000);

	krad_link->krad_gui = kradgui_create_with_internal_surface(krad_link->composite_width, krad_link->composite_height);

	if (krad_link->operation_mode == CAPTURE) {


		krad_link->krad_framepool = krad_framepool_create ( DEFAULT_CAPTURE_WIDTH,
															DEFAULT_CAPTURE_HEIGHT,
															DEFAULT_CAPTURE_BUFFER_FRAMES);
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
	
	if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {
		
		if (krad_link->transport_mode == UDP) {
			pthread_create(&krad_link->udp_input_thread, NULL, udp_input_thread, (void *)krad_link);	
		} else {
			pthread_create(&krad_link->stream_input_thread, NULL, stream_input_thread, (void *)krad_link);	
		}
		
		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_create(&krad_link->video_decoding_thread, NULL, video_decoding_thread, (void *)krad_link);
		}
	
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_create(&krad_link->audio_decoding_thread, NULL, audio_decoding_thread, (void *)krad_link);
		}

	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {

		krad_link->encoding = 1;

		
		if ((krad_link->mjpeg_passthru == 0) && ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO))) {
			pthread_create (&krad_link->video_encoding_thread, NULL, video_encoding_thread, (void *)krad_link);
		}
	
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pthread_create (&krad_link->audio_encoding_thread, NULL, audio_encoding_thread, (void *)krad_link);
		}
	
		if ((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == UDP)) {
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
		printk ("hrm wtf");
	} else {
		//printk ("tag size %zu", ebml_data_size);
	}
	
	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK_OPERATION_MODE) {
		printk ("hrm wtf");
	} else {
		//printk ("tag size %zu", ebml_data_size);
	}
	
	
	krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
	krad_link->operation_mode = krad_link_string_to_operation_mode (string);

	if (krad_link->operation_mode == RECEIVE) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
			printk ("hrm wtf3");
		} else {
			//printk ("tag value size %zu", ebml_data_size);
		}

		krad_link->port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
	
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		if (krad_link->transport_mode == FILESYSTEM) {
	
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
				printk ("hrm wtf3");
			} else {
				//printk ("tag value size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->input, ebml_data_size);
		}
		
		if (krad_link->transport_mode == TCP) {
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
				printk ("hrm wtf2");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->host, ebml_data_size);
	
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				printk ("hrm wtf3");
			} else {
				//printk ("tag value size %zu", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
				printk ("hrm wtf2");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->mount, ebml_data_size);
		}
	
	}
	
	if (krad_link->operation_mode == CAPTURE) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE) {
			printk ("hrm wtf");
		} else {
			//printk ("tag size %zu", ebml_data_size);
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
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {


		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_AV_MODE) {
			printk ("hrm wtf");
		} else {
			//printk ("tag size %zu", ebml_data_size);
		}
	
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
		krad_link->av_mode = krad_link_string_to_av_mode (string);

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC) {
				printk ("hrm wtf2v");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
			
			krad_link->video_codec = krad_string_to_codec (string);
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH) {
				printk ("hrm wtf2v");
			} else {
				krad_link->encoding_width = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT) {
				printk ("hrm wtf2v");
			} else {
				krad_link->encoding_height = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
			}
			
			
			if (krad_link->video_codec == VP8) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
					printk ("hrm wtf2v");
				} else {
					krad_link->vp8_bitrate = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}
			
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC) {
				printk ("hrm wtf2a");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
			
			krad_link->audio_codec = krad_string_to_codec (string);
		}

	
	}
	
	
	if (krad_link->operation_mode == TRANSMIT) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->host, ebml_data_size);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
			printk ("hrm wtf3");
		} else {
			//printk ("tag value size %zu", ebml_data_size);
		}

		krad_link->port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->mount, ebml_data_size);

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PASSWORD) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->password, ebml_data_size);

		if (strstr(krad_link->mount, "flac") != NULL) {
			krad_link->audio_codec = FLAC;
		}
			
		if (strstr(krad_link->mount, "opus") != NULL) {
			krad_link->audio_codec = OPUS;
		}

	}
	
	if (krad_link->operation_mode == RECORD) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->output, ebml_data_size);

		if (strstr(krad_link->output, "flac") != NULL) {
			krad_link->audio_codec = FLAC;
		}
			
		if (strstr(krad_link->output, "opus") != NULL) {
			krad_link->audio_codec = OPUS;
		}

		krad_link->transport_mode = FILESYSTEM;

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
	
	if (krad_link->operation_mode == RECEIVE) {

		krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
								krad_link_transport_mode_to_string (krad_link->transport_mode));

		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
			krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
		}
	}	
	
	if (krad_link->operation_mode == CAPTURE) {
		krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE, krad_link_video_source_to_string (krad_link->video_source));
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
								krad_link_transport_mode_to_string (krad_link->transport_mode));
	
		if (krad_link->transport_mode == FILESYSTEM) {
	
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FILENAME, krad_link->input);
		}
		
		if (krad_link->transport_mode == TCP) {
	
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
			krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
		}
		
	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
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
		
		if (krad_link->operation_mode == TRANSMIT) {
	
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
									krad_link_transport_mode_to_string (krad_link->transport_mode));
	
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
			krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
		}
		
		if (krad_link->operation_mode == RECORD) {
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FILENAME, krad_link->output);
		}

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == OPUS)) {

			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, 
									krad_opus_signal_to_string (kradopus_get_signal (krad_link->krad_opus)));
			krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, 
									krad_opus_bandwidth_to_string (kradopus_get_bandwidth (krad_link->krad_opus)));
			krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, kradopus_get_bitrate (krad_link->krad_opus));
			krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, kradopus_get_complexity (krad_link->krad_opus));
			krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, kradopus_get_frame_size (krad_link->krad_opus));

			//EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));		

		}

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
	
	char string[128];	
	
	uint64_t bigint;
	uint8_t tinyint;
	int k;
	
	string[0] = '\0';
	bigint = 0;
	k = 0;
	
	pthread_mutex_lock (&krad_linker->change_lock);	

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

		case EBML_ID_KRAD_LINK_CMD_LIST_LINKS:
			printk ("krad linker handler! LIST_LINKS");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					printk ("Link %d Active: %s", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
		case EBML_ID_KRAD_LINK_CMD_CREATE_LINK:
			printk ("krad linker handler! CREATE_LINK");
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] == NULL) {

					krad_linker->krad_link[k] = krad_link_create ();
					krad_link = krad_linker->krad_link[k];
					krad_link->krad_radio = krad_linker->krad_radio;
					krad_link->krad_linker = krad_linker;
					
					sprintf (krad_link->sysname, "link%d", k);

					krad_linker_ebml_to_link ( krad_ipc, krad_link );
					
					krad_link_run (krad_link);

					break;
				}
			}
			break;
		case EBML_ID_KRAD_LINK_CMD_DESTROY_LINK:
			printk ("krad linker handler! DESTROY_LINK");
			
			tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
			k = tinyint;
			printk ("krad linker handler! DESTROY_LINK: %d %u", k, tinyint);
			
			if (krad_linker->krad_link[k] != NULL) {
				krad_link_destroy (krad_linker->krad_link[k]);
				krad_linker->krad_link[k] = NULL;
			}
			
			break;
		case EBML_ID_KRAD_LINK_CMD_UPDATE_LINK:
			printk ("krad linker handler! UPDATE_LINK");
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	
			
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_NUMBER) {
			
				tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
				k = tinyint;
				printk ("krad linker handler! UPDATE_LINK: %d %u", k, tinyint);
			
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

							kradopus_set_signal (krad_linker->krad_link[k]->krad_opus, 
													krad_opus_string_to_signal(string));
						}
						
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) {
							
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							
							kradopus_set_bandwidth (krad_linker->krad_link[k]->krad_opus, 
													krad_opus_string_to_bandwidth(string));
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
					
					if (krad_linker->krad_link[k]->video_codec == VP8) {
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_bitrate_set (krad_linker->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_FORCE_KEYFRAME) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_want_keyframe (krad_linker->krad_link[k]->krad_vpx_encoder);
							}
						}
					}


				}
			}
			
			break;
	
		case EBML_ID_KRAD_LINKER_CMD_LISTEN_ENABLE:
		
			krad_ebml_read_element ( krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printke ("hrm wtf6");
			}
		
			bigint = krad_ebml_read_number ( krad_ipc->current_client->krad_ebml, ebml_data_size);
		
			krad_linker_listen (krad_linker, bigint);
		
			break;

		case EBML_ID_KRAD_LINKER_CMD_LISTEN_DISABLE:
		
			krad_linker_stop_listening (krad_linker);
		
			break;
			
			
		case EBML_ID_KRAD_LINKER_CMD_TRANSMITTER_ENABLE:
		
			krad_ebml_read_element ( krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printke ("hrm wtf6");
			}
		
			bigint = krad_ebml_read_number ( krad_ipc->current_client->krad_ebml, ebml_data_size);
		
			krad_transmitter_listen_on (krad_linker->krad_transmitter, bigint);
		
			break;

		case EBML_ID_KRAD_LINKER_CMD_TRANSMITTER_DISABLE:
		
			krad_transmitter_stop_listening (krad_linker->krad_transmitter);
		
			break;			

	}

	pthread_mutex_unlock (&krad_linker->change_lock);

	return 0;
}

void krad_linker_listen_promote_client (krad_linker_listen_client_t *client) {

	krad_linker_t *krad_linker;
	krad_link_t *krad_link;	
	int k;

	krad_linker = client->krad_linker;

	pthread_mutex_lock (&krad_linker->change_lock);


	for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
		if (krad_linker->krad_link[k] == NULL) {

			krad_linker->krad_link[k] = krad_link_create ();
			krad_link = krad_linker->krad_link[k];
			krad_link->krad_radio = krad_linker->krad_radio;
			krad_link->krad_linker = krad_linker;
			
			sprintf (krad_link->sysname, "link%d", k);

			krad_link->sd = client->sd;
			strcpy (krad_link->mount, client->mount);
			strcpy (krad_link->content_type, client->content_type);
			strcpy (krad_link->host, "ListenSD");
			krad_link->port = client->sd;
			krad_link->operation_mode = PLAYBACK;
			krad_link->transport_mode = TCP;
			//FIXME default
			krad_link->av_mode = AUDIO_AND_VIDEO;
			
			krad_link_run (krad_link);

			break;
		}
	}

	pthread_mutex_unlock (&krad_linker->change_lock);
	
	free (client);
	
	pthread_exit(0);	

}

void *krad_linker_listen_client_thread (void *arg) {


	krad_linker_listen_client_t *client = (krad_linker_listen_client_t *)arg;
	
	int ret;
	char *string;
	char byte;

	while (1) {
		ret = read (client->sd, client->in_buffer + client->in_buffer_pos, 1);		
	
		if (ret == 0 || ret == -1) {
			printk ("done with linker listen client");
			krad_linker_listen_destroy_client (client);
		} else {
	
			client->in_buffer_pos += ret;
			
			byte = client->in_buffer[client->in_buffer_pos - 1];
			
			if ((byte == '\n') || (byte == '\r')) {
			
				if (client->got_mount == 0) {
					if (client->in_buffer_pos > 8) {
					
						if (strncmp(client->in_buffer, "SOURCE /", 8) == 0) {
							ret = strcspn (client->in_buffer + 8, "\n\r ");
							memcpy (client->mount, client->in_buffer + 8, ret);
							client->mount[ret] = '\0';
						
							printk ("Got a mount! its %s", client->mount);
						
							client->got_mount = 1;
						} else {
							printk ("client no good! %s", client->in_buffer);
							krad_linker_listen_destroy_client (client);							
						}
					
					} else {
						printk ("client no good! .. %s", client->in_buffer);
						krad_linker_listen_destroy_client (client);
					}
				} else {
					printk ("client buffer: %s", client->in_buffer);
					
					
					if (client->got_content_type == 0) {
						if (((string = strstr(client->in_buffer, "Content-Type:")) != NULL) ||
						    ((string = strstr(client->in_buffer, "content-type:")) != NULL) ||
							((string = strstr(client->in_buffer, "Content-type:")) != NULL)) {
							ret = strcspn(string + 14, "\n\r ");
							memcpy(client->content_type, string + 14, ret);
							client->content_type[ret] = '\0';
							client->got_content_type = 1;
							printk ("Got a content_type! its %s", client->content_type);							
						}
					} else {
					
						if (memcmp ("\r\n\r\n", &client->in_buffer[client->in_buffer_pos - 4], 4) == 0) {
						//if ((string = strstr(client->in_buffer, "\r\n\r\n")) != NULL) {
							printk ("got to the end of the http headers!");
							
							char *goodresp = "HTTP/1.0 200 OK\r\n\r\n";
							
							write (client->sd, goodresp, strlen(goodresp));
							
							krad_linker_listen_promote_client (client);
						}
					}
				}
			}
			
			
			if (client->in_buffer_pos > 1000) {
				printk ("client no good! .. %s", client->in_buffer);
				krad_linker_listen_destroy_client (client);
			}
		}
	}

	
	printk ("Krad HTTP Request: %s\n", client->in_buffer);
	

	krad_linker_listen_destroy_client (client);

	return NULL;	
	
}


void krad_linker_listen_create_client (krad_linker_t *krad_linker, int sd) {

	krad_linker_listen_client_t *client = calloc(1, sizeof(krad_linker_listen_client_t));

	client->krad_linker = krad_linker;
	
	client->sd = sd;
	
	pthread_create (&client->client_thread, NULL, krad_linker_listen_client_thread, (void *)client);
	pthread_detach (client->client_thread);	

}

void krad_linker_listen_destroy_client (krad_linker_listen_client_t *krad_linker_listen_client) {

	close (krad_linker_listen_client->sd);
		
	free (krad_linker_listen_client);
	
	pthread_exit(0);	

}

void *krad_linker_listening_thread (void *arg) {

	krad_linker_t *krad_linker = (krad_linker_t *)arg;

	int ret;
	int addr_size;
	int client_fd;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];
	
	printk ("Krad Linker: Listening thread starting\n");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_linker->stop_listening == 0) {

		sockets[0].fd = krad_linker->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	

		if (ret < 0) {
			printke ("Krad Linker: Failed on poll\n");
			krad_linker->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
		
			if ((client_fd = accept(krad_linker->sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size)) < 0) {
				close (krad_linker->sd);
				failfast ("Krad Linker: socket error on accept mayb a signal or such\n");
			}

			krad_linker_listen_create_client (krad_linker, client_fd);

		}
	}
	
	close (krad_linker->sd);
	krad_linker->port = 0;
	krad_linker->listening = 0;	

	printk ("Krad Linker: Listening thread exiting\n");

	return NULL;
}

void krad_linker_stop_listening (krad_linker_t *krad_linker) {

	if (krad_linker->listening == 1) {
		krad_linker->stop_listening = 1;
		pthread_join (krad_linker->listening_thread, NULL);
		krad_linker->stop_listening = 0;
	}
}


int krad_linker_listen (krad_linker_t *krad_linker, int port) {

	if (krad_linker->listening == 1) {
		krad_linker_stop_listening (krad_linker);
	}

	krad_linker->port = port;
	krad_linker->listening = 1;
	
	krad_linker->local_address.sin_family = AF_INET;
	krad_linker->local_address.sin_port = htons (krad_linker->port);
	krad_linker->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((krad_linker->sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printke ("Krad Linker: system call socket error\n");
		krad_linker->listening = 0;
		krad_linker->port = 0;		
		return 1;
	}

	if (bind (krad_linker->sd, (struct sockaddr *)&krad_linker->local_address, sizeof(krad_linker->local_address)) == -1) {
		printke ("Krad Linker: bind error for tcp port %d\n", krad_linker->port);
		close (krad_linker->sd);
		krad_linker->listening = 0;
		krad_linker->port = 0;
		return 1;
	}
	
	if (listen (krad_linker->sd, SOMAXCONN) <0) {
		printke ("Krad Linker: system call listen error\n");
		close (krad_linker->sd);
		return 1;
	}	
	
	pthread_create (&krad_linker->listening_thread, NULL, krad_linker_listening_thread, (void *)krad_linker);
	
	return 0;
}


krad_link_t *krad_linker_get_link_from_sysname (krad_linker_t *krad_linker, char *sysname) {

	int i;
	krad_link_t *krad_link;

	for (i = 0; i < KRAD_LINKER_MAX_LINKS; i++) {
		krad_link = krad_linker->krad_link[i];
		if (krad_link != NULL) {
			if (strcmp(sysname, krad_link->sysname) == 0) {	
				return krad_link;
			}
		}
	}

	return NULL;
}

krad_tags_t *krad_linker_get_tags_for_link (krad_linker_t *krad_linker, char *sysname) {

	krad_link_t *krad_link;
	
	krad_link = krad_linker_get_link_from_sysname (krad_linker, sysname);

	if (krad_link != NULL) {
		return krad_link_get_tags (krad_link);
	} else {
		return NULL;
	}
}


krad_linker_t *krad_linker_create (krad_radio_t *krad_radio) {

	krad_linker_t *krad_linker;
	
	krad_linker = calloc(1, sizeof(krad_linker_t));

	krad_linker->krad_radio = krad_radio;

	pthread_mutex_init (&krad_linker->change_lock, NULL);	

	krad_linker->krad_transmitter = krad_transmitter_create ();

	return krad_linker;

}

void krad_linker_destroy (krad_linker_t *krad_linker) {

	int l;

	krad_transmitter_destroy (krad_linker->krad_transmitter);

	pthread_mutex_lock (&krad_linker->change_lock);	
	for (l = 0; l < KRAD_LINKER_MAX_LINKS; l++) {
		if (krad_linker->krad_link[l] != NULL) {
			krad_link_destroy (krad_linker->krad_link[l]);
			krad_linker->krad_link[l] = NULL;
		}
	}
	pthread_mutex_unlock (&krad_linker->change_lock);		
	pthread_mutex_destroy (&krad_linker->change_lock);
	free (krad_linker);

}


