#include "krad_transponder.h"

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
  
  krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
                                   "X11In",
                                   INPUT,
                                   krad_link->krad_x11->screen_width, krad_link->krad_x11->screen_height);

}

int x11_capture_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;
  
  krad_frame_t *krad_frame;

  if (krad_link->krad_ticker == NULL) {
    krad_link->krad_ticker = krad_ticker_create (krad_link->krad_radio->krad_compositor->frame_rate_numerator,
                      krad_link->krad_radio->krad_compositor->frame_rate_denominator);
    krad_ticker_start (krad_link->krad_ticker);
  } else {
    krad_ticker_wait (krad_link->krad_ticker);
  }
  
  krad_frame = krad_framepool_getframe (krad_link->krad_framepool);

  krad_x11_capture (krad_link->krad_x11, (unsigned char *)krad_frame->pixels);
  
  krad_compositor_port_push_rgba_frame (krad_link->krad_compositor_port, krad_frame);

  krad_framepool_unref_frame (krad_frame);

  printk ("captured!");

  return 0;
}

void x11_capture_unit_destroy (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;
  
  krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);

  krad_ticker_destroy (krad_link->krad_ticker);
  krad_link->krad_ticker = NULL;

  krad_x11_destroy (krad_link->krad_x11);

  printk ("X11 capture thread exited");
  
}


int krad_link_decklink_video_callback (void *arg, void *buffer, int length) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  int stride;
  krad_frame_t *krad_frame;
  
  stride = krad_link->capture_width + ((krad_link->capture_width/2) * 2);
  //printk ("krad link decklink frame received %d bytes", length);

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

#define SAMPLE_16BIT_SCALING 32767.0f

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
  krad_decklink_set_video_mode (krad_link->krad_decklink,
                                krad_link->capture_width, krad_link->capture_height,
                                krad_link->fps_numerator, krad_link->fps_denominator);
  krad_decklink_set_audio_input (krad_link->krad_decklink, krad_link->audio_input);
  krad_decklink_set_video_input (krad_link->krad_decklink, "hdmi");

  krad_link->krad_framepool = krad_framepool_create ( krad_link->capture_width,
                                                      krad_link->capture_height,
                                                      DEFAULT_CAPTURE_BUFFER_FRAMES);

  krad_link->krad_mixer_portgroup = krad_mixer_portgroup_create (krad_link->krad_radio->krad_mixer,
                                                                 krad_link->krad_decklink->simplename,
                                                                 INPUT, NOTOUTPUT, 2, 
                                                                 krad_link->krad_radio->krad_mixer->master_mix,
                                                                 KRAD_LINK, krad_link, 0);  
  
  krad_link->krad_compositor_port = krad_compositor_port_create (krad_link->krad_radio->krad_compositor,
                                                                 krad_link->krad_decklink->simplename,
                                                                 INPUT, krad_link->capture_width,
                                                                 krad_link->capture_height);

  krad_link->krad_decklink->callback_pointer = krad_link;
  krad_link->krad_decklink->audio_frames_callback = krad_link_decklink_audio_callback;
  krad_link->krad_decklink->video_frame_callback = krad_link_decklink_video_callback;
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

