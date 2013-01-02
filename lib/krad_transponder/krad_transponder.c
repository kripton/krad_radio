#include "krad_transponder.h"

extern int verbose;

void krad_link_activate (krad_link_t *krad_link);
static void *krad_transponder_listening_thread (void *arg);
static void krad_transponder_listen_destroy_client (krad_transponder_listen_client_t *krad_transponder_listen_client);
static void krad_transponder_listen_create_client (krad_transponder_t *krad_transponder, int sd);
static void *krad_transponder_listen_client_thread (void *arg);

#ifndef __MACH__

void v4l2_capture_unit_create (void *arg) {
	//krad_system_set_thread_name ("kr_cap_v4l2");

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	printk ("Video capture creating..");
	
	krad_link->krad_v4l2 = krad_v4l2_create ();

	if (krad_link->video_codec != NOCODEC) {
		if (krad_link->video_codec == MJPEG) {
			krad_v4l2_mjpeg_mode (krad_link->krad_v4l2);
		}
		if (krad_link->video_codec == H264) {
			krad_v4l2_h264_mode (krad_link->krad_v4l2);
		}
	}

	krad_v4l2_open (krad_link->krad_v4l2, krad_link->device, krad_link->capture_width, 
				   krad_link->capture_height, 30);

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
	
	krad_v4l2_start_capturing (krad_link->krad_v4l2);

	printk ("Video capture started..");

}

int v4l2_capture_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  void *captured_frame = NULL;
  krad_frame_t *krad_frame;

  captured_frame = krad_v4l2_read (krad_link->krad_v4l2);		

  krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

  if (krad_link->video_passthru == 1) {
    memcpy (krad_frame->pixels, captured_frame, krad_link->krad_v4l2->encoded_size);
    krad_frame->encoded_size = krad_link->krad_v4l2->encoded_size;
    krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);
  } else {
    if (krad_link->video_codec == MJPEG) {
      krad_v4l2_mjpeg_to_rgb (krad_link->krad_v4l2, (unsigned char *)krad_frame->pixels,
                             captured_frame, krad_link->krad_v4l2->encoded_size);			
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
    }
  }

  krad_v4l2_frame_done (krad_link->krad_v4l2);
  krad_framepool_unref_frame (krad_frame);

  if (krad_link->video_passthru == 1) {
    krad_compositor_passthru_process (krad_link->krad_radio->krad_compositor);
  }

  return 0;
}

void v4l2_capture_unit_destroy (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_v4l2_stop_capturing (krad_link->krad_v4l2);
	krad_v4l2_close(krad_link->krad_v4l2);
	krad_v4l2_destroy(krad_link->krad_v4l2);

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	krad_link->encoding = 2;

	printk ("v4l2 capture unit destroy");
	
}

#endif

void x11_capture_unit_create (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_system_set_thread_name ("kr_x11_cap");
	
	printk ("X11 capture thread begins");
	
	krad_link->krad_x11 = krad_x11_create();
	
	if (krad_link->video_source == X11) {
		krad_link->krad_framepool = krad_framepool_create ( krad_link->krad_x11->screen_width,
															krad_link->krad_x11->screen_height,
															DEFAULT_CAPTURE_BUFFER_FRAMES);
	}
	
	krad_x11_enable_capture (krad_link->krad_x11, krad_link->krad_x11->screen_width, krad_link->krad_x11->screen_height);
	krad_link->capturing = 1;
	
	krad_link->krad_ticker = krad_ticker_create (krad_link->krad_radio->krad_compositor->frame_rate_numerator,
									  krad_link->krad_radio->krad_compositor->frame_rate_denominator);
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
																   "X11In",
																   INPUT,
																   krad_link->krad_x11->screen_width, krad_link->krad_x11->screen_height);

	krad_ticker_start (krad_link->krad_ticker);	

}

int x11_capture_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;
  
	krad_frame_t *krad_frame;
  
	krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

	krad_x11_capture (krad_link->krad_x11, (unsigned char *)krad_frame->pixels);
	
	krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);

	krad_framepool_unref_frame (krad_frame);

	krad_ticker_wait (krad_link->krad_ticker);

  return 0;
}

void x11_capture_unit_destroy (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

	krad_ticker_destroy (krad_link->krad_ticker);

  krad_x11_destroy (krad_link->krad_x11);

	printk ("X11 capture thread exited");
	
}


void video_encoding_unit_create (void *arg) {
	//krad_system_set_thread_name ("kr_video_enc");

  krad_link_t *krad_link = (krad_link_t *)arg;

  //printk ("Video encoding thread started");

  krad_link->color_depth = PIX_FMT_YUV420P;

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
      krad_link->krad_vpx_encoder->cfg.rc_min_quantizer = 5;
      krad_link->krad_vpx_encoder->cfg.rc_max_quantizer = 35;					
    }

    krad_vpx_encoder_config_set (krad_link->krad_vpx_encoder, &krad_link->krad_vpx_encoder->cfg);

    krad_vpx_encoder_deadline_set (krad_link->krad_vpx_encoder, 13333);

    krad_vpx_encoder_print_config (krad_link->krad_vpx_encoder);
  }

  if (krad_link->video_codec == THEORA) {
    krad_link->krad_theora_encoder = krad_theora_encoder_create (krad_link->encoding_width, 
                                                                 krad_link->encoding_height,
                                                                 krad_link->encoding_fps_numerator,
                                                                 krad_link->encoding_fps_denominator,
                                                                 krad_link->color_depth,
                                                                 krad_link->theora_quality);
  }

  if (krad_link->video_codec == Y4M) {
    krad_link->krad_y4m = krad_y4m_create (krad_link->encoding_width, krad_link->encoding_height, krad_link->color_depth);
  }

  if (krad_link->video_codec == KVHS) {
    krad_link->krad_vhs = krad_vhs_create_encoder (krad_link->encoding_width, krad_link->encoding_height);
  }

  /* COMPOSITOR CONNECTION */

  krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
                                                                 "VIDEnc",
                                                                 OUTPUT,
                                                                 krad_link->encoding_width, 
                                                                 krad_link->encoding_height);

  krad_link->krad_compositor_port_fd = krad_compositor_port_get_fd (krad_link->krad_compositor_port);

}

