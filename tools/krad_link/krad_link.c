#include "krad_link.h"

extern int verbose;

void krad_link_activate (krad_link_t *krad_link);
static void *krad_linker_listening_thread (void *arg);
static void krad_linker_listen_destroy_client (krad_linker_listen_client_t *krad_linker_listen_client);
static void krad_linker_listen_create_client (krad_linker_t *krad_linker, int sd);
static void *krad_linker_listen_client_thread (void *arg);

void *video_capture_thread (void *arg) {

#ifndef __MACH__
	krad_system_set_thread_name ("kr_cap_v4l2");

	krad_link_t *krad_link = (krad_link_t *)arg;

	void *captured_frame = NULL;
	krad_frame_t *krad_frame;
	
	printk ("Video capture thread started");
	
	krad_link->krad_v4l2 = kradv4l2_create ();

	if ((krad_link->video_codec != NOCODEC) && (krad_link->video_passthru == 1)) {
		if (krad_link->video_codec == MJPEG) {
			krad_v4l2_mjpeg_mode (krad_link->krad_v4l2);
		}
		if (krad_link->video_codec == H264) {
			krad_v4l2_h264_mode (krad_link->krad_v4l2);
		}
		if ((krad_link->video_codec != MJPEG) && (krad_link->video_codec != H264)) {
			krad_link->video_passthru = 0;
		}
	}

	kradv4l2_open (krad_link->krad_v4l2, krad_link->device, krad_link->capture_width, 
				   krad_link->capture_height, krad_link->capture_fps);

	if ((krad_link->capture_width != krad_link->krad_v4l2->width) ||
		(krad_link->capture_height != krad_link->krad_v4l2->height)) {

		printke ("Got a different resolution from V4L2 than requested.");
		printk ("Wanted: %dx%d Got: %dx%d",
				krad_link->capture_width, krad_link->capture_height,
				krad_link->krad_v4l2->width, krad_link->krad_v4l2->height
				);
				 
		krad_link->capture_width = krad_link->krad_v4l2->width;
		krad_link->capture_height = krad_link->krad_v4l2->height;
	}


	if (krad_link->video_passthru == 1) {
		krad_link->krad_framepool = krad_framepool_create_for_passthru (350000, DEFAULT_CAPTURE_BUFFER_FRAMES * 3);
	} else {

		krad_link->krad_framepool = krad_framepool_create_for_upscale ( krad_link->capture_width,
															krad_link->capture_height,
															DEFAULT_CAPTURE_BUFFER_FRAMES,
															krad_link->composite_width, krad_link->composite_height);
	}

	if (krad_link->video_passthru == 1) {
		if (krad_link->video_codec == MJPEG) {
			krad_link->krad_compositor_port = 
			krad_compositor_passthru_port_create (krad_link->krad_radio->krad_compositor, "V4L2MJPEGpassthruIn", INPUT);
		}
		if (krad_link->video_codec == H264) {
			krad_link->krad_compositor_port = 
			krad_compositor_passthru_port_create (krad_link->krad_radio->krad_compositor, "V4L2H264passthruIn", INPUT);
		}	
	} else {
		krad_link->krad_compositor_port = 
		krad_compositor_port_create (krad_link->krad_radio->krad_compositor, "V4L2In", INPUT,
									 krad_link->capture_width, krad_link->capture_height);
	}
	
	kradv4l2_start_capturing (krad_link->krad_v4l2);

	while (krad_link->capturing == 1) {

		captured_frame = kradv4l2_read_frame_wait_adv (krad_link->krad_v4l2);
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
		
		if ((0) && (krad_link->video_passthru == 0)) {
			//FIXME mjpeg mode but not passthu
			kradv4l2_mjpeg_to_rgb (krad_link->krad_v4l2, (unsigned char *)krad_frame->pixels,
								   captured_frame, krad_link->krad_v4l2->encoded_size);
		}

		if (krad_link->video_passthru == 1) {
			memcpy (krad_frame->pixels, captured_frame, krad_link->krad_v4l2->encoded_size);
			krad_frame->encoded_size = krad_link->krad_v4l2->encoded_size;			
			kradv4l2_frame_done (krad_link->krad_v4l2);
			krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);			
			
		} else {			
			if ((0) && (krad_link->video_passthru == 0)) {
				//FIXME mjpeg mode but not passthu			
				kradv4l2_frame_done (krad_link->krad_v4l2);
				krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);
			} else {
			
				krad_frame->format = PIX_FMT_YUYV422;

				krad_frame->yuv_pixels[0] = captured_frame;
				krad_frame->yuv_pixels[1] = NULL;
				krad_frame->yuv_pixels[2] = NULL;

				krad_frame->yuv_strides[0] = krad_link->capture_width + (krad_link->capture_width/2) * 2;
				krad_frame->yuv_strides[1] = 0;
				krad_frame->yuv_strides[2] = 0;
				krad_frame->yuv_strides[3] = 0;

				krad_compositor_port_push_yuv_frame (krad_link->krad_compositor_port, krad_frame);
				
				kradv4l2_frame_done (krad_link->krad_v4l2);				
				
			}	
			
		}

		krad_framepool_unref_frame (krad_frame);
		
		if (krad_link->video_passthru == 1) {
			krad_compositor_passthru_process (krad_link->krad_radio->krad_compositor);
		} else {
			//krad_compositor_process (krad_link->krad_radio->krad_compositor);
		}
	
	}

	kradv4l2_stop_capturing (krad_link->krad_v4l2);
	kradv4l2_close(krad_link->krad_v4l2);
	kradv4l2_destroy(krad_link->krad_v4l2);

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	krad_link->encoding = 2;

	printk ("Video capture thread exited");
	
#endif

	return NULL;
	
}

void *info_screen_generator_thread (void *arg) {

	krad_system_set_thread_name ("kr_info_gen");

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	printk ("info screen generator thread begins");
	
	krad_frame_t *krad_frame;
	krad_ticker_t *krad_ticker;
	krad_gui_t *krad_gui;
	
	int width;
	int height;
	
	width = 480;
	height = 270;
	
	width = 1280;
	height = 720;	

	krad_link->krad_framepool = krad_framepool_create ( width,
														height,
														DEFAULT_CAPTURE_BUFFER_FRAMES);

	krad_gui = krad_gui_create (width, height);
	
	krad_ticker = krad_ticker_create (DEFAULT_FPS_NUMERATOR, DEFAULT_FPS_DENOMINATOR);

	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "NFOIn",
																   INPUT,
																   width, height);

	krad_ticker_start (krad_ticker);

	while (krad_link->capturing == 1) {
	
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
		krad_gui_set_surface (krad_gui, krad_frame->cst);
		krad_gui_render (krad_gui);
		
		krad_gui_render_text (krad_gui, 400, 200, 80, "KRAD RADIO");
		
		krad_gui_render_text (krad_gui, 400, 300, 40, krad_link->krad_radio->sysname);		
		
		krad_gui_render_hex (krad_gui, width - 48, 40, 18);
		
		
		
		int p;
		char *artist;
		char *title;
		char *playtime;
		krad_mixer_portgroup_t *portgroup;

		for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
			portgroup = krad_link->krad_radio->krad_mixer->portgroup[p];
			if ((portgroup != NULL) && (portgroup->active)) {
				artist = krad_tags_get_tag (portgroup->krad_tags, "artist");
				if ((artist != NULL) && strlen (artist)) {
					krad_gui_render_text (krad_gui, 120, 500, 22, artist);
					//printk ("now playing %s", now_playing);
					break;
				}
			}
		}
	
		for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
			portgroup = krad_link->krad_radio->krad_mixer->portgroup[p];
			if ((portgroup != NULL) && (portgroup->active)) {
				title = krad_tags_get_tag (portgroup->krad_tags, "title");
				if ((title != NULL) && strlen (title)) {
					krad_gui_render_text (krad_gui, 120, 550, 22, title);
					//printk ("now playing %s", now_playing);
					break;
				}
			}
		}
		
	
		for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
			portgroup = krad_link->krad_radio->krad_mixer->portgroup[p];
			if ((portgroup != NULL) && (portgroup->active)) {
				playtime = krad_tags_get_tag (portgroup->krad_tags, "playtime");
				if ((playtime != NULL) && strlen (playtime)) {
					krad_gui_render_text (krad_gui, 120, 420, 32, playtime);
					//printk ("now playing %s", now_playing);
					break;
				}
			}
		}
		
		krad_link->krad_radio->krad_compositor->render_vu_meters = 1;
		krad_frame->format = PIX_FMT_RGB32;
		krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		krad_ticker_wait (krad_ticker);

	}

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	krad_ticker_destroy (krad_ticker);

	krad_gui_destroy (krad_gui);

	printk ("info_screen_generator thread exited");
	
	return NULL;
	
}