void audio_encoding_unit_create (void *arg) {

  //krad_system_set_thread_name ("kr_audio_enc");

  krad_link_t *krad_link = (krad_link_t *)arg;

  int c;

  printk ("Audio unit create");
  
  krad_link->channels = 2;
  
  if (krad_link->audio_codec != VORBIS) {
    krad_link->au_buffer = malloc (300000);
  }
  
  krad_link->au_interleaved_samples = malloc (8192 * 4 * KRAD_MIXER_MAX_CHANNELS);
  
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
  //unsigned char *vorbis_buffer;
  float **float_buffer;
  int s;
  int bytes;
  int frames;
  int ret;
  char buffer[1];
  krad_slice_t *krad_slice;

  krad_slice = NULL;
  
  ret = read (krad_link->socketpair[1], buffer, 1);
  if (ret != 1) {
    if (ret == 0) {
      printk ("Krad AU Transponder: port read got EOF");
      return -1;
    }
    printk ("Krad AU Transponder: port read unexpected read return value %d", ret);
  }
  
  if (krad_link->audio_codec != VORBIS) {
    frames = krad_link->au_framecnt;
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
    if (krad_link->audio_codec == VORBIS) {
      krad_vorbis_encoder_prepare (krad_link->krad_vorbis, krad_link->au_framecnt, &float_buffer);
      for (c = 0; c < krad_link->channels; c++) {
        krad_ringbuffer_read (krad_link->audio_input_ringbuffer[c], (char *)float_buffer[c], krad_link->au_framecnt * 4);
      }      
      krad_vorbis_encoder_wrote (krad_link->krad_vorbis, krad_link->au_framecnt);
      bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &krad_link->au_buffer);
    }

    while (bytes > 0) {
      if (krad_link->au_subunit != NULL) {
        krad_slice = krad_slice_create_with_data (krad_link->au_buffer, bytes);
        krad_slice->frames = frames;
        krad_Xtransponder_encoder_broadcast (krad_link->au_subunit, &krad_slice);
        krad_slice_unref (krad_slice);
      }
      bytes = 0;
      if (krad_link->audio_codec == VORBIS) {
        bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &krad_link->au_buffer);
      }
      if (krad_link->audio_codec == OPUS) {
        bytes = krad_opus_encoder_read (krad_link->krad_opus, krad_link->au_buffer, &krad_link->au_framecnt);
      }
    }
  }

  return 0;
}