int video_encoding_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  int ret;
  char buffer[1];
  krad_frame_t *krad_frame;
  void *video_packet;
  int keyframe;
  int packet_size;
  char keyframe_char[1];
  unsigned char *planes[3];
  int strides[3];

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

  if (krad_link->video_codec == Y4M) {
    planes[0] = krad_link->krad_y4m->planes[0];
    planes[1] = krad_link->krad_y4m->planes[1];
    planes[2] = krad_link->krad_y4m->planes[2];
    strides[0] = krad_link->krad_y4m->strides[0];
    strides[1] = krad_link->krad_y4m->strides[1];
    strides[2] = krad_link->krad_y4m->strides[2];
  }
  
  ret = read (krad_link->krad_compositor_port->socketpair[1], buffer, 1);
  if (ret != 1) {
    if (ret == 0) {
      printk ("Krad OTransponder: port read got EOF");
      return -1;
    }
    printk ("Krad OTransponder: port read unexpected read return value %d", ret);
  }

  if (krad_link->video_codec == KVHS) {
    krad_frame = krad_compositor_port_pull_frame (krad_link->krad_compositor_port);
  } else {
    krad_frame = krad_compositor_port_pull_yuv_frame (krad_link->krad_compositor_port, planes, strides, krad_link->color_depth);
  }

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

    if (krad_link->video_codec == KVHS) {

      keyframe_char[0] = 1;

      packet_size = krad_vhs_encode (krad_link->krad_vhs, (unsigned char *)krad_frame->pixels);

      krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
      krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
      krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)krad_link->krad_vhs->enc_buffer, packet_size);


    } else {

      if (krad_link->video_codec == Y4M) {

        keyframe_char[0] = 1;

        packet_size = krad_link->krad_y4m->frame_size + Y4M_FRAME_HEADER_SIZE;

        krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
        krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
        krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)Y4M_FRAME_HEADER, Y4M_FRAME_HEADER_SIZE);
        krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)krad_link->krad_y4m->planes[0], krad_link->krad_y4m->size[0]);
        krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)krad_link->krad_y4m->planes[1], krad_link->krad_y4m->size[1]);
        krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)krad_link->krad_y4m->planes[2], krad_link->krad_y4m->size[2]);
		
      } else {

        if ((packet_size) || (krad_link->video_codec == THEORA)) {

          //FIXME un needed memcpy

          keyframe_char[0] = keyframe;

          krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
          krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
          krad_ringbuffer_write (krad_link->encoded_video_ringbuffer, (char *)video_packet, packet_size);
        }
      }
    }
    krad_framepool_unref_frame (krad_frame);
	}
	
  return 0;
	
}
	
void video_encoding_unit_destroy (void *arg) {
	
  krad_link_t *krad_link = (krad_link_t *)arg;	
	
  void *video_packet;
  int keyframe;
  int packet_size;
  char keyframe_char[1];
	
  printk ("Video encoding unit destroying");	

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

  if (krad_link->video_codec == Y4M) {
    krad_y4m_destroy (krad_link->krad_y4m);	
  }

  if (krad_link->video_codec == KVHS) {
    krad_vhs_destroy (krad_link->krad_vhs);	
  }	

  // FIXME make shutdown sequence more pretty
  krad_link->encoding = 3;
  if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->audio_codec == NOCODEC)) {
    krad_link->encoding = 4;
  }

  printk ("Video encoding unit exited");
  
}


void krad_link_audio_samples_callback (int frames, void *userdata, float **samples) {

	krad_link_t *krad_link = (krad_link_t *)userdata;
	
	int c;
  int wrote;

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

    wrote = write (krad_link->socketpair[0], "0", 1);
    if (wrote != 1) {
      printk ("Krad Transponder: au port write unexpected write return value %d", wrote);
    }
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		krad_link->audio_frames_captured += frames;
		krad_ringbuffer_read (krad_link->audio_capture_ringbuffer[0], (char *)samples[0], frames * 4);
		krad_ringbuffer_read (krad_link->audio_capture_ringbuffer[1], (char *)samples[1], frames * 4);
	}	

}

void audio_encoding_unit_create (void *arg) {

	//krad_system_set_thread_name ("kr_audio_enc");

	krad_link_t *krad_link = (krad_link_t *)arg;

	int c;

	printk ("Audio unit create");
	
	krad_link->channels = 2;
	
	krad_link->au_interleaved_samples = malloc (8192 * 4 * KRAD_MIXER_MAX_CHANNELS);
	krad_link->au_buffer = malloc (300000);
	
	for (c = 0; c < krad_link->channels; c++) {
    krad_link->au_samples[c] = malloc (8192 * 4);
		krad_link->samples[c] = malloc (8192 * 4);
		krad_link->audio_input_ringbuffer[c] = krad_ringbuffer_create (2000000);		
	}
	
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, krad_link->socketpair)) {
    printk ("Krad Compositor: subunit could not create socketpair errno: %d", errno);
    return;
  }
	
	krad_link->mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, 
                                                            OUTPUT, DIRECT, krad_link->channels,
                                                            krad_link->krad_radio->krad_mixer->master_mix,
                                                            KRAD_LINK, krad_link, 0);		
		
	switch (krad_link->audio_codec) {
		case VORBIS:
			krad_link->krad_vorbis = krad_vorbis_encoder_create (krad_link->channels,
																 krad_link->krad_radio->krad_mixer->sample_rate,
																 krad_link->vorbis_quality);
		  krad_link->au_framecnt = KRAD_DEFAULT_VORBIS_FRAME_SIZE;
			break;
		case FLAC:
			krad_link->krad_flac = krad_flac_encoder_create (krad_link->channels,
															 krad_link->krad_radio->krad_mixer->sample_rate,
															 krad_link->flac_bit_depth);
      krad_link->au_framecnt = KRAD_DEFAULT_FLAC_FRAME_SIZE;
			break;
		case OPUS:
			krad_link->krad_opus = krad_opus_encoder_create (krad_link->channels,
															 krad_link->krad_radio->krad_mixer->sample_rate,
															 krad_link->opus_bitrate,
															 OPUS_APPLICATION_AUDIO);
		  krad_link->au_framecnt = KRAD_MIN_OPUS_FRAME_SIZE;
			break;			
		default:
			failfast ("Krad Link Audio Encoder: Unknown Audio Codec");
	}
	
	krad_link->audio_encoder_ready = 1;
	
}
	