void *test_screen_generator_thread (void *arg) {

	krad_system_set_thread_name ("kr_test_gen");

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	printk ("test screen generator thread begins");
	
	krad_frame_t *krad_frame;
	krad_ticker_t *krad_ticker;
	krad_gui_t *krad_gui;
	
	int width;
	int height;
	
	width = 480;
	height = 270;
	
	width = 1280;
	height = 720;
	
	krad_link->krad_framepool = krad_framepool_create ( width,
														height,
														DEFAULT_CAPTURE_BUFFER_FRAMES);

	krad_gui = krad_gui_create (width, height);
	
	krad_gui_test_screen (krad_gui, krad_link->krad_linker->krad_radio->sysname);
	
	krad_ticker = krad_ticker_create (krad_link->krad_radio->krad_compositor->frame_rate_numerator,
									  krad_link->krad_radio->krad_compositor->frame_rate_denominator);	
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "TSTIn",
																   INPUT,
																   width, height);

	krad_ticker_start (krad_ticker);

	while (krad_link->capturing == 1) {
	
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
		krad_gui_set_surface (krad_gui, krad_frame->cst);
		krad_gui_render (krad_gui);
		krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		krad_ticker_wait (krad_ticker);

	}

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	krad_ticker_destroy (krad_ticker);

	krad_gui_destroy (krad_gui);

	printk ("test_screen_generator thread exited");
	
	return NULL;
	
}

void *x11_capture_thread (void *arg) {

	krad_system_set_thread_name ("kr_capture_x11");

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	printk ("X11 capture thread begins");
	
	krad_frame_t *krad_frame;
	krad_ticker_t *krad_ticker;
	
	krad_ticker = krad_ticker_create (DEFAULT_FPS_NUMERATOR, DEFAULT_FPS_DENOMINATOR);	
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "X11In",
																   INPUT,
																   krad_link->krad_x11->screen_width, krad_link->krad_x11->screen_height);

	krad_ticker_start (krad_ticker);

	while (krad_link->capturing == 1) {
	
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

		krad_x11_capture (krad_link->krad_x11, (unsigned char *)krad_frame->pixels);
		
		krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		krad_ticker_wait (krad_ticker);

	}

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	krad_ticker_destroy (krad_ticker);

	printk ("X11 capture thread exited");
	
	return NULL;
	
}


void *video_encoding_thread (void *arg) {

	krad_system_set_thread_name ("kr_video_enc");

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("Video encoding thread started");
	
	krad_frame_t *krad_frame;
	void *video_packet;
	int keyframe;
	int packet_size;
	char keyframe_char[1];
	unsigned char *planes[3];
	int strides[3];

	keyframe = 0;
	krad_frame = NULL;
	
	/* CODEC SETUP */

	if (krad_link->video_codec == VP8) {

		krad_link->krad_vpx_encoder = krad_vpx_encoder_create (krad_link->encoding_width,
															   krad_link->encoding_height,
															   krad_link->encoding_fps_numerator,
															   krad_link->encoding_fps_denominator,															   
															   krad_link->vp8_bitrate);

		if (krad_link->operation_mode == TRANSMIT) {
			krad_link->krad_vpx_encoder->cfg.kf_max_dist = 90;
		}

		if (krad_link->operation_mode == RECORD) {
			krad_link->krad_vpx_encoder->cfg.rc_max_quantizer = 28;
		}

		krad_vpx_encoder_config_set (krad_link->krad_vpx_encoder, &krad_link->krad_vpx_encoder->cfg);

		krad_vpx_encoder_deadline_set (krad_link->krad_vpx_encoder, (((1000 / ((krad_link->encoding_fps_numerator / krad_link->encoding_fps_denominator)) / 3) * 3) * 1000));

		printk ("Video encoding deadline set to %ld", krad_link->krad_vpx_encoder->deadline);
	
	}
	
	if (krad_link->video_codec == THEORA) {
		krad_link->krad_theora_encoder = krad_theora_encoder_create (krad_link->encoding_width, 
																	 krad_link->encoding_height,
																	 krad_link->encoding_fps_numerator,
																	 krad_link->encoding_fps_denominator,
																	 krad_link->theora_quality);
	}
	
	if (krad_link->video_codec == H264) {
		krad_link->krad_x264_encoder = krad_x264_encoder_create (krad_link->encoding_width, 
																	 krad_link->encoding_height,
																	 krad_link->encoding_fps_numerator,
																	 krad_link->encoding_fps_denominator,
																	 krad_link->vp8_bitrate);
	}	
	
	/* COMPOSITOR CONNECTION */
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "VIDEnc",
																   OUTPUT,
																   krad_link->encoding_width, 
																   krad_link->encoding_height);
	
	printk ("Encoding loop start");
	
	while (krad_link->encoding == 1) {

		if (krad_link->video_codec == VP8) {
			planes[0] = krad_link->krad_vpx_encoder->image->planes[0];
			planes[1] = krad_link->krad_vpx_encoder->image->planes[1];
			planes[2] = krad_link->krad_vpx_encoder->image->planes[2];
			strides[0] = krad_link->krad_vpx_encoder->image->stride[0];
			strides[1] = krad_link->krad_vpx_encoder->image->stride[1];
			strides[2] = krad_link->krad_vpx_encoder->image->stride[2];
		}
		
		if (krad_link->video_codec == THEORA) {			
			planes[0] = krad_link->krad_theora_encoder->ycbcr[0].data;
			planes[1] = krad_link->krad_theora_encoder->ycbcr[1].data;
			planes[2] = krad_link->krad_theora_encoder->ycbcr[2].data;
			strides[0] = krad_link->krad_theora_encoder->ycbcr[0].stride;
			strides[1] = krad_link->krad_theora_encoder->ycbcr[1].stride;
			strides[2] = krad_link->krad_theora_encoder->ycbcr[2].stride;	
		}
		
		if (krad_link->video_codec == H264) {			
			planes[0] = krad_link->krad_x264_encoder->picture->img.plane[0];
			planes[1] = krad_link->krad_x264_encoder->picture->img.plane[1];
			planes[2] = krad_link->krad_x264_encoder->picture->img.plane[2];
			strides[0] = krad_link->krad_x264_encoder->picture->img.i_stride[0];
			strides[1] = krad_link->krad_x264_encoder->picture->img.i_stride[1];
			strides[2] = krad_link->krad_x264_encoder->picture->img.i_stride[2];	
		}		
				
		krad_frame = krad_compositor_port_pull_yuv_frame (krad_link->krad_compositor_port, planes, strides);

		if (krad_frame != NULL) {

			/* ENCODE FRAME */
		
			if (krad_link->video_codec == VP8) {
		
				if (krad_vpx_encoder_deadline_get(krad_link->krad_vpx_encoder) > 1) {	
					if (krad_compositor_port_frames_avail (krad_link->krad_compositor_port) > 25) {
						krad_vpx_encoder_deadline_set (krad_link->krad_vpx_encoder, 1);
						printk ("Alert! Reduced VP8 deadline due to frames avail > 25");
					}
				}
				if (krad_vpx_encoder_deadline_get(krad_link->krad_vpx_encoder) == 1) {
					if (krad_compositor_port_frames_avail(krad_link->krad_compositor_port) < 1) {
						krad_vpx_encoder_deadline_set (krad_link->krad_vpx_encoder,
												  ((((1000 / (krad_link->encoding_fps_numerator / krad_link->encoding_fps_denominator)) / 3) * 2) * 1000));
						printk ("Alert! Increased VP8 deadline");
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
			
			if (krad_link->video_codec == H264) {
				packet_size = krad_x264_encoder_write (krad_link->krad_x264_encoder,
									   (unsigned char **)&video_packet,
									   					 &keyframe);
			}		
		
			if ((packet_size) || (krad_link->video_codec == THEORA)) {
			
				//FIXME un needed memcpy
			
				keyframe_char[0] = keyframe;

				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
				krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)video_packet, packet_size);

			}
			
			krad_framepool_unref_frame (krad_frame);
	
		} else {
			// FIXME signal
			usleep (3000);
		}
	}
	
	printk ("Encoding loop done");	

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);
		
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
	
	if (krad_link->video_codec == H264) {
		krad_x264_encoder_destroy (krad_link->krad_x264_encoder);	
	}	
	
	// FIXME make shutdown sequence more pretty
	krad_link->encoding = 3;
	if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->audio_codec == NOCODEC)) {
		krad_link->encoding = 4;
	}
	
	printk ("Video encoding thread exited");
	
	return NULL;
	
}