void audio_encoding_unit_destroy (void *arg) {
  
  krad_link_t *krad_link = (krad_link_t *)arg;

  krad_mixer_portgroup_destroy (krad_link->krad_radio->krad_mixer, krad_link->mixer_portgroup);
  
  int c;
  //unsigned char *vorbis_buffer;
  //int bytes;
  //int frames;
  
  if (krad_link->krad_vorbis != NULL) {
  
    krad_vorbis_encoder_finish (krad_link->krad_vorbis);
/*
    // DUPEY
    bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
    while (bytes > 0) {
      krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&bytes, 4);
      krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
      krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)vorbis_buffer, bytes);
      bytes = krad_vorbis_encoder_read (krad_link->krad_vorbis, &frames, &vorbis_buffer);
    }
*/
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

  if (krad_link->audio_codec != VORBIS) {
    free (krad_link->au_buffer);
  }
  printk ("Audio encoding thread exiting");  
  
}


void muxer_unit_create (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  krad_system_set_thread_name ("kr_stream_out");
  
  int packet_size;
  
  packet_size = 0;
  
  krad_link->muxer_audio_frames_per_video_frame = 0;

  printk ("Output/Muxing thread starting");
  
  if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
    krad_link->muxer_audio_frames_per_video_frame = krad_link->krad_radio->krad_mixer->sample_rate;
    krad_link->muxer_audio_frames_per_video_frame = krad_link->muxer_audio_frames_per_video_frame / krad_link->encoding_fps_numerator;
    krad_link->muxer_audio_frames_per_video_frame = krad_link->muxer_audio_frames_per_video_frame * krad_link->encoding_fps_denominator;
  }

  printk ("audio frames per video frame %f", krad_link->muxer_audio_frames_per_video_frame);

  krad_link->muxer_packet = malloc (6300000);
  
  if (krad_link->host[0] != '\0') {
  
    if ((strcmp(krad_link->host, "transmitter") == 0) &&
      (krad_link->krad_transponder->krad_transmitter->listening == 1)) {
      
      krad_link->muxer_krad_transmission = krad_transmitter_transmission_create (krad_link->krad_transponder->krad_transmitter,
                                    krad_link->mount + 1,
                                    krad_link_select_mimetype(krad_link->mount + 1));

      krad_link->port = krad_link->krad_transponder->krad_transmitter->port;

      krad_link->krad_container = krad_container_open_transmission (krad_link->muxer_krad_transmission);
  
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
        krad_link->muxer_seen_passthu_keyframe = 1;
      }
    
      krad_link->krad_compositor_port = krad_compositor_passthru_port_create (krad_link->krad_radio->krad_compositor,
                                         "passthruStreamOut",
                                         OUTPUT);
    }
    
    if (krad_link->video_codec == Y4M) {
    
      packet_size = krad_y4m_generate_header (krad_link->muxer_packet, krad_link->encoding_width, krad_link->encoding_height,
                                              krad_link->encoding_fps_numerator, krad_link->encoding_fps_denominator, PIX_FMT_YUV420P);    

      krad_codec_header_t *krad_codec_header = calloc (1, sizeof(krad_codec_header_t));
      
      krad_codec_header->header_count = 1;
      krad_codec_header->header_combined = krad_link->muxer_packet;
      krad_codec_header->header_combined_size = packet_size;
      krad_codec_header->header[0] = krad_link->muxer_packet;
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

}


int muxer_unit_process (void *arg) {


//  int frames;
  int packet_size;
  int keyframe;
  char keyframe_char[1];
  krad_frame_t *krad_frame;
  krad_slice_t *krad_slice;
  
//  frames = 0;
  keyframe = 0;
  packet_size = 0;
  keyframe_char[0] = 0;
  krad_frame = NULL;
  krad_slice = NULL;

  krad_link_t *krad_link = (krad_link_t *)arg;

  if ((krad_link->av_mode != AUDIO_ONLY) && (krad_link->video_passthru == 0)) {
    if ((krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) >= 4) && (krad_link->encoding < 3)) {

      krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);

      while ((krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) < packet_size + 1) &&
           (krad_link->encoding < 3)) {    

        usleep(10000);
      }
    
      if ((krad_ringbuffer_read_space (krad_link->encoded_video_ringbuffer) < packet_size + 1) &&
        (krad_link->encoding > 2)) {
        
        return 1;
      }
    
      krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, keyframe_char, 1);
      krad_ringbuffer_read (krad_link->encoded_video_ringbuffer, (char *)krad_link->muxer_packet, packet_size);

      keyframe = keyframe_char[0];

      krad_container_add_video (krad_link->krad_container, 
                    krad_link->video_track,
                    krad_link->muxer_packet,
                    packet_size,
                    keyframe);

      krad_link->muxer_video_frames_muxed++;
    }
  }
    
  if (krad_link->video_passthru == 1) {

    krad_frame = krad_compositor_port_pull_frame (krad_link->krad_compositor_port);

    if (krad_frame != NULL) {

      if (krad_link->video_codec == H264) {
        keyframe = krad_v4l2_is_h264_keyframe ((unsigned char *)krad_frame->pixels);
        if (krad_link->muxer_seen_passthu_keyframe == 0) {
          if (keyframe == 1) {
            krad_link->muxer_seen_passthu_keyframe = 1;
            printk("Got first h264 passthru keyframe, skipped %d frames before it",
                 krad_link->muxer_initial_passthu_frames_skipped);
          } else {
            krad_link->muxer_initial_passthu_frames_skipped++;
          }
        }
      }
      
      if (krad_link->video_codec == MJPEG) {
        if (krad_link->muxer_video_frames_muxed % 30 == 0) {
          keyframe = 1;
        } else {
          keyframe = 0;
        }        
      }
      
      if (krad_link->muxer_seen_passthu_keyframe == 1) {
        krad_container_add_video (krad_link->krad_container,
                      krad_link->video_track, 
             (unsigned char *)krad_frame->pixels,
                      krad_frame->encoded_size,
                      keyframe);
        krad_link->muxer_video_frames_muxed++;
      }
      
      krad_framepool_unref_frame (krad_frame);
    }
    
    usleep(2000);
  }
    
  if (krad_link->av_mode != VIDEO_ONLY) {

    //((krad_link->muxer_video_frames_muxed * krad_link->muxer_audio_frames_per_video_frame) > krad_link->muxer_audio_frames_muxed))) {

    //krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
    //krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)&frames, 4);
    //krad_ringbuffer_read (krad_link->encoded_audio_ringbuffer, (char *)krad_link->muxer_packet, packet_size);

    krad_slice = krad_Xtransponder_get_slice (krad_link->mux_subunit);
    if (krad_slice != NULL) {
      krad_container_add_audio (krad_link->krad_container,
                    krad_link->audio_track,
                    krad_slice->data,
                    krad_slice->size,
                    krad_slice->frames);

      krad_link->muxer_audio_frames_muxed += krad_slice->frames;
      printk ("ebml muxed audio frames: %d", krad_link->muxer_audio_frames_muxed);

      krad_slice_unref (krad_slice);      
      
    } else {
      printk ("null slice wtf!"); 
    }
  }

  if (krad_link->encoding == 4) {
    return 1;
  }
  
  return 0;
}