int audio_encoding_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  int c;
  unsigned char *vorbis_buffer;
  float **float_buffer;
  int s;
  int bytes;
  int frames;
  
  int ret;
  char buffer[1];
  
  ret = read (krad_link->socketpair[1], buffer, 1);
  if (ret != 1) {
    if (ret == 0) {
      printk ("Krad AU Transponder: port read got EOF");
      return -1;
    }
    printk ("Krad AU Transponder: port read unexpected read return value %d", ret);
  }

  while (krad_ringbuffer_read_space(krad_link->audio_input_ringbuffer[krad_link->channels - 1]) >= krad_link->au_framecnt * 4) {

	  if (krad_link->audio_codec == OPUS) {

		  for (c = 0; c < krad_link->channels; c++) {
			  krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)krad_link->au_samples[c], (krad_link->au_framecnt * 4) );
			  krad_opus_encoder_write (krad_link->krad_opus, c + 1, (char *)krad_link->au_samples[c], krad_link->au_framecnt * 4);
		  }

		  bytes = krad_opus_encoder_read (krad_link->krad_opus, krad_link->au_buffer, &krad_link->au_framecnt);
	  }
	
	  if (krad_link->audio_codec == FLAC) {
		
		  for (c = 0; c < krad_link->channels; c++) {
			  krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)krad_link->au_samples[c], (krad_link->au_framecnt * 4) );
		  }
	
		  for (s = 0; s < krad_link->au_framecnt; s++) {
			  for (c = 0; c < krad_link->channels; c++) {
				  krad_link->au_interleaved_samples[s * krad_link->channels + c] = krad_link->au_samples[c][s];
			  }
		  }
	
		  bytes = krad_flac_encode (krad_link->krad_flac, krad_link->au_interleaved_samples, krad_link->au_framecnt, krad_link->au_buffer);
	  }			
	
	  if ((krad_link->audio_codec == FLAC) || (krad_link->audio_codec == OPUS)) {

		  while (bytes > 0) {
			
			  krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
			  krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&krad_link->au_framecnt, 4);
			  krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)krad_link->au_buffer, bytes);
			
			  bytes = 0;
			
			  if (krad_link->audio_codec == OPUS) {
				  bytes = krad_opus_encoder_read (krad_link->krad_opus, krad_link->au_buffer, &krad_link->au_framecnt);
			  }
		  }
	  }
	
	  if (krad_link->audio_codec == VORBIS) {
	
		  krad_vorbis_encoder_prepare (krad_link->krad_vorbis, krad_link->au_framecnt, &float_buffer);
					
		  for (c = 0; c < krad_link->channels; c++) {
			  krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)float_buffer[c], krad_link->au_framecnt * 4);
		  }			
	
		  krad_vorbis_encoder_wrote (krad_link->krad_vorbis, krad_link->au_framecnt);
      // DUPE BELOW
		  bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
		  while (bytes > 0) {
			  krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
			  krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
			  krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)vorbis_buffer, bytes);
			  bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
		  }
	  }
  }

  return 0;

}