void krad_link_audio_samples_callback (int frames, void *userdata, float **samples) {

	krad_link_t *krad_link = (krad_link_t *)userdata;
	
	int c;
	
	if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {
		if (((krad_link->playing > 0) || (krad_link->av_mode == AUDIO_ONLY) || (1)) && 
			 (krad_ringbuffer_read_space (krad_link->audio_output_ringbuffer[0]) >= frames * 4) && 
			 (krad_ringbuffer_read_space (krad_link->audio_output_ringbuffer[1]) >= frames * 4)) {
			 	krad_ringbuffer_read (krad_link->audio_output_ringbuffer[0], (char *)samples[0], frames * 4);
				krad_ringbuffer_read (krad_link->audio_output_ringbuffer[1], (char *)samples[1], frames * 4);
		} else {
			memset(samples[0], '0', frames * 4);
			memset(samples[1], '0', frames * 4);
			
			if (krad_link->playing == 3) {
				krad_link->destroy = 1;
			}
		}
	}

	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
		krad_link->audio_frames_captured += frames;
		for (c = 0; c < krad_link->channels; c++ ) {
			krad_ringbuffer_write (krad_link->audio_input_ringbuffer[c], (char *)samples[c], frames * 4);
		}
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		krad_link->audio_frames_captured += frames;
		krad_ringbuffer_read (krad_link->audio_capture_ringbuffer[0], (char *)samples[0], frames * 4);
		krad_ringbuffer_read (krad_link->audio_capture_ringbuffer[1], (char *)samples[1], frames * 4);
	}	

}

void *audio_encoding_thread (void *arg) {

	krad_system_set_thread_name ("kr_audio_enc");

	krad_link_t *krad_link = (krad_link_t *)arg;

	int c;
	int s;
	int bytes;
	int frames;
	float *samples[KRAD_MIXER_MAX_CHANNELS];
	float *interleaved_samples;
	unsigned char *buffer;
	int framecnt;
	krad_mixer_portgroup_t *mixer_portgroup;

	printk ("Audio encoding thread starting");
	
	krad_link->channels = 2;
	
	interleaved_samples = malloc (8192 * 4 * KRAD_MIXER_MAX_CHANNELS);
	buffer = malloc (300000);
	
	for (c = 0; c < krad_link->channels; c++) {
		samples[c] = malloc (8192 * 4);
		krad_link->samples[c] = malloc (8192 * 4);
		krad_link->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);		
	}
	
	mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, 
												   OUTPUT, krad_link->channels,
												   krad_link->krad_radio->krad_mixer->master_mix,
												   KRAD_LINK, krad_link, 0);		
		
	switch (krad_link->audio_codec) {
		case VORBIS:
			krad_link->krad_vorbis = krad_vorbis_encoder_create (krad_link->channels,
																 krad_link->krad_radio->krad_mixer->sample_rate,
																 krad_link->vorbis_quality);
			framecnt = KRAD_DEFAULT_VORBIS_FRAME_SIZE;
			break;
		case FLAC:
			krad_link->krad_flac = krad_flac_encoder_create (krad_link->channels,
															 krad_link->krad_radio->krad_mixer->sample_rate,
															 krad_link->flac_bit_depth);
			framecnt = KRAD_DEFAULT_FLAC_FRAME_SIZE;
			break;
		case OPUS:
			krad_link->krad_opus = krad_opus_encoder_create (krad_link->channels,
															 krad_link->krad_radio->krad_mixer->sample_rate,
															 krad_link->opus_bitrate,
															 OPUS_APPLICATION_AUDIO);
			framecnt = KRAD_MIN_OPUS_FRAME_SIZE;
			break;			
		default:
			failfast ("Krad Link Audio Encoder: Unknown Audio Codec");
	}
	
	krad_link->audio_encoder_ready = 1;
	
	while (krad_link->encoding) {

		while (krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[krad_link->channels - 1]) >= framecnt * 4) {

			if (krad_link->audio_codec == OPUS) {

				for (c = 0; c < krad_link->channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)samples[c], (framecnt * 4) );
					krad_opus_encoder_write (krad_link->krad_opus, c + 1, (char *)samples[c], framecnt * 4);
				}

				bytes = krad_opus_encoder_read (krad_link->krad_opus, buffer, &framecnt);
			}
			
			if (krad_link->audio_codec == FLAC) {
				
				for (c = 0; c < krad_link->channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)samples[c], (framecnt * 4) );
				}
			
				for (s = 0; s < framecnt; s++) {
					for (c = 0; c < krad_link->channels; c++) {
						interleaved_samples[s * krad_link->channels + c] = samples[c][s];
					}
				}
			
				bytes = krad_flac_encode (krad_link->krad_flac, interleaved_samples, framecnt, buffer);
			}			
			
			if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {
	
				while (bytes > 0) {
					
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&framecnt, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
					
					bytes = 0;
					
					if (krad_link->audio_codec == OPUS) {
						bytes = krad_opus_encoder_read (krad_link->krad_opus, buffer, &framecnt);
					}
				}
			}
			
			if (krad_link->audio_codec == VORBIS) {
			
				unsigned char *vorbis_buffer;
				float **float_buffer;

				krad_vorbis_encoder_prepare (krad_link->krad_vorbis, framecnt, &float_buffer);
							
				for (c = 0; c < krad_link->channels; c++) {
					krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)float_buffer[c], framecnt * 4);
				}			
			
				krad_vorbis_encoder_wrote (krad_link->krad_vorbis, framecnt);

				bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);

				while (bytes > 0) {
				
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
					krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)vorbis_buffer, bytes);
					
					bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
				}
			}
		}

		/* Wait for available audio to encode */

		while (krad_ringbuffer_read_space (krad_link->audio_input_ringbuffer[krad_link->channels - 1]) < framecnt * 4) {
			usleep (5000);
			if (krad_link->encoding == 3) {
				break;
			}
		}
		
		if ((krad_link->encoding == 3) && 
			(krad_ringbuffer_read_space (krad_link->audio_input_ringbuffer[krad_link->channels - 1]) < framecnt * 4)) {
				break;
		}
	}

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, mixer_portgroup);
	
	
	if (krad_link->krad_vorbis != NULL) {
		krad_vorbis_encoder_destroy (krad_link->krad_vorbis);
		krad_link->krad_vorbis = NULL;
	}
	
	if (krad_link->krad_flac != NULL) {
		krad_flac_encoder_destroy (krad_link->krad_flac);
		krad_link->krad_flac = NULL;
	}

	if (krad_link->krad_opus != NULL) {
		krad_opus_encoder_destroy (krad_link->krad_opus);
		krad_link->krad_opus = NULL;
	}
	
	krad_link->encoding = 4;
	
	for (c = 0; c < krad_link->channels; c++) {
		free (krad_link->samples[c]);
		free (samples[c]);
		krad_ringbuffer_free (krad_link->audio_input_ringbuffer[c]);		
	}	
	
	free (interleaved_samples);
	free (buffer);
	
	printk ("Audio encoding thread exiting");	
	
	return NULL;
}