void muxer_unit_destroy (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  krad_container_destroy (krad_link->krad_container);
  
  free (krad_link->muxer_packet);
  
  if (krad_link->video_passthru == 1) {
    krad_compositor_port_destroy (krad_link->krad_radio->krad_compositor, krad_link->krad_compositor_port);
  }

  if (krad_link->muxer_krad_transmission != NULL) {
    krad_transmitter_transmission_destroy (krad_link->muxer_krad_transmission);
  }

  printk ("Output/Muxing thread exiting");
  

}

void demuxer_unit_create (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  krad_system_set_thread_name ("kr_stream_in");

  printk ("Input/Demuxing thread starting");

  krad_link->demux_header_buffer = malloc (4096 * 512);
  krad_link->demux_buffer = malloc (4096 * 2048);
  
  krad_link->demux_video_packets = 0;
  krad_link->demux_audio_packets = 0;  
  krad_link->demux_current_track = -1;  
  
  if (krad_link->host[0] != '\0') {
    krad_link->krad_container = krad_container_open_stream (krad_link->host, krad_link->port, krad_link->mount, NULL);
  } else {
    krad_link->krad_container = krad_container_open_file (krad_link->input, KRAD_IO_READONLY);
  }
}

int demuxer_unit_process (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  int codec_bytes;
  int packet_size;
  uint64_t packet_timecode;
  int header_size;
  int h;
  int total_header_size;
  int writeheaders;

  krad_link->demux_nocodec = NOCODEC;
  packet_size = 0;
  codec_bytes = 0;
  header_size = 0;
  total_header_size = 0;  
  writeheaders = 0;

  packet_size = krad_container_read_packet ( krad_link->krad_container, &krad_link->demux_current_track, &packet_timecode, krad_link->demux_buffer);
  //printk ("packet track %d timecode: %zu size %d", current_track, packet_timecode, packet_size);
  if ((packet_size <= 0) && (packet_timecode == 0) && ((krad_link->demux_video_packets + krad_link->demux_audio_packets) > 20))  {
    //printk ("stream input thread packet size was: %d", packet_size);
    return 1;
  }

  if (krad_container_track_changed (krad_link->krad_container, krad_link->demux_current_track)) {
    printk ("track %d changed! status is %d header count is %d",
            krad_link->demux_current_track, krad_container_track_active(krad_link->krad_container, krad_link->demux_current_track),
            krad_container_track_header_count(krad_link->krad_container, krad_link->demux_current_track));

    krad_link->demux_track_codecs[krad_link->demux_current_track] = krad_container_track_codec (krad_link->krad_container, krad_link->demux_current_track);

    if (krad_link->demux_track_codecs[krad_link->demux_current_track] == NOCODEC) {
      return 1;
    }
    writeheaders = 1;
    for (h = 0; h < krad_container_track_header_count (krad_link->krad_container, krad_link->demux_current_track); h++) {
      printk ("header %d is %d bytes", h, krad_container_track_header_size (krad_link->krad_container, krad_link->demux_current_track, h));
      total_header_size += krad_container_track_header_size (krad_link->krad_container, krad_link->demux_current_track, h);
    }
  }

  if ((krad_link->demux_track_codecs[krad_link->demux_current_track] == Y4M) ||
      (krad_link->demux_track_codecs[krad_link->demux_current_track] == KVHS) ||
      (krad_link->demux_track_codecs[krad_link->demux_current_track] == VP8) ||
      (krad_link->demux_track_codecs[krad_link->demux_current_track] == THEORA)) {

    krad_link->demux_video_packets++;

    while ((krad_ringbuffer_write_space(krad_link->encoded_video_ringbuffer) < packet_size + 4 + total_header_size + 4 + 4 + 8) && (!krad_link->destroy)) {
      usleep(10000);
    }

    if (writeheaders == 1) {
      krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&krad_link->demux_nocodec, 4);
      krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&krad_link->demux_track_codecs[krad_link->demux_current_track], 4);
      for (h = 0; h < krad_container_track_header_count(krad_link->krad_container, krad_link->demux_current_track); h++) {
        header_size = krad_container_track_header_size(krad_link->krad_container, krad_link->demux_current_track, h);
        krad_container_read_track_header(krad_link->krad_container, krad_link->demux_header_buffer, krad_link->demux_current_track, h);
        krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&header_size, 4);
        krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)krad_link->demux_header_buffer, header_size);
        codec_bytes += packet_size;
      }
    } else {
      krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&krad_link->demux_track_codecs[krad_link->demux_current_track], 4);
    }
    krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_timecode, 8);
    krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)&packet_size, 4);
    krad_ringbuffer_write(krad_link->encoded_video_ringbuffer, (char *)krad_link->demux_buffer, packet_size);
    codec_bytes += packet_size;
  }

  if ((krad_link->demux_track_codecs[krad_link->demux_current_track] == VORBIS) ||
      (krad_link->demux_track_codecs[krad_link->demux_current_track] == OPUS) ||
      (krad_link->demux_track_codecs[krad_link->demux_current_track] == FLAC)) {

    krad_link->demux_audio_packets++;

    while ((krad_ringbuffer_write_space (krad_link->encoded_audio_ringbuffer) < packet_size + 4 + total_header_size + 4 + 4) && (!krad_link->destroy)) {
      usleep(10000);
    }

    if (writeheaders == 1) {
      krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&krad_link->demux_nocodec, 4);
      krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&krad_link->demux_track_codecs[krad_link->demux_current_track], 4);
      for (h = 0; h < krad_container_track_header_count (krad_link->krad_container, krad_link->demux_current_track); h++) {
        header_size = krad_container_track_header_size (krad_link->krad_container, krad_link->demux_current_track, h);
        krad_container_read_track_header (krad_link->krad_container, krad_link->demux_header_buffer, krad_link->demux_current_track, h);
        krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&header_size, 4);
        krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)krad_link->demux_header_buffer, header_size);
        codec_bytes += packet_size;
      }
    } else {
      krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&krad_link->demux_track_codecs[krad_link->demux_current_track], 4);
    }
    krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)&packet_size, 4);
    krad_ringbuffer_write (krad_link->encoded_audio_ringbuffer, (char *)krad_link->demux_buffer, packet_size);
    codec_bytes += packet_size;
  }

  return 0;  
}