void audio_encoding_unit_destroy (void *arg) {
	
  krad_link_t *krad_link = (krad_link_t *)arg;

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, krad_link->mixer_portgroup);
	
  int c;
  unsigned char *vorbis_buffer;
  int bytes;
  int frames;
	
	if (krad_link->krad_vorbis != NULL) {
	
    krad_vorbis_encoder_finish (krad_link->krad_vorbis);

    // DUPEY
		bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
		while (bytes > 0) {
			krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
			krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
			krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)vorbis_buffer, bytes);
			bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
		}
	
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
	
  close (krad_link->socketpair[0]);
  close (krad_link->socketpair[1]);
	
	for (c = 0; c < krad_link->channels; c++) {
		free (krad_link->samples[c]);
		free (krad_link->au_samples[c]);
		krad_ringbuffer_free (krad_link->audio_input_ringbuffer[c]);		
	}	
	
	free (krad_link->au_interleaved_samples);
	free (krad_link->au_buffer);
	
	printk ("Audio encoding thread exiting");	
	
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
	uint64_t video_frames_muxed;
	uint64_t audio_frames_muxed;
	float audio_frames_per_video_frame;
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
  	audio_frames_per_video_frame = krad_link->krad_radio->krad_mixer->sample_rate;
  	audio_frames_per_video_frame = audio_frames_per_video_frame / krad_link->encoding_fps_numerator;
  	audio_frames_per_video_frame = audio_frames_per_video_frame * krad_link->encoding_fps_denominator;
	}

  printk ("audio frames per video frame %f", audio_frames_per_video_frame);

	packet = malloc (6300000);
	
	if (krad_link->host[0] != '\0') {
	
		if ((strcmp(krad_link->host, "transmitter") == 0) &&
			(krad_link->krad_transponder->krad_transmitter->listening == 1)) {
			
			krad_transmission = krad_transmitter_transmission_create (krad_link->krad_transponder->krad_transmitter,
																	  krad_link->mount + 1,
																	  krad_link_select_mimetype(krad_link->mount + 1));

			krad_link->port = krad_link->krad_transponder->krad_transmitter->port;

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
		
		if (krad_link->video_codec == Y4M) {
		
      packet_size = krad_y4m_generate_header (packet, krad_link->encoding_width, krad_link->encoding_height,
                                              krad_link->encoding_fps_numerator, krad_link->encoding_fps_denominator, PIX_FMT_YUV420P);		

      krad_codec_header_t *krad_codec_header = calloc (1, sizeof(krad_codec_header_t));
      
      krad_codec_header->header_count = 1;
      krad_codec_header->header_combined = packet;
      krad_codec_header->header_combined_size = packet_size;
	    krad_codec_header->header[0] = packet;
      krad_codec_header->header_size[0] = packet_size;

			krad_link->video_track = 
			krad_container_add_video_track_with_private_data (krad_link->krad_container, 
														      krad_link->video_codec,
														   	  krad_link->encoding_fps_numerator,
															  krad_link->encoding_fps_denominator,
															  krad_link->encoding_width,
															  krad_link->encoding_height,
															  krad_codec_header);
															  
															  
		  free (krad_codec_header);
		
		}
		
		
		if ((krad_link->video_codec == VP8) || (krad_link->video_codec == MJPEG) || (krad_link->video_codec == KVHS) || (krad_link->video_codec == H264)) {
		
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
					keyframe = krad_v4l2_is_h264_keyframe ((unsigned char *)krad_frame->pixels);
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
	buffer = malloc (4096 * 2048);
	
	if (krad_link->host[0] != '\0') {
		krad_link->krad_container = krad_container_open_stream (krad_link->host, krad_link->port, krad_link->mount, NULL);
	} else {
		krad_link->krad_container = krad_container_open_file (krad_link->input, KRAD_IO_READONLY);
	}
	
	// create video codec output port
	// create audio codec output port
	
	
	// poll on container fd and control fd
	
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
		
		if ((track_codecs[current_track] == Y4M) || (track_codecs[current_track] == KVHS) || (track_codecs[current_track] == VP8) || (track_codecs[current_track] == THEORA)) {

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

void video_decoding_unit_create (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	krad_system_set_thread_name ("kr_video_dec");

	printk ("Video decoding thread starting");

  int h;
	
	for (h = 0; h < 3; h++) {
		krad_link->vu_header[h] = malloc(100000);
		krad_link->vu_header_len[h] = 0;
	}

	krad_link->vu_buffer = malloc(3000000);
	
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
	
	
}


int video_decoding_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;
  
	int bytes;
	uint64_t timecode;
	uint64_t timecode2;
	krad_frame_t *krad_frame;
	int port_updated;
	int h;
	
	port_updated = 0;
	bytes = 0;	

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
			if (krad_link->last_video_codec == KVHS) {
				krad_vhs_destroy (krad_link->krad_vhs);
				krad_link->krad_vhs = NULL;
			}				
			if (krad_link->last_video_codec == THEORA) {
				krad_theora_decoder_destroy (krad_link->krad_theora_decoder);
				krad_link->krad_theora_decoder = NULL;
			}
		}

		if (krad_link->video_codec == NOCODEC) {
			krad_link->last_video_codec = krad_link->video_codec;
			return 1;
		}

		if (krad_link->video_codec == VP8) {
			krad_link->krad_vpx_decoder = krad_vpx_decoder_create();
			port_updated = 0;
		}
		
		if (krad_link->video_codec == Y4M) {
			//krad_link->krad_vpx_decoder = krad_vpx_decoder_create();
			port_updated = 0;
		}
		
		if (krad_link->video_codec == KVHS) {
			krad_link->krad_vhs = krad_vhs_create_decoder ();
			port_updated = 0;
		}						

		if (krad_link->video_codec == THEORA) {
			
			for (h = 0; h < 3; h++) {

				while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < 4) && (!krad_link->destroy)) {
					usleep(10000);
				}

				krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)&krad_link->vu_header_len[h], 4);
	
				while ((krad_ringbuffer_read_space(krad_link->encoded_video_ringbuffer) < krad_link->vu_header_len[h]) && (!krad_link->destroy)) {
					usleep(10000);
				}
	
				krad_ringbuffer_read(krad_link->encoded_video_ringbuffer, (char *)krad_link->vu_header[h], krad_link->vu_header_len[h]);
			}

			printk ("Theora Header byte sizes: %d %d %d", krad_link->vu_header_len[0], krad_link->vu_header_len[1], krad_link->vu_header_len[2]);
			krad_link->krad_theora_decoder = krad_theora_decoder_create(krad_link->vu_header[0], krad_link->vu_header_len[0], krad_link->vu_header[1], krad_link->vu_header_len[1], krad_link->vu_header[2], krad_link->vu_header_len[2]);

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
	
	krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)krad_link->vu_buffer, bytes);
	
	krad_frame = krad_framepool_getframe (krad_link->krad_framepool);
	while ((krad_frame == NULL) && (!krad_link->destroy)) {
		usleep(10000);
		krad_frame = krad_framepool_getframe (krad_link->krad_framepool);				
	}

	if (krad_link->destroy) {
		return 1;
	}		
	
	if ((krad_link->playing == 0) && (krad_link->krad_compositor_port->start_timecode != 1)) {
		krad_link->playing = 1;
	}
	
	if (krad_link->video_codec == THEORA) {
	
		krad_theora_decoder_decode (krad_link->krad_theora_decoder, krad_link->vu_buffer, bytes);		
		krad_theora_decoder_timecode (krad_link->krad_theora_decoder, &timecode2);			
		//printk ("timecode1: %zu timecode2: %zu", timecode, timecode2);
		timecode = timecode2;

		krad_frame->format = krad_link->krad_theora_decoder->color_depth;

		krad_frame->yuv_pixels[0] = krad_link->krad_theora_decoder->ycbcr[0].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[0].stride);
		krad_frame->yuv_pixels[1] = krad_link->krad_theora_decoder->ycbcr[1].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[1].stride);
		krad_frame->yuv_pixels[2] = krad_link->krad_theora_decoder->ycbcr[2].data + (krad_link->krad_theora_decoder->offset_y * krad_link->krad_theora_decoder->ycbcr[2].stride);

		krad_frame->yuv_strides[0] = krad_link->krad_theora_decoder->ycbcr[0].stride;
		krad_frame->yuv_strides[1] = krad_link->krad_theora_decoder->ycbcr[1].stride;
		krad_frame->yuv_strides[2] = krad_link->krad_theora_decoder->ycbcr[2].stride;
    krad_frame->timecode = timecode;
		krad_compositor_port_push_yuv_frame (krad_link->krad_compositor_port, krad_frame);

	}
		
	if (krad_link->video_codec == VP8) {

		krad_vpx_decoder_decode (krad_link->krad_vpx_decoder, krad_link->vu_buffer, bytes);
			
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
	    krad_frame->timecode = timecode;
			krad_compositor_port_push_yuv_frame (krad_link->krad_compositor_port, krad_frame);

		}
	}
	
	if (krad_link->video_codec == KVHS) {

    krad_vhs_decode (krad_link->krad_vhs, krad_link->vu_buffer, (unsigned char *)krad_frame->pixels);	
			
		if (krad_link->krad_vhs->width != 0) {
			if (port_updated == 0) {
        printk ("got vhs res: %dx%d", krad_link->krad_vhs->width, krad_link->krad_vhs->height);
				krad_compositor_port_set_io_params (krad_link->krad_compositor_port,
													krad_link->krad_vhs->width,
													krad_link->krad_vhs->height);
																
				port_updated = 1;
			}

			krad_frame->format = PIX_FMT_RGB32;
	    krad_frame->timecode = timecode;
	    //krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);
	    krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);
		}
	}
	
	if (krad_link->video_codec == Y4M) {
	  //fixme
	
			//if (port_updated == 0) {
			//	krad_compositor_port_set_io_params (krad_link->krad_compositor_port,
			//										krad_link->krad_vhs->width,
			//										krad_link->krad_vhs->height);
			//													
			//	port_updated = 1;
			//}		
	
	    krad_frame->timecode = timecode;
	    krad_compositor_port_push_frame (krad_link->krad_compositor_port, krad_frame);		
	}		
	
	krad_framepool_unref_frame (krad_frame);		
		

  return 0;

}
	