void *stream_output_thread (void *arg) {

	krad_system_set_thread_name ("kr_stream_out");
	
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
	int seen_passthu_keyframe;
	int initial_passthu_frames_skipped;
	krad_frame_t *krad_frame;

	krad_transmission = NULL;
	krad_frame = NULL;
	audio_frames_per_video_frame = 0;
	audio_frames_muxed = 0;
	video_frames_muxed = 0;

	seen_passthu_keyframe = 0;
	initial_passthu_frames_skipped = 0;

	printk ("Output/Muxing thread starting");
	
	if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

		if (krad_link->audio_codec == OPUS) {
			audio_frames_per_video_frame = krad_link->krad_radio->krad_mixer->sample_rate / DEFAULT_FPS;
		} else {
			audio_frames_per_video_frame = krad_link->krad_radio->krad_mixer->sample_rate / DEFAULT_FPS;
			//audio_frames_per_video_frame = 1602;
		}
	}

	packet = malloc (2000000);
	
	if (krad_link->host[0] != '\0') {
	
		if ((strcmp(krad_link->host, "transmitter") == 0) &&
			(krad_link->krad_linker->krad_transmitter->listening == 1)) {
			
			krad_transmission = krad_transmitter_transmission_create (krad_link->krad_linker->krad_transmitter,
																	  krad_link->mount + 1,
																	  krad_link_select_mimetype(krad_link->mount + 1));

			krad_link->port = krad_link->krad_linker->krad_transmitter->port;

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
		if (krad_link->video_passthru == 1) {
		
			if (krad_link->video_codec == MJPEG) {
				seen_passthu_keyframe = 1;
			}
		
			krad_link->krad_compositor_port = krad_compositor_passthru_port_create (krad_link->krad_radio->krad_compositor,
																				 "passthruStreamOut",
																				 OUTPUT);
		}
		
		if ((krad_link->video_codec == VP8) || (krad_link->video_codec == MJPEG)) {
		
			krad_link->video_track = krad_container_add_video_track (krad_link->krad_container,
																	 krad_link->video_codec,
																	 krad_link->encoding_fps_numerator,
																	 krad_link->encoding_fps_denominator,
																	 krad_link->encoding_width,
																	 krad_link->encoding_height);
		}
		
		
		if (krad_link->video_codec == THEORA) {
		
			usleep (50000);
		
			krad_link->video_track = 
			krad_container_add_video_track_with_private_data (krad_link->krad_container, 
														      krad_link->video_codec,
														   	  krad_link->encoding_fps_numerator,
															  krad_link->encoding_fps_denominator,
															  krad_link->encoding_width,
															  krad_link->encoding_height,
															  &krad_link->krad_theora_encoder->krad_codec_header);
		
		}
		
		if (krad_link->video_codec == H264) {
		
			if (krad_link->video_passthru == 1) {
				krad_link->krad_x264_encoder = krad_x264_encoder_create (krad_link->encoding_width, 
																			 krad_link->encoding_height,
																			 krad_link->encoding_fps_numerator,
																			 krad_link->encoding_fps_denominator,
																			 krad_link->vp8_bitrate);
			} else {
				usleep (50000);
			}
			krad_link->video_track = 
			krad_container_add_video_track_with_private_data (krad_link->krad_container, 
														      krad_link->video_codec,
														   	  krad_link->encoding_fps_numerator,
															  krad_link->encoding_fps_denominator,
															  krad_link->encoding_width,
															  krad_link->encoding_height,
															  &krad_link->krad_x264_encoder->krad_codec_header);
		
			if (krad_link->video_passthru == 1) {
				krad_x264_encoder_destroy (krad_link->krad_x264_encoder);	
			}

		}
		
	}
	
	if (krad_link->av_mode != VIDEO_ONLY) {
	
		switch (krad_link->audio_codec) {
			case VORBIS:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container,
																		 krad_link->audio_codec,
																		 krad_link->krad_radio->krad_mixer->sample_rate,
																		 krad_link->channels, 
																		 &krad_link->krad_vorbis->krad_codec_header);
				break;
			case FLAC:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container,
																		 krad_link->audio_codec,
																		 krad_link->krad_radio->krad_mixer->sample_rate,
																		 krad_link->channels,
																		 &krad_link->krad_flac->krad_codec_header);
				break;
			case OPUS:
				krad_link->audio_track = krad_container_add_audio_track (krad_link->krad_container,
																		 krad_link->audio_codec,
																		 krad_link->krad_radio->krad_mixer->sample_rate,
																		 krad_link->channels, 
																		 &krad_link->krad_opus->krad_codec_header);
				break;
			default:
				failfast ("Unknown audio codec");
		}
	
	}
	
	printk ("Output/Muxing thread waiting..");
		
	while ( krad_link->encoding ) {

		if (krad_link->encoding == 4) {
			break;
		}

		if ((krad_link->av_mode != AUDIO_ONLY) && (krad_link->video_passthru == 0)) {
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
		
		if (krad_link->video_passthru == 1) {

			krad_frame = krad_compositor_port_pull_frame (krad_link->krad_compositor_port);

			if (krad_frame != NULL) {

				if (krad_link->video_codec == H264) {
					keyframe = krad_x264_is_keyframe ((unsigned char *)krad_frame->pixels);
					if (seen_passthu_keyframe == 0) {
						if (keyframe == 1) {
							seen_passthu_keyframe = 1;
							printk("Got first h264 passthru keyframe, skipped %d frames before it",
								   initial_passthu_frames_skipped);
						} else {
							initial_passthu_frames_skipped++;
						}
					}
				}
				
				if (krad_link->video_codec == MJPEG) {
					if (video_frames_muxed % 30 == 0) {
						keyframe = 1;
					} else {
						keyframe = 0;
					}				
				}
				
				if (seen_passthu_keyframe == 1) {
					krad_container_add_video (krad_link->krad_container,
											  krad_link->video_track, 
							 (unsigned char *)krad_frame->pixels,
											  krad_frame->encoded_size,
											  keyframe);
					video_frames_muxed++;
				}
				
				krad_framepool_unref_frame (krad_frame);
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
		
		if ((krad_link->av_mode == VIDEO_ONLY) && (krad_link->video_passthru == 0)) {
		
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
		
		usleep (6000);		
		
		//krad_ebml_write_tag (krad_link->krad_ebml, "test tag 1", "monkey 123");
	}

	krad_container_destroy (krad_link->krad_container);
	
	free (packet);
	
	if (krad_link->video_passthru == 1) {
		krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);
	}

	if (krad_transmission != NULL) {
		krad_transmitter_transmission_destroy (krad_transmission);
	}

	printk ("Output/Muxing thread exiting");
	
	return NULL;
	
}


void *udp_output_thread(void *arg) {

	krad_system_set_thread_name ("kr_udpout");

	printk ("UDP Output thread starting");

	krad_link_t *krad_link = (krad_link_t *)arg;

	unsigned char *buffer;
	int count;
	int packet_size;
	int frames;
	//uint64_t frames_big;
	
	//frames_big = 0;
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
				//frames_big = frames;
				//memcpy (buffer, &frames_big, 8);
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)buffer, packet_size);

				krad_slicer_sendto (krad_link->krad_slicer, buffer, packet_size, 1, krad_link->host, krad_link->port);
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

void *krad_link_run_thread (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_system_set_thread_name ("kradlink");

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

void *stream_input_thread (void *arg) {

	krad_system_set_thread_name ("kr_stream_in");

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
		//printk ("packet track %d timecode: %zu size %d", current_track, packet_timecode, packet_size);
		if ((packet_size <= 0) && (packet_timecode == 0) && ((video_packets + audio_packets) > 20))  {
			//printk ("stream input thread packet size was: %d", packet_size);
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
		
		if ((track_codecs[current_track] == VP8) || (track_codecs[current_track] == THEORA)) {

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
	
	
	krad_link->playing = 3;
	
	
	printk ("");
	printk ("Input/Demuxing thread exiting");
	
	krad_container_destroy (krad_link->krad_container);
	
	free (buffer);
	free (header_buffer);
	return NULL;

}

void *udp_input_thread(void *arg) {

	krad_system_set_thread_name ("kr_udp_in");

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
	
	opus_temp = krad_opus_encoder_create (2, krad_link->krad_radio->krad_mixer->sample_rate, 110000, 
										 OPUS_APPLICATION_AUDIO);
										 
	opus_header_size = opus_temp->header_data_size;
	memcpy (opus_header, opus_temp->header_data, opus_header_size);
	krad_opus_encoder_destroy(opus_temp);
	
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

void *video_decoding_thread (void *arg) {

	krad_system_set_thread_name ("kr_video_dec");

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
	int port_updated;
	
	for (h = 0; h < 3; h++) {
		header[h] = malloc(100000);
		header_len[h] = 0;
	}

	port_updated = 0;
	bytes = 0;
	buffer = malloc(3000000);
	
	krad_link->last_video_codec = NOCODEC;
	krad_link->video_codec = NOCODEC;
	
	krad_link->krad_framepool = krad_framepool_create ( krad_link->composite_width,
														krad_link->composite_height,
														DEFAULT_CAPTURE_BUFFER_FRAMES);
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "VidDecIn",
																   INPUT,
																   krad_link->composite_width,
																   krad_link->composite_height);
	
	
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
			}
	
			if (krad_link->video_codec == NOCODEC) {
				krad_link->last_video_codec = krad_link->video_codec;
				continue;
			}
	
			if (krad_link->video_codec == VP8) {
				krad_link->krad_vpx_decoder = krad_vpx_decoder_create();
				port_updated = 0;
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

				krad_compositor_port_set_io_params (krad_link->krad_compositor_port,
													krad_link->krad_theora_decoder->width,
													krad_link->krad_theora_decoder->height);
				port_updated = 1;													
			}
		}

		krad_link->last_video_codec = krad_link->video_codec;
	
		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 12) && (!krad_link->destroy)) {
			usleep(10000);
		}
	
		krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)&timecode, 8);
	
		krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < bytes) && (!krad_link->destroy)) {
			usleep(10000);
		}
		
		krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)buffer, bytes);
		
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
		while ((krad_frame == NULL) && (!krad_link->destroy)) {
			usleep(10000);
			krad_frame = krad_framepool_getframe (krad_link->krad_framepool);				
		}

		if (krad_link->destroy) {
			break;
		}		
		
		if ((krad_link->playing == 0) && (krad_link->krad_compositor_port->start_timecode != 1)) {
			krad_link->playing = 1;
		}
		
		if (krad_link->video_codec == THEORA) {
		
			krad_theora_decoder_decode (krad_link->krad_theora_decoder, buffer, bytes);		
			krad_theora_decoder_timecode (krad_link->krad_theora_decoder, &timecode2);			
			//printk ("timecode1: %zu timecode2: %zu", timecode, timecode2);
			timecode = timecode2;

			krad_frame->format = PIX_FMT_YUV420P;

			krad_frame->yuv_pixels[0] = krad_link->krad_theora_decoder->ycbcr[0].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[0].stride);
			krad_frame->yuv_pixels[1] = krad_link->krad_theora_decoder->ycbcr[1].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[1].stride);
			krad_frame->yuv_pixels[2] = krad_link->krad_theora_decoder->ycbcr[2].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[2].stride);
	
			krad_frame->yuv_strides[0] = krad_link->krad_theora_decoder->ycbcr[0].stride;
			krad_frame->yuv_strides[1] = krad_link->krad_theora_decoder->ycbcr[1].stride;
			krad_frame->yuv_strides[2] = krad_link->krad_theora_decoder->ycbcr[2].stride;

			krad_compositor_port_push_yuv_frame (krad_link->krad_compositor_port, krad_frame);

		}
			
		if (krad_link->video_codec == VP8) {

			krad_vpx_decoder_decode (krad_link->krad_vpx_decoder, buffer, bytes);
				
			if (krad_link->krad_vpx_decoder->img != NULL) {
				
				if (port_updated == 0) {
					krad_compositor_port_set_io_params (krad_link->krad_compositor_port,
														krad_link->krad_vpx_decoder->width,
														krad_link->krad_vpx_decoder->height);
																	
					port_updated = 1;
				}

				krad_frame->format = PIX_FMT_YUV420P;

				krad_frame->yuv_pixels[0] = krad_link->krad_vpx_decoder->img->planes[0];
				krad_frame->yuv_pixels[1] = krad_link->krad_vpx_decoder->img->planes[1];
				krad_frame->yuv_pixels[2] = krad_link->krad_vpx_decoder->img->planes[2];
	
				krad_frame->yuv_strides[0] = krad_link->krad_vpx_decoder->img->stride[0];
				krad_frame->yuv_strides[1] = krad_link->krad_vpx_decoder->img->stride[1];
				krad_frame->yuv_strides[2] = krad_link->krad_vpx_decoder->img->stride[2];
				
				krad_compositor_port_push_yuv_frame (krad_link->krad_compositor_port, krad_frame);

			}
		}
		
		krad_frame->timecode = timecode;
		//printk ("frame timecode: %zu", krad_frame->timecode);

		krad_framepool_unref_frame (krad_frame);		
		
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

	krad_system_set_thread_name ("kr_audio_dec");

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
	
	krad_resample_ring_t *krad_resample_ring[KRAD_MIXER_MAX_CHANNELS];
	
	/* SET UP */
	
	krad_link->channels = 2;

	for (h = 0; h < 3; h++) {
		header[h] = malloc(100000);
		header_len[h] = 0;
	}

	for (c = 0; c < krad_link->channels; c++) {
		krad_resample_ring[c] = krad_resample_ring_create (4000000, 48000,
														   krad_link->krad_linker->krad_radio->krad_mixer->sample_rate);

		krad_link->audio_output_ringbuffer[c] = krad_resample_ring[c]->krad_ringbuffer;
		//krad_link->audio_output_ringbuffer[c] = krad_ringbuffer_create (4000000);
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


		/* THE FOLLOWING IS WHERE WE ENSURE WE ARE ON THE RIGHT CODEC AND READ HEADERS IF NEED BE */

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
					krad_opus_decoder_destroy (krad_link->krad_opus);
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
				for (c = 0; c < krad_link->channels; c++) {
					krad_resample_ring_set_input_sample_rate (krad_resample_ring[c], krad_link->krad_flac->sample_rate);
				}
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

				for (c = 0; c < krad_link->channels; c++) {
					krad_resample_ring_set_input_sample_rate (krad_resample_ring[c], krad_link->krad_vorbis->sample_rate);
				}

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
				krad_link->krad_opus = krad_opus_decoder_create (header[0], header_len[0], krad_link->krad_radio->krad_mixer->sample_rate);

				for (c = 0; c < krad_link->channels; c++) {
					krad_resample_ring_set_input_sample_rate (krad_resample_ring[c], krad_link->krad_radio->krad_mixer->sample_rate);
				}

			}
		}

		krad_link->last_audio_codec = krad_link->audio_codec;
	
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
			usleep(4000);
		}
	
		krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
		
		while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < bytes) && (!krad_link->destroy)) {
			usleep(1000);
		}
		
		krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)buffer, bytes);
		
		/* DECODING HAPPENS HERE */
		
		if (krad_link->audio_codec == VORBIS) {
			krad_vorbis_decoder_decode (krad_link->krad_vorbis, buffer, bytes);
			
			len = 1;
			
			while (len ) {
			
				len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 0, (char *)samples[0], 512);

				if (len) {
				
					while ((krad_resample_ring_write_space (krad_resample_ring[0]) < len) && (!krad_link->destroy)) {
						usleep(25000);
					}
				
					//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[0], (char *)samples[0], len);
					krad_resample_ring_write (krad_resample_ring[0], (unsigned char *)samples[0], len);										
				}

				len = krad_vorbis_decoder_read_audio (krad_link->krad_vorbis, 1, (char *)samples[1], 512);

				if (len) {

					while ((krad_resample_ring_write_space (krad_resample_ring[1]) < len) && (!krad_link->destroy)) {
						//printk ("wait!");
						usleep(25000);
					}

					//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[1], (char *)samples[1], len);
					krad_resample_ring_write (krad_resample_ring[1], (unsigned char *)samples[1], len);					
				}
			
			}
		}
			
		if (krad_link->audio_codec == FLAC) {

			audio_frames = krad_flac_decode (krad_link->krad_flac, buffer, bytes, samples);
			
			for (c = 0; c < krad_link->channels; c++) {
				//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[c], (char *)samples[c], audio_frames * 4);
				krad_resample_ring_write (krad_resample_ring[c], (unsigned char *)samples[c], audio_frames * 4);
			}
		}
			
		if (krad_link->audio_codec == OPUS) {

			krad_opus_decoder_write (krad_link->krad_opus, buffer, bytes);
			
			bytes = -1;

			while (bytes != 0) {
				for (c = 0; c < 2; c++) {
					bytes = krad_opus_decoder_read (krad_link->krad_opus, c + 1, (char *)audio, 120 * 4);
					if (bytes) {
						if ((bytes / 4) != 120) {
							failfast ("uh oh crazyto");
						}

						while ((krad_resample_ring_write_space (krad_resample_ring[c]) < bytes) &&
							   (!krad_link->destroy)) {
								usleep(20000);
						}

						//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[c], (char *)audio, bytes);
						krad_resample_ring_write (krad_resample_ring[c], (unsigned char *)audio, bytes);						

					}
				}
			}
		}
	}
	
	/* ITS ALL OVER */
	
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
		krad_opus_decoder_destroy(krad_link->krad_opus);
		krad_link->krad_opus = NULL;
	}

	free (buffer);
	free (audio);
	
	for (c = 0; c < krad_link->channels; c++) {
		free(krad_link->samples[c]);
		free(samples[c]);
		//krad_ringbuffer_free ( krad_link->audio_output_ringbuffer[c] );		
		krad_resample_ring_destroy ( krad_resample_ring[c] );		
	}	

	for (h = 0; h < 3; h++) {
		free(header[h]);
	}

	printk ("Audio decoding thread exiting");

	return NULL;

}