void demuxer_unit_destroy (void *arg) {

  krad_link_t *krad_link = (krad_link_t *)arg;

  krad_link->playing = 3;

  printk ("Input/Demuxing thread exiting");

  krad_container_destroy (krad_link->krad_container);

  free (krad_link->demux_buffer);
  free (krad_link->demux_header_buffer);

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
    if (krad_link->last_video_codec != NOCODEC)  {
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
      //  krad_compositor_port_set_io_params (krad_link->krad_compositor_port,
      //                    krad_link->krad_vhs->width,
      //                    krad_link->krad_vhs->height);
      //                          
      //  port_updated = 1;
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
  //  usleep(5000);
  //}
  
  krad_ringbuffer_read(krad_link->encoded_audio_ringbuffer, (char *)&krad_link->audio_codec, 4);
  if ((krad_link->last_audio_codec != krad_link->audio_codec) || (krad_link->audio_codec == NOCODEC)) {
    printk ("audio codec is %d", krad_link->audio_codec);
    if (krad_link->last_audio_codec != NOCODEC)  {
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
  } else {
    krad_link->encoding = 2;
  }
  
  if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {

    if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
        krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->vu_graph_id);
    }

    if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      if (krad_link->audio_codec != NOCODEC) {
        krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->au_graph_id);
      }
    }

    if (((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == TCP)) ||
         (krad_link->operation_mode == RECORD)) {
        krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->mux_graph_id);
    }
  }
  
  if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {


    if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->vud_graph_id);
    }
    
    if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->aud_graph_id);
    }
    
    if ((krad_link->transport_mode == TCP) || (krad_link->transport_mode == FILESYSTEM)) {
      krad_Xtransponder_subunit_remove (krad_link->krad_transponder->krad_Xtransponder, krad_link->demux_graph_id);
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

krad_link_t *krad_link_prepare (int linknum) {

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

void krad_link_start (krad_link_t *krad_link) {

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
      watch->idle_callback_interval = 5;
      watch->readable_callback = x11_capture_unit_process;
      watch->destroy_callback = x11_capture_unit_destroy;
      krad_link->cap_graph_id = krad_Xtransponder_add_capture (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->cap_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->cap_graph_id);      
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
      watch->destroy_callback = v4l2_capture_unit_destroy;
      krad_link->cap_graph_id = krad_Xtransponder_add_capture (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->cap_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->cap_graph_id);      
      free (watch);
    }
    
    if (krad_link->video_source == DECKLINK) {
      krad_link->capturing = 1;
      decklink_capture_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = NULL;
      watch->destroy_callback = decklink_capture_unit_destroy;
      krad_link->cap_graph_id = krad_Xtransponder_add_capture (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->cap_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->cap_graph_id);      
      free (watch);
      
    }
  }
  
  if ((krad_link->operation_mode == RECEIVE) || (krad_link->operation_mode == PLAYBACK)) {

    demuxer_unit_create ((void *)krad_link);
    krad_transponder_watch_t *watch;
    watch = calloc (1, sizeof(krad_transponder_watch_t));
    watch->idle_callback_interval = 5;
    watch->callback_pointer = (void *)krad_link;
    watch->readable_callback = demuxer_unit_process;
    watch->destroy_callback = demuxer_unit_destroy;
    krad_link->demux_graph_id = krad_Xtransponder_add_demuxer (krad_link->krad_transponder->krad_Xtransponder, watch);
    krad_link->demux_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->demux_graph_id);      
    free (watch);
        
    if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      video_decoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = video_decoding_unit_process;
      watch->destroy_callback = video_decoding_unit_destroy;
      krad_link->vud_graph_id = krad_Xtransponder_add_decoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->vud_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->vud_graph_id);      
      free (watch);
    }
  
    if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      audio_decoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = audio_decoding_unit_process;
      watch->destroy_callback = audio_decoding_unit_destroy;
      krad_link->aud_graph_id = krad_Xtransponder_add_decoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->aud_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->aud_graph_id);      
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
      watch->destroy_callback = video_encoding_unit_destroy;
      krad_link->vu_graph_id = krad_Xtransponder_add_encoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->vu_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->vu_graph_id);      
      free (watch);
    }
  
    if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
      audio_encoding_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      watch->fd = krad_link->socketpair[1];
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = audio_encoding_unit_process;
      watch->destroy_callback = audio_encoding_unit_destroy;
      krad_link->au_graph_id = krad_Xtransponder_add_encoder (krad_link->krad_transponder->krad_Xtransponder, watch);
      free (watch);
      krad_link->au_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->au_graph_id);
    }
  
    if (((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode != UDP)) ||
         (krad_link->operation_mode == RECORD)) {
      muxer_unit_create ((void *)krad_link);
      krad_transponder_watch_t *watch;
      watch = calloc (1, sizeof(krad_transponder_watch_t));
      //watch->idle_callback_interval = 5;
      watch->callback_pointer = (void *)krad_link;
      watch->readable_callback = muxer_unit_process;
      watch->destroy_callback = muxer_unit_destroy;
      krad_link->mux_graph_id = krad_Xtransponder_add_muxer (krad_link->krad_transponder->krad_Xtransponder, watch);
      krad_link->mux_subunit = krad_Xtransponder_get_subunit (krad_link->krad_transponder->krad_Xtransponder, krad_link->mux_graph_id);      
      free (watch);
    }
  }
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
  
  krad_transponder = calloc (1, sizeof(krad_transponder_t));

  krad_transponder->krad_radio = krad_radio;
  pthread_mutex_init (&krad_transponder->change_lock, NULL);
  krad_transponder->krad_receiver = krad_receiver_create (krad_transponder);  
  krad_transponder->krad_transmitter = krad_transmitter_create ();
  krad_transponder->krad_Xtransponder = krad_Xtransponder_create (krad_transponder->krad_radio);

  return krad_transponder;

}

void krad_transponder_destroy (krad_transponder_t *krad_transponder) {

  int l;
  
  printk ("Krad Transponder: Destroy Started");

  pthread_mutex_lock (&krad_transponder->change_lock);
  
  for (l = 0; l < KRAD_TRANSPONDER_MAX_LINKS; l++) {
    if (krad_transponder->krad_link[l] != NULL) {
      krad_link_destroy (krad_transponder->krad_link[l]);
      krad_transponder->krad_link[l] = NULL;
    }
  }

  krad_receiver_destroy (krad_transponder->krad_receiver);
  krad_transmitter_destroy (krad_transponder->krad_transmitter);
  krad_Xtransponder_destroy (&krad_transponder->krad_Xtransponder);

  pthread_mutex_unlock (&krad_transponder->change_lock);    
  pthread_mutex_destroy (&krad_transponder->change_lock);
  free (krad_transponder);
  
  printk ("Krad Transponder: Destroy Completed");

}