void video_decoding_unit_destroy (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	int h;

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);
	
	if (krad_link->krad_vpx_decoder != NULL) {
		krad_vpx_decoder_destroy(krad_link->krad_vpx_decoder);
    krad_link->krad_vpx_decoder = NULL;
	}
  if (krad_link->krad_vhs != NULL) {
    krad_vhs_destroy (krad_link->krad_vhs);
    krad_link->krad_vhs = NULL;
  }				
  if (krad_link->krad_theora_decoder != NULL) {
    krad_theora_decoder_destroy (krad_link->krad_theora_decoder);
    krad_link->krad_theora_decoder = NULL;
  }

	free (krad_link->vu_buffer);
	for (h = 0; h < 3; h++) {
		free (krad_link->vu_header[h]);
	}
	printk ("Video decoding thread exiting");

}

void audio_decoding_unit_create (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_system_set_thread_name ("kr_audio_dec");

	printk ("Audio decoding thread starting");

	int c;
	int h;

	/* SET UP */
	
	krad_link->channels = 2;

	for (h = 0; h < 3; h++) {
		krad_link->au_header[h] = malloc(100000);
		krad_link->au_header_len[h] = 0;
	}

	for (c = 0; c < krad_link->channels; c++) {
		krad_link->krad_resample_ring[c] = krad_resample_ring_create (4000000, 48000,
														   krad_link->krad_transponder->krad_radio->krad_mixer->sample_rate);

		krad_link->audio_output_ringbuffer[c] = krad_link->krad_resample_ring[c]->krad_ringbuffer;
		//krad_link->audio_output_ringbuffer[c] = krad_ringbuffer_create (4000000);
		krad_link->au_samples[c] = malloc(4 * 8192);
		krad_link->samples[c] = malloc(4 * 8192);
	}
	
	krad_link->au_buffer = malloc(2000000);
	krad_link->au_audio = calloc(1, 8192 * 4 * 4);
	
	krad_link->last_audio_codec = NOCODEC;
	krad_link->audio_codec = NOCODEC;
	
	krad_link->mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->sysname, INPUT, NOTOUTPUT, 2, 
												   krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);
	
}