int krad_link_decklink_video_callback (void *arg, void *buffer, int length) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	int stride;

	stride = krad_link->capture_width + (krad_link->capture_width/2) * 2;

	//printk ("krad link decklink frame received %d bytes", length);

	krad_frame_t *krad_frame;

	krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

	if (krad_frame != NULL) {

		krad_frame->format = PIX_FMT_UYVY422;

		krad_frame->yuv_pixels[0] = buffer;
		krad_frame->yuv_pixels[1] = NULL;
		krad_frame->yuv_pixels[2] = NULL;

		krad_frame->yuv_strides[0] = stride;
		krad_frame->yuv_strides[1] = 0;
		krad_frame->yuv_strides[2] = 0;
		krad_frame->yuv_strides[3] = 0;

		krad_compositor_port_push_yuv_frame (krad_link->krad_compositor_port, krad_frame);

		krad_framepool_unref_frame (krad_frame);

		//krad_compositor_process (krad_link->krad_linker->krad_radio->krad_compositor);

	} else {
	
		//failfast ("Krad Decklink underflow");
	
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

	for (c = 0; c < 2; c++) {
		krad_link_int16_to_float ( krad_link->krad_decklink->samples[c], (char *)buffer + (c * 2), frames, 4);
		krad_ringbuffer_write (krad_link->audio_capture_ringbuffer[c], (char *)krad_link->krad_decklink->samples[c], frames * 4);
	}

	krad_link->audio_frames_captured += frames;
	
	if (krad_mixer_get_pusher(krad_link->krad_radio->krad_mixer) == DECKLINKAUDIO) {
		krad_mixer_process (frames, krad_link->krad_radio->krad_mixer);
	}
	
	return 0;

}

void krad_link_start_decklink_capture (krad_link_t *krad_link) {

	krad_link->krad_decklink = krad_decklink_create (krad_link->device);
	krad_decklink_set_video_mode (krad_link->krad_decklink, krad_link->capture_width, krad_link->capture_height,
								  krad_link->fps_numerator, krad_link->fps_denominator);
	
	krad_decklink_set_audio_input (krad_link->krad_decklink, krad_link->audio_input);
	
	krad_link->krad_mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->krad_decklink->simplename, INPUT, 2, 
														  krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);	
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor, krad_link->krad_decklink->simplename, INPUT,
																	krad_link->capture_width, krad_link->capture_height);	
	
	
	krad_link->krad_decklink->callback_pointer = krad_link;
	krad_link->krad_decklink->audio_frames_callback = krad_link_decklink_audio_callback;
	krad_link->krad_decklink->video_frame_callback = krad_link_decklink_video_callback;
	
	krad_decklink_set_verbose (krad_link->krad_decklink, verbose);
	
	if (krad_mixer_has_pusher(krad_link->krad_radio->krad_mixer) == 0) {
		//	krad_mixer_set_pusher (krad_link->krad_radio->krad_mixer, DECKLINKAUDIO);
	}
	
	krad_decklink_start (krad_link->krad_decklink);
}

void krad_link_stop_decklink_capture (krad_link_t *krad_link) {

	if (krad_link->krad_decklink != NULL) {
		krad_decklink_destroy ( krad_link->krad_decklink );
		krad_link->krad_decklink = NULL;
	}
	
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
		for (c = 0; c < krad_link->channels; c++) {
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
	
	krad_ringbuffer_free ( krad_link->encoded_audio_ringbuffer );
	krad_ringbuffer_free ( krad_link->encoded_video_ringbuffer );

	if (krad_link->video_source == X11) {
		krad_x11_destroy (krad_link->krad_x11);
	}
	
	krad_tags_destroy (krad_link->krad_tags);	
	
	printk ("Krad Link Closed Clean");
	
	free (krad_link);
}

krad_link_t *krad_link_create (int linknum) {

	krad_link_t *krad_link;
	
	krad_link = calloc (1, sizeof(krad_link_t));

	krad_link->capture_buffer_frames = DEFAULT_CAPTURE_BUFFER_FRAMES;

	krad_link->encoding_fps_numerator = 0;
	krad_link->encoding_fps_denominator = 0;
	krad_link->encoding_width = 0;
	krad_link->encoding_height = 0;
	krad_link->capture_width = 0;
	krad_link->capture_height = 0;
	
	krad_link->vp8_bitrate = DEFAULT_VPX_BITRATE;
	
	strncpy(krad_link->device, DEFAULT_V4L2_DEVICE, sizeof(krad_link->device));
	strncpy(krad_link->alsa_capture_device, DEFAULT_ALSA_CAPTURE_DEVICE, sizeof(krad_link->alsa_capture_device));
	strncpy(krad_link->alsa_playback_device, DEFAULT_ALSA_PLAYBACK_DEVICE, sizeof(krad_link->alsa_playback_device));
	sprintf(krad_link->output, "%s/Videos/krad_link_%"PRIu64".webm", getenv ("HOME"), ktime());
	krad_link->port = 0;
	krad_link->operation_mode = CAPTURE;
	krad_link->video_codec = KRAD_LINK_DEFAULT_VIDEO_CODEC;
	krad_link->audio_codec = KRAD_LINK_DEFAULT_AUDIO_CODEC;
	krad_link->vorbis_quality = DEFAULT_VORBIS_QUALITY;
	krad_link->flac_bit_depth = KRAD_DEFAULT_FLAC_BIT_DEPTH;
	krad_link->opus_bitrate = KRAD_DEFAULT_OPUS_BITRATE;
	krad_link->theora_quality = DEFAULT_THEORA_QUALITY;
	krad_link->video_source = NOVIDEO;
	krad_link->transport_mode = TCP;

	sprintf (krad_link->sysname, "link%d", linknum);
	krad_link->krad_tags = krad_tags_create (krad_link->sysname);

	return krad_link;
}

void krad_link_activate (krad_link_t *krad_link) {

	int c;

	krad_compositor_get_resolution (krad_link->krad_radio->krad_compositor,
							  &krad_link->composite_width,
							  &krad_link->composite_height);

	if ((krad_link->encoding_fps_numerator == 0) || (krad_link->encoding_fps_denominator == 0)) {
		krad_compositor_get_frame_rate (krad_link->krad_radio->krad_compositor,
										&krad_link->encoding_fps_numerator,
										&krad_link->encoding_fps_denominator);
	}
	
	if ((krad_link->fps_numerator == 0) || (krad_link->fps_denominator == 0)) {
		krad_compositor_get_frame_rate (krad_link->krad_radio->krad_compositor,
										&krad_link->fps_numerator,
										&krad_link->fps_denominator);
	}	

	if ((krad_link->encoding_width == 0) || (krad_link->encoding_height == 0)) {
		krad_link->encoding_width = krad_link->composite_width;
		krad_link->encoding_height = krad_link->composite_height;
	}

	krad_link->encoded_audio_ringbuffer = krad_ringbuffer_create (2000000);
	krad_link->encoded_video_ringbuffer = krad_ringbuffer_create (6000000);

	if (krad_link->operation_mode == CAPTURE) {

		krad_link->channels = 2;

		if ((krad_link->capture_width == 0) || (krad_link->capture_height == 0)) {
			krad_link->capture_width = krad_link->composite_width;
			krad_link->capture_height = krad_link->composite_height;
		}
		//FIXME
		krad_link->capture_fps = DEFAULT_FPS;

		for (c = 0; c < krad_link->channels; c++) {
			krad_link->audio_capture_ringbuffer[c] = krad_ringbuffer_create (2000000);		
		}

		if (krad_link->video_source == X11) {
			if (krad_link->krad_x11 == NULL) {
				krad_link->krad_x11 = krad_x11_create();
			}
			
			if (krad_link->video_source == X11) {
				krad_link->krad_framepool = krad_framepool_create ( krad_link->krad_x11->screen_width,
																	krad_link->krad_x11->screen_height,
																	DEFAULT_CAPTURE_BUFFER_FRAMES);
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

                                krad_link->krad_framepool = krad_framepool_create ( krad_link->capture_width,
                                													krad_link->capture_height,
                                													DEFAULT_CAPTURE_BUFFER_FRAMES);


			krad_link->capturing = 1;
			// delay slightly so audio encoder ringbuffer ready?
			usleep (150000);
			krad_link_start_decklink_capture(krad_link);
		}
		
		if (krad_link->video_source == TEST) {

			krad_link->capturing = 1;
			pthread_create(&krad_link->video_capture_thread, NULL, test_screen_generator_thread, (void *)krad_link);
		}
		
		if (krad_link->video_source == INFO) {

			krad_link->capturing = 1;
			pthread_create(&krad_link->video_capture_thread, NULL, info_screen_generator_thread, (void *)krad_link);
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

		if ((krad_link->video_passthru == 0) && ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO))) {
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
	
		//TEMP KLUDGE
		krad_link->av_mode = AUDIO_ONLY;

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
		
		if ((krad_link->video_source == V4L2) || (krad_link->video_source == X11) || 
			(krad_link->video_source == TEST) || (krad_link->video_source == INFO)) {
			krad_link->av_mode = VIDEO_ONLY;
		}

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_CAPTURE_DEVICE) {
			printk ("hrm wtf");
		} else {
			//printk ("tag size %zu", ebml_data_size);
		}
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->device, ebml_data_size);
	
		if (krad_link->video_source == V4L2) {
			if (strlen(krad_link->device) == 0) {
				strncpy(krad_link->device, DEFAULT_V4L2_DEVICE, sizeof(krad_link->device));
			}
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
			if (ebml_id != EBML_ID_KRAD_LINK_LINK_CAPTURE_PASSTHRU_CODEC) {
				printk ("hrm wtf");
			} else {
				//printk ("tag size %zu", ebml_data_size);
			}
	
			string[0] = '\0';
			
			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
			if (strlen(string)) {
				krad_link->video_codec = krad_string_to_codec (string);
				if (krad_link->video_codec != NOCODEC) {
					krad_link->video_passthru = 1;
				}
			} else {
				krad_link->video_passthru = 0;
			}
		}
		if (krad_link->video_source == DECKLINK) {
			if (strlen(krad_link->device) == 0) {
				strncpy(krad_link->device, DEFAULT_DECKLINK_DEVICE, sizeof(krad_link->device));
			}
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
			if (ebml_id != EBML_ID_KRAD_LINK_LINK_CAPTURE_DECKLINK_AUDIO_INPUT) {
				printk ("hrm wtf");
			} else {
				//printk ("tag size %zu", ebml_data_size);
			}
	
			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->audio_input, ebml_data_size);	
		}		

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH) {
			printk ("hrm wtf2v");
		} else {
			krad_link->capture_width = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}
		
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT) {
			printk ("hrm wtf2v");
		} else {
			krad_link->capture_height = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR) {
			printk ("hrm wtf2v");
		} else {
			krad_link->fps_numerator = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}
		
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR) {
			printk ("hrm wtf2v");
		} else {
			krad_link->fps_denominator = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
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
			
			if ((krad_link->video_codec == H264) || (krad_link->video_codec == MJPEG)) {

				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC) {
					printk ("hrm wtf2v");
				} else {
					//printk ("tag name size %zu", ebml_data_size);
				}

				krad_link->video_passthru = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

				if (krad_link->video_codec == MJPEG) {
					//FIXME should be optional
					krad_link->video_passthru = 1;
				}

			}			
			
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
			
			
			if ((krad_link->video_codec == VP8) || (krad_link->video_codec == H264)) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
					printk ("hrm wtf2v");
				} else {
					krad_link->vp8_bitrate = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}
			
			if (krad_link->video_codec == THEORA) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY) {
					printk ("hrm wtf2v");
				} else {
					krad_link->theora_quality = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
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
			
			if (krad_link->audio_codec == VORBIS) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
				if (ebml_id == EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY) {
					krad_link->vorbis_quality = krad_ebml_read_float (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}

			if (krad_link->audio_codec == OPUS) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
				if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
					krad_link->opus_bitrate = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}
			
			if (krad_link->audio_codec == FLAC) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
				if (ebml_id == EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH) {
					krad_link->flac_bit_depth = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}			
			
			
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
			
		if (strstr(krad_link->mount, ".opus") != NULL) {
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
			
		if (strstr(krad_link->output, ".opus") != NULL) {
			krad_link->audio_codec = OPUS;
		}

		krad_link->transport_mode = FILESYSTEM;

	}
}