int audio_decoding_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;
  
  int c;
  int h;
  int bytes;
  int len;

	/* THE FOLLOWING IS WHERE WE ENSURE WE ARE ON THE RIGHT CODEC AND READ HEADERS IF NEED BE */

	//while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
	//	usleep(5000);
	//}
	
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
			return 1;
		}

		if (krad_link->audio_codec == FLAC) {
			krad_link->krad_flac = krad_flac_decoder_create();
			for (h = 0; h < 1; h++) {

				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
					usleep(10000);
				}

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&krad_link->au_header_len[h], 4);
	
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < krad_link->au_header_len[h]) && (!krad_link->destroy)) {
					usleep(10000);
				}
	
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)krad_link->au_header[h], krad_link->au_header_len[h]);
			}
			krad_flac_decode (krad_link->krad_flac, krad_link->au_header[0], krad_link->au_header_len[0], NULL);
			for (c = 0; c < krad_link->channels; c++) {
				krad_resample_ring_set_input_sample_rate (krad_link->krad_resample_ring[c], krad_link->krad_flac->sample_rate);
			}
		}

		if (krad_link->audio_codec == VORBIS) {
			
			for (h = 0; h < 3; h++) {

				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
					usleep(10000);
				}

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&krad_link->au_header_len[h], 4);
	
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < krad_link->au_header_len[h]) && (!krad_link->destroy)) {
					usleep(10000);
				}
	
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)krad_link->au_header[h], krad_link->au_header_len[h]);
			}

			printk ("Vorbis Header byte sizes: %d %d %d", krad_link->au_header_len[0], krad_link->au_header_len[1], krad_link->au_header_len[2]);

			krad_link->krad_vorbis = krad_vorbis_decoder_create (krad_link->au_header[0], krad_link->au_header_len[0], krad_link->au_header[1], krad_link->au_header_len[1], krad_link->au_header[2], krad_link->au_header_len[2]);

			for (c = 0; c < krad_link->channels; c++) {
				krad_resample_ring_set_input_sample_rate (krad_link->krad_resample_ring[c], krad_link->krad_vorbis->sample_rate);
			}

		}

		if (krad_link->audio_codec == OPUS) {
		
			for (h = 0; h < 1; h++) {

				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < 4) && (!krad_link->destroy)) {
					usleep(5000);
				}

				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&krad_link->au_header_len[h], 4);
	
				while ((krad_ringbuffer_read_space(krad_link->encoded_audio_ringbuffer) < krad_link->au_header_len[h]) && (!krad_link->destroy)) {
					usleep(5000);
				}
	
				krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)krad_link->au_header[h], krad_link->au_header_len[h]);
			}
		
			printk ("Opus Header size: %d", krad_link->au_header_len[0]);
			krad_link->krad_opus = krad_opus_decoder_create (krad_link->au_header[0], krad_link->au_header_len[0], krad_link->krad_radio->krad_mixer->sample_rate);

			for (c = 0; c < krad_link->channels; c++) {
				krad_resample_ring_set_input_sample_rate (krad_link->krad_resample_ring[c], krad_link->krad_radio->krad_mixer->sample_rate);
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
	
	krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)krad_link->au_buffer, bytes);
	
	/* DECODING HAPPENS HERE */
	
	if (krad_link->audio_codec == VORBIS) {
		krad_vorbis_decoder_decode (krad_link->krad_vorbis, krad_link->au_buffer, bytes);
		
		len = 1;
		
		while (len ) {
		
			len = krad_vorbis_decoder_read_audio(krad_link->krad_vorbis, 0, (char *)krad_link->au_samples[0], 512);

			if (len) {
			
				while ((krad_resample_ring_write_space (krad_link->krad_resample_ring[0]) < len) && (!krad_link->destroy)) {
					usleep(25000);
				}
			
				//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[0], (char *)samples[0], len);
				krad_resample_ring_write (krad_link->krad_resample_ring[0], (unsigned char *)krad_link->au_samples[0], len);										
			}

			len = krad_vorbis_decoder_read_audio (krad_link->krad_vorbis, 1, (char *)krad_link->au_samples[1], 512);

			if (len) {

				while ((krad_resample_ring_write_space (krad_link->krad_resample_ring[1]) < len) && (!krad_link->destroy)) {
					//printk ("wait!");
					usleep(25000);
				}

				//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[1], (char *)samples[1], len);
				krad_resample_ring_write (krad_link->krad_resample_ring[1], (unsigned char *)krad_link->au_samples[1], len);					
			}
		
		}
	}
		
	if (krad_link->audio_codec == FLAC) {

		len = krad_flac_decode (krad_link->krad_flac, krad_link->au_buffer, bytes, krad_link->au_samples);
		
		for (c = 0; c < krad_link->channels; c++) {
			krad_resample_ring_write (krad_link->krad_resample_ring[c], (unsigned char *)krad_link->au_samples[c], len * 4);
		}
	}
		
	if (krad_link->audio_codec == OPUS) {

		krad_opus_decoder_write (krad_link->krad_opus, krad_link->au_buffer, bytes);
		
		bytes = -1;

		while (bytes != 0) {
			for (c = 0; c < 2; c++) {
				bytes = krad_opus_decoder_read (krad_link->krad_opus, c + 1, (char *)krad_link->au_audio, 120 * 4);
				if (bytes) {
					if ((bytes / 4) != 120) {
						failfast ("uh oh crazyto");
					}

					while ((krad_resample_ring_write_space (krad_link->krad_resample_ring[c]) < bytes) &&
						   (!krad_link->destroy)) {
							usleep(20000);
					}

					//krad_ringbuffer_write (krad_link->audio_output_ringbuffer[c], (char *)audio, bytes);
					krad_resample_ring_write (krad_link->krad_resample_ring[c], (unsigned char *)krad_link->au_audio, bytes);						

				}
			}
		}
	}
	

  return 0;
}

void audio_decoding_unit_destroy (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;
	
	int c;
	int h;
	
	/* ITS ALL OVER */
	
	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, krad_link->mixer_portgroup);

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

	free (krad_link->au_buffer);
	free (krad_link->au_audio);
	
	for (c = 0; c < krad_link->channels; c++) {
		free(krad_link->samples[c]);
		free(krad_link->au_samples[c]);
		krad_resample_ring_destroy ( krad_link->krad_resample_ring[c] );		
	}	

	for (h = 0; h < 3; h++) {
		free (krad_link->au_header[h]);
	}

	printk ("Audio decoding thread exiting");

}

int krad_link_decklink_video_callback (void *arg, void *buffer, int length) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	int stride;

	stride = krad_link->capture_width + ((krad_link->capture_width/2) * 2);

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

/*
    krad_frame->format = PIX_FMT_RGB32;
		krad_frame->pixels = buffer;    

		krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);
*/
		krad_framepool_unref_frame (krad_frame);

		//krad_compositor_process (krad_link->krad_transponder->krad_radio->krad_compositor);

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
	
	return 0;

}

void decklink_capture_unit_create (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_system_set_thread_name ("kr_decklink");

	krad_link->krad_decklink = krad_decklink_create (krad_link->device);
	krad_decklink_set_video_mode (krad_link->krad_decklink, krad_link->capture_width, krad_link->capture_height,
								  krad_link->fps_numerator, krad_link->fps_denominator);
	
	krad_decklink_set_audio_input (krad_link->krad_decklink, krad_link->audio_input);
	
	krad_decklink_set_video_input (krad_link->krad_decklink, "hdmi");
	
	krad_link->krad_mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer, krad_link->krad_decklink->simplename, INPUT, NOTOUTPUT, 2, 
														  krad_link->krad_radio->krad_mixer->master_mix, KRAD_LINK, krad_link, 0);	
	
	krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor, krad_link->krad_decklink->simplename, INPUT,
																	krad_link->capture_width, krad_link->capture_height);	
	
	
	krad_link->krad_decklink->callback_pointer = krad_link;
	krad_link->krad_decklink->audio_frames_callback = krad_link_decklink_audio_callback;
	krad_link->krad_decklink->video_frame_callback = krad_link_decklink_video_callback;
	
	krad_decklink_set_verbose (krad_link->krad_decklink, verbose);
	
	krad_decklink_start (krad_link->krad_decklink);
}

void decklink_capture_unit_destroy (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	if (krad_link->krad_decklink != NULL) {
		krad_decklink_destroy ( krad_link->krad_decklink );
		krad_link->krad_decklink = NULL;
	}
	
	krad_link->encoding = 2;

	krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, krad_link->krad_mixer_portgroup);

	krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

}