void krad_linker_broadcast_link_created ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link) {

	int c;
	uint64_t element;
	uint64_t subelement;

	for (c = 0; c < KRAD_IPC_SERVER_MAX_CLIENTS; c++) {
		//if ((krad_ipc_server->clients[c].confirmed == 1) && (krad_ipc_server->current_client != &krad_ipc_server->clients[c])) {
		if (krad_ipc_server->clients[c].broadcasts == 1) {
			if ((krad_ipc_server->current_client == &krad_ipc_server->clients[c]) || (krad_ipc_aquire_client (&krad_ipc_server->clients[c]))) {
				krad_ebml_start_element (krad_ipc_server->clients[c].krad_ebml2, EBML_ID_KRAD_LINK_MSG, &element);	
				krad_ebml_start_element (krad_ipc_server->clients[c].krad_ebml2, EBML_ID_KRAD_LINK_LINK_CREATED, &subelement);		
				krad_linker_link_to_ebml ( &krad_ipc_server->clients[c], krad_link);
				krad_ebml_finish_element (krad_ipc_server->clients[c].krad_ebml2, subelement);
				krad_ebml_finish_element (krad_ipc_server->clients[c].krad_ebml2, element);
				krad_ebml_write_sync (krad_ipc_server->clients[c].krad_ebml2);
				if (krad_ipc_server->current_client != &krad_ipc_server->clients[c]) {
					krad_ipc_release_client (&krad_ipc_server->clients[c]);
				}
				
			}
		}
	}
}

int krad_link_wait_codec_init (krad_link_t *krad_link) {

	//FIXME potential stuckness

	if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		if (krad_link->audio_codec == OPUS) {
			while (krad_link->krad_opus == NULL) {
				usleep (2000);
			}
		}
		if (krad_link->audio_codec == VORBIS) {
			while (krad_link->krad_vorbis == NULL) {
				usleep (2000);
			}
		}							
		if (krad_link->audio_codec == FLAC) {
			while (krad_link->krad_flac == NULL) {
				usleep (2000);
			}
		}
	}

	if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		if (krad_link->video_codec == THEORA) {
			while (krad_link->krad_theora_encoder == NULL) {
				usleep (2000);
			}
		}							
		if (krad_link->video_codec == VP8) {
			while (krad_link->krad_vpx_encoder == NULL) {
				usleep (2000);
			}
		}
	}
	
	return 0;
	
}


void krad_linker_link_to_ebml ( krad_ipc_server_client_t *client, krad_link_t *krad_link) {

	uint64_t link;

	krad_ebml_start_element (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK, &link);	

	krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_NUMBER, krad_link->link_num);

	switch ( krad_link->av_mode ) {

		case AUDIO_ONLY:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "audio only");
			break;
		case VIDEO_ONLY:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "video only");
			break;
		case AUDIO_AND_VIDEO:		
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "audio and video");
			break;
	}
	
	
	switch ( krad_link->operation_mode ) {

		case TRANSMIT:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "transmit");
			break;
		case RECORD:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "record");
			break;
		case PLAYBACK:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "playback");
			break;
		case RECEIVE:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "receive");
			break;
		case CAPTURE:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "capture");
			break;
		default:		
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "Other/Unknown");
			break;
	}
	
	if (krad_link->operation_mode == RECEIVE) {

		krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
								krad_link_transport_mode_to_string (krad_link->transport_mode));

		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
		}
	}	
	
	if (krad_link->operation_mode == CAPTURE) {
		krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE, krad_link_video_source_to_string (krad_link->video_source));
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
								krad_link_transport_mode_to_string (krad_link->transport_mode));
	
		if (krad_link->transport_mode == FILESYSTEM) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FILENAME, krad_link->input);
		}

		if (krad_link->transport_mode == TCP) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
		}
	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
		switch ( krad_link->av_mode ) {
			case AUDIO_ONLY:
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (krad_link->audio_codec));
				break;
			case VIDEO_ONLY:
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (krad_link->video_codec));
				break;
			case AUDIO_AND_VIDEO:		
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (krad_link->video_codec));				
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (krad_link->audio_codec));
				break;
		}

		if (krad_link->operation_mode == TRANSMIT) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
									krad_link_transport_mode_to_string (krad_link->transport_mode));
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
		}
		
		if (krad_link->operation_mode == RECORD) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FILENAME, krad_link->output);
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CHANNELS, 2);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_SAMPLE_RATE, 48000);
		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, krad_link->encoding_width);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, krad_link->encoding_height);						
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR, krad_link->encoding_fps_numerator);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR, krad_link->encoding_fps_denominator);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_COLOR_DEPTH, 420);							
		}		

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			if (krad_link->audio_codec == VORBIS) {
				krad_ebml_write_float (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY, krad_link->krad_vorbis->quality);
			}

			if (krad_link->audio_codec == FLAC) {
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, krad_link->krad_flac->bit_depth);
			}

			if (krad_link->audio_codec == OPUS) {
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, 
										krad_opus_signal_to_string (krad_opus_get_signal (krad_link->krad_opus)));
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, 
										krad_opus_bandwidth_to_string (krad_opus_get_bandwidth (krad_link->krad_opus)));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, krad_opus_get_bitrate (krad_link->krad_opus));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, krad_opus_get_complexity (krad_link->krad_opus));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, krad_opus_get_frame_size (krad_link->krad_opus));

				//EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
			}
		
		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			if (krad_link->video_codec == THEORA) {
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, krad_theora_encoder_quality_get (krad_link->krad_theora_encoder));
			}

			if (krad_link->video_codec == VP8) {
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, krad_vpx_encoder_bitrate_get (krad_link->krad_vpx_encoder));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER, krad_vpx_encoder_min_quantizer_get (krad_link->krad_vpx_encoder));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER, krad_vpx_encoder_max_quantizer_get (krad_link->krad_vpx_encoder));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE, krad_vpx_encoder_deadline_get (krad_link->krad_vpx_encoder));
			}
		}
	}
	
	krad_ebml_finish_element (client->krad_ebml2, link);

}