void *krad_link_run_thread (void *arg) {

	krad_link_t *krad_link = (krad_link_t *)arg;

	krad_system_set_thread_name ("kr_link");

	krad_link_activate ( krad_link );
	
	//while (!krad_link->destroy) {
	//	usleep(50000);
	//}

	return NULL;

}

void krad_link_run (krad_link_t *krad_link) {

	pthread_create (&krad_link->main_thread, NULL, krad_link_run_thread, (void *)krad_link);
	pthread_detach (krad_link->main_thread);
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
    krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->cap_graph_id);
		if (krad_link->video_source == X11) {
      x11_capture_unit_destroy ((void *)krad_link);
		}
		if (krad_link->video_source == V4L2) {
      v4l2_capture_unit_destroy ((void *)krad_link);
		}
		if (krad_link->video_source == DECKLINK) {
      decklink_capture_unit_destroy ((void *)krad_link);
		}		
	} else {
		krad_link->encoding = 2;
	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			  //pthread_join (krad_link->video_encoding_thread, NULL);
        krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->vu_graph_id);
        video_encoding_unit_destroy ((void *)krad_link);
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		  if (krad_link->audio_codec != NOCODEC) {
			  //pthread_join (krad_link->audio_encoding_thread, NULL);
        krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->au_graph_id);
        audio_encoding_unit_destroy ((void *)krad_link);
		  }
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
      krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->vud_graph_id);
      video_decoding_unit_destroy ((void *)krad_link);
		}
		
    if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->aud_graph_id);
      audio_decoding_unit_destroy ((void *)krad_link);
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
	
	krad_ringbuffer_free ( krad_link->encoded_audio_ringbuffer );
	krad_ringbuffer_free ( krad_link->encoded_video_ringbuffer );

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
	krad_link->encoded_video_ringbuffer = krad_ringbuffer_create (10000000);

	if (krad_link->operation_mode == CAPTURE) {

		krad_link->channels = 2;

		if ((krad_link->capture_width == 0) || (krad_link->capture_height == 0)) {
			krad_link->capture_width = krad_link->composite_width;
			krad_link->capture_height = krad_link->composite_height;
		}

		for (c = 0; c < krad_link->channels; c++) {
			krad_link->audio_capture_ringbuffer[c] = krad_ringbuffer_create (2000000);		
		}

		if (krad_link->video_source == X11) {
			krad_link->capturing = 1;
      x11_capture_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = x11_capture_unit_process;
      krad_link->cap_graph_id = krad_Xtransponder_add_capture (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
		}
		
		if (krad_link->video_source == V4L2) {
			krad_link->capturing = 1;
      v4l2_capture_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->fd = krad_link->krad_v4l2->fd;
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = v4l2_capture_unit_process;
      krad_link->cap_graph_id = krad_Xtransponder_add_capture (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
		}
		
		if (krad_link->video_source == DECKLINK) {

      krad_link->krad_framepool = krad_framepool_create ( krad_link->capture_width,
      													krad_link->capture_height,
      													DEFAULT_CAPTURE_BUFFER_FRAMES);

			krad_link->capturing = 1;
      decklink_capture_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = NULL;
      krad_link->cap_graph_id = krad_Xtransponder_add_capture (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
			
		}
	}
	
	if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {
		if (krad_link->transport_mode == UDP) {
			pthread_create(&krad_link->udp_input_thread, NULL, udp_input_thread, (void *)krad_link);	
		} else {
			pthread_create(&krad_link->stream_input_thread, NULL, stream_input_thread, (void *)krad_link);	
		}
		
		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      video_decoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = video_decoding_unit_process;
      krad_link->vud_graph_id = krad_Xtransponder_add_decoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
		}
	
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      audio_decoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = audio_decoding_unit_process;
      krad_link->aud_graph_id = krad_Xtransponder_add_decoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
		}

	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {

		krad_link->encoding = 1;

		if ((krad_link->video_passthru == 0) && ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO))) {
      video_encoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->fd = krad_link->krad_compositor_port_fd;
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = video_encoding_unit_process;
      krad_link->vu_graph_id = krad_Xtransponder_add_encoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
		}
	
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      audio_encoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->fd = krad_link->socketpair[1];
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = audio_encoding_unit_process;
      krad_link->au_graph_id = krad_Xtransponder_add_encoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
		}
	
		if ((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == UDP)) {
			pthread_create (&krad_link->udp_output_thread, NULL, udp_output_thread, (void *)krad_link);	
		} else {
			pthread_create (&krad_link->stream_output_thread, NULL, stream_output_thread, (void *)krad_link);	
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




void krad_transponder_listen_promote_client (krad_transponder_listen_client_t *client) {

	krad_transponder_t *krad_transponder;
	krad_link_t *krad_link;	
	int k;

	krad_transponder = client->krad_transponder;

	pthread_mutex_lock (&krad_transponder->change_lock);


	for (k = 0; k < KRAD_TRANSPONDER_MAX_LINKS; k++) {
		if (krad_transponder->krad_link[k] == NULL) {

			krad_transponder->krad_link[k] = krad_link_create (k);
			krad_link = krad_transponder->krad_link[k];
			krad_link->krad_radio = krad_transponder->krad_radio;
			krad_link->krad_transponder = krad_transponder;
			
			krad_tags_set_set_tag_callback (krad_link->krad_tags, krad_transponder->krad_radio->krad_ipc, 
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

	pthread_mutex_unlock (&krad_transponder->change_lock);
	
	free (client);
	
	pthread_exit(0);	

}

void *krad_transponder_listen_client_thread (void *arg) {


	krad_transponder_listen_client_t *client = (krad_transponder_listen_client_t *)arg;
	
	int ret;
	int wot;
	char *string;
	char byte;

	krad_system_set_thread_name ("kr_lsn_client");

	while (1) {
		ret = read (client->sd, client->in_buffer + client->in_buffer_pos, 1);		

		if (ret == 0 || ret == -1) {
			printk ("done with transponder listen client");
			krad_transponder_listen_destroy_client (client);
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
							krad_transponder_listen_destroy_client (client);							
						}
					
					} else {
						printk ("client no good! .. %s", client->in_buffer);
						krad_transponder_listen_destroy_client (client);
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
							
							wot = write (client->sd, goodresp, strlen(goodresp));
              if (wot != strlen(goodresp)) {
                printke ("krad transponder: unexpected write return value %d in transponder_listen_client_thread", wot);
              }							
							
							krad_transponder_listen_promote_client (client);
						}
					}
				}
			}
			
			
			if (client->in_buffer_pos > 1000) {
				printk ("client no good! .. %s", client->in_buffer);
				krad_transponder_listen_destroy_client (client);
			}
		}
	}

	
	printk ("Krad HTTP Request: %s\n", client->in_buffer);
	

	krad_transponder_listen_destroy_client (client);

	return NULL;	
	
}


void krad_transponder_listen_create_client (krad_transponder_t *krad_transponder, int sd) {

	krad_transponder_listen_client_t *client = calloc(1, sizeof(krad_transponder_listen_client_t));

	client->krad_transponder = krad_transponder;
	
	client->sd = sd;
	
	pthread_create (&client->client_thread, NULL, krad_transponder_listen_client_thread, (void *)client);
	pthread_detach (client->client_thread);	

}

void krad_transponder_listen_destroy_client (krad_transponder_listen_client_t *krad_transponder_listen_client) {

	close (krad_transponder_listen_client->sd);
		
	free (krad_transponder_listen_client);
	
	pthread_exit(0);	

}

void *krad_transponder_listening_thread (void *arg) {

	krad_transponder_t *krad_transponder = (krad_transponder_t *)arg;

	int ret;
	int addr_size;
	int client_fd;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];

	krad_system_set_thread_name ("kr_listener");
	
	printk ("Krad transponder: Listening thread starting\n");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_transponder->stop_listening == 0) {

		sockets[0].fd = krad_transponder->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	

		if (ret < 0) {
			printke ("Krad transponder: Failed on poll\n");
			krad_transponder->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
		
			if ((client_fd = accept(krad_transponder->sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size)) < 0) {
				close (krad_transponder->sd);
				failfast ("Krad transponder: socket error on accept mayb a signal or such\n");
			}

			krad_transponder_listen_create_client (krad_transponder, client_fd);

		}
	}
	
	close (krad_transponder->sd);
	krad_transponder->port = 0;
	krad_transponder->listening = 0;	

	printk ("Krad transponder: Listening thread exiting\n");

	return NULL;
}

void krad_transponder_stop_listening (krad_transponder_t *krad_transponder) {

	if (krad_transponder->listening == 1) {
		krad_transponder->stop_listening = 1;
		pthread_join (krad_transponder->listening_thread, NULL);
		krad_transponder->stop_listening = 0;
	}
}


int krad_transponder_listen (krad_transponder_t *krad_transponder, int port) {

	if (krad_transponder->listening == 1) {
		krad_transponder_stop_listening (krad_transponder);
	}

	krad_transponder->port = port;
	krad_transponder->listening = 1;
	
	krad_transponder->local_address.sin_family = AF_INET;
	krad_transponder->local_address.sin_port = htons (krad_transponder->port);
	krad_transponder->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((krad_transponder->sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printke ("Krad transponder: system call socket error\n");
		krad_transponder->listening = 0;
		krad_transponder->port = 0;		
		return 1;
	}

	if (bind (krad_transponder->sd, (struct sockaddr *)&krad_transponder->local_address, sizeof(krad_transponder->local_address)) == -1) {
		printke ("Krad transponder: bind error for tcp port %d\n", krad_transponder->port);
		close (krad_transponder->sd);
		krad_transponder->listening = 0;
		krad_transponder->port = 0;
		return 1;
	}
	
	if (listen (krad_transponder->sd, SOMAXCONN) <0) {
		printke ("Krad transponder: system call listen error\n");
		close (krad_transponder->sd);
		return 1;
	}	
	
	pthread_create (&krad_transponder->listening_thread, NULL, krad_transponder_listening_thread, (void *)krad_transponder);
	
	return 0;
}


krad_link_t *krad_transponder_get_link_from_sysname (krad_transponder_t *krad_transponder, char *sysname) {

	int i;
	krad_link_t *krad_link;

	for (i = 0; i < KRAD_TRANSPONDER_MAX_LINKS; i++) {
		krad_link = krad_transponder->krad_link[i];
		if (krad_link != NULL) {
			if (strcmp(sysname, krad_link->sysname) == 0) {	
				return krad_link;
			}
		}
	}

	return NULL;
}

krad_tags_t *krad_transponder_get_tags_for_link (krad_transponder_t *krad_transponder, char *sysname) {

	krad_link_t *krad_link;
	
	krad_link = krad_transponder_get_link_from_sysname (krad_transponder, sysname);

	if (krad_link != NULL) {
		return krad_link_get_tags (krad_link);
	} else {
		return NULL;
	}
}


krad_transponder_t *krad_transponder_create (krad_radio_t *krad_radio) {

	krad_transponder_t *krad_transponder;
	
	krad_transponder = calloc(1, sizeof(krad_transponder_t));

	krad_transponder->krad_radio = krad_radio;

	pthread_mutex_init (&krad_transponder->change_lock, NULL);	

	krad_transponder->krad_transmitter = krad_transmitter_create ();

	krad_transponder->krad_Xtransponder = krad_Xtransponder_create (krad_transponder->krad_radio);

	return krad_transponder;

}

void krad_transponder_destroy (krad_transponder_t *krad_transponder) {

	int l;
	
  printk ("Krad transponder destroy started");		

	pthread_mutex_lock (&krad_transponder->change_lock);
	
	for (l = 0; l < KRAD_TRANSPONDER_MAX_LINKS; l++) {
		if (krad_transponder->krad_link[l] != NULL) {
			krad_link_destroy (krad_transponder->krad_link[l]);
			krad_transponder->krad_link[l] = NULL;
		}
	}

	krad_transmitter_destroy (krad_transponder->krad_transmitter);

	krad_Xtransponder_destroy (&krad_transponder->krad_Xtransponder);

	pthread_mutex_unlock (&krad_transponder->change_lock);		
	pthread_mutex_destroy (&krad_transponder->change_lock);
	free (krad_transponder);
	
  printk ("Krad transponder destroy completed");	

}