int krad_linker_handler ( krad_linker_t *krad_linker, krad_ipc_server_t *krad_ipc ) {

	krad_link_t *krad_link;

	uint32_t ebml_id;

	uint32_t command;
	uint64_t ebml_data_size;

	uint64_t element;
	uint64_t response;
	
	char string[256];	
	
	uint64_t bigint;
	//int regint;
	uint8_t tinyint;
	int k;
	int devices;
	//float floatval;
	
	string[0] = '\0';
	bigint = 0;
	k = 0;
	
	pthread_mutex_lock (&krad_linker->change_lock);	

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

		case EBML_ID_KRAD_LINK_CMD_LIST_LINKS:
			//printk ("krad linker handler! LIST_LINKS");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					printk ("Link %d Active: %s", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc->current_client, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
			
		case EBML_ID_KRAD_LINK_CMD_CREATE_LINK:
			//printk ("krad linker handler! CREATE_LINK");
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] == NULL) {

					krad_linker->krad_link[k] = krad_link_create (k);
					krad_link = krad_linker->krad_link[k];
					krad_link->link_num = k;
					krad_link->krad_radio = krad_linker->krad_radio;
					krad_link->krad_linker = krad_linker;

					krad_linker_ebml_to_link ( krad_ipc, krad_link );
					
					krad_link_run (krad_link);
				
					if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
						if (krad_link_wait_codec_init (krad_link) == 0) {
							krad_linker_broadcast_link_created ( krad_ipc, krad_link );
						}
					} else {
						krad_linker_broadcast_link_created ( krad_ipc, krad_link );
					}

					break;
				}
			}
			break;
		case EBML_ID_KRAD_LINK_CMD_DESTROY_LINK:
			tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
			k = tinyint;
			//printk ("krad linker handler! DESTROY_LINK: %d %u", k, tinyint);
			
			if (krad_linker->krad_link[k] != NULL) {
				krad_link_destroy (krad_linker->krad_link[k]);
				krad_linker->krad_link[k] = NULL;
			}
			
			krad_ipc_server_simple_number_broadcast ( krad_ipc,
													  EBML_ID_KRAD_LINK_MSG,
													  EBML_ID_KRAD_LINK_LINK_DESTROYED,
													  EBML_ID_KRAD_LINK_LINK_NUMBER,
											 		  k);			
			
			break;
		case EBML_ID_KRAD_LINK_CMD_UPDATE_LINK:
			//printk ("krad linker handler! UPDATE_LINK");

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	
			
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_NUMBER) {
			
				tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
				k = tinyint;
				//printk ("krad linker handler! UPDATE_LINK: %d %u", k, tinyint);
			
				if (krad_linker->krad_link[k] != NULL) {

					krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

					if (krad_linker->krad_link[k]->audio_codec == OPUS) {

						/*
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_APPLICATION) {
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							if (strncmp(string, "OPUS_APPLICATION_VOIP", 21) == 0) {
								krad_opus_set_application (krad_linker->krad_link[k]->krad_opus, OPUS_APPLICATION_VOIP);
							}
							if (strncmp(string, "OPUS_APPLICATION_AUDIO", 22) == 0) {
								krad_opus_set_application (krad_linker->krad_link[k]->krad_opus, OPUS_APPLICATION_AUDIO);
							}
							if (strncmp(string, "OPUS_APPLICATION_RESTRICTED_LOWDELAY", 36) == 0) {
								krad_opus_set_application (krad_linker->krad_link[k]->krad_opus, OPUS_APPLICATION_RESTRICTED_LOWDELAY);
							}
						}
						*/
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL) {
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

							krad_opus_set_signal (krad_linker->krad_link[k]->krad_opus, 
													krad_opus_string_to_signal(string));
						}
						
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) {
							
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							
							krad_opus_set_bandwidth (krad_linker->krad_link[k]->krad_opus, 
													krad_opus_string_to_bandwidth(string));
						}						

						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if ((bigint >= 500) && (bigint <= 512000)) {
								krad_opus_set_bitrate (krad_linker->krad_link[k]->krad_opus, bigint);
							}
						}
						
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if ((bigint >= 0) && (bigint <= 10)) {
								krad_opus_set_complexity (krad_linker->krad_link[k]->krad_opus, bigint);
							}
						}						
				
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							if ((bigint == 120) || (bigint == 240) || (bigint == 480) || (bigint == 960) || (bigint == 1920) || (bigint == 2880)) {
								krad_opus_set_frame_size (krad_linker->krad_link[k]->krad_opus, bigint);
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
					
					if (krad_linker->krad_link[k]->video_codec == THEORA) {
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							krad_theora_encoder_quality_set (krad_linker->krad_link[k]->krad_theora_encoder, bigint);
						}
					}				
					
					if (krad_linker->krad_link[k]->video_codec == VP8) {
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_bitrate_set (krad_linker->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_min_quantizer_set (krad_linker->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_max_quantizer_set (krad_linker->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_deadline_set (krad_linker->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}												
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_FORCE_KEYFRAME) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_want_keyframe (krad_linker->krad_link[k]->krad_vpx_encoder);
							}
						}
					}

					if ((ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) || (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL)) {

						krad_ipc_server_advanced_string_broadcast ( krad_ipc,
																  EBML_ID_KRAD_LINK_MSG,
																  EBML_ID_KRAD_LINK_LINK_UPDATED,
																  EBML_ID_KRAD_LINK_LINK_NUMBER,
														 		  k,
														 		  ebml_id,
														 		  string);

					} else {
						krad_ipc_server_advanced_number_broadcast ( krad_ipc,
																  EBML_ID_KRAD_LINK_MSG,
																  EBML_ID_KRAD_LINK_LINK_UPDATED,
																  EBML_ID_KRAD_LINK_LINK_NUMBER,
														 		  k,
														 		  ebml_id,
														 		  bigint);
					}
				}
			}
			
			break;
			
		case EBML_ID_KRAD_LINK_CMD_LIST_DECKLINK:
			printk ("krad linker handler! LIST_DECKLINK");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			
			devices = krad_decklink_detect_devices();

			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_DECKLINK_LIST, &element);
			krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_LIST_COUNT, devices);
			
			for (k = 0; k < devices; k++) {

				krad_decklink_get_device_name (k, string);
				krad_ebml_write_string (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_LINK_DECKLINK_DEVICE_NAME, string);
					
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
	
		case EBML_ID_KRAD_LINK_CMD_LIST_V4L2:
			printk ("krad linker handler! LIST_V4L2");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			
			devices = kradv4l2_detect_devices ();

			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_V4L2_LIST, &element);
			krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_LIST_COUNT, devices);
			
			for (k = 0; k < devices; k++) {

				if (kradv4l2_get_device_filename (k, string) > 0) {
					krad_ebml_write_string (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_LINK_V4L2_DEVICE_FILENAME, string);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
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

			krad_linker->krad_link[k] = krad_link_create (k);
			krad_link = krad_linker->krad_link[k];
			krad_link->krad_radio = krad_linker->krad_radio;
			krad_link->krad_linker = krad_linker;
			
			krad_tags_set_set_tag_callback (krad_link->krad_tags, krad_linker->krad_radio->krad_ipc, 
											(void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);
	
			
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

	krad_system_set_thread_name ("kr_lsn_client");

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

	krad_system_set_thread_name ("kr_listener");
	
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


