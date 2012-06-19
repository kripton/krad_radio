#include "krad_compositor.h"

static void krad_compositor_close_display (krad_compositor_t *krad_compositor);
static void krad_compositor_open_display (krad_compositor_t *krad_compositor);
static void *krad_compositor_display_thread (void *arg);


static void *krad_compositor_display_thread (void *arg) {

	krad_compositor_t *krad_compositor = (krad_compositor_t *)arg;

	krad_x11_t *krad_x11;
	krad_frame_t *krad_frame;
	krad_compositor_port_t *krad_compositor_port;

	int w;
	int h;
	
	krad_compositor_get_info (krad_compositor,
							  &w,
							  &h);
	
	krad_compositor_port = krad_compositor_port_create (krad_compositor, "X11Out", OUTPUT,
														60000, 1000, 
														w, h);
	
	krad_x11 = krad_x11_create ();
	
	krad_x11_create_glx_window (krad_x11, "Krad Radio", w, h, NULL);
	
	while ((krad_x11->close_window == 0) && (krad_compositor->display_open == 1)) {
	
		krad_frame = krad_compositor_port_pull_frame (krad_compositor_port);

		if (krad_frame != NULL) {
			memcpy (krad_x11->pixels, krad_frame->pixels, w * h * 4);
			krad_framepool_unref_frame (krad_frame);
			krad_x11_glx_render (krad_x11);
		} else {
			krad_x11_glx_sync (krad_x11);
			krad_x11_glx_check_for_input (krad_x11);
		}
	}
	
	krad_compositor_port_destroy (krad_compositor, krad_compositor_port);
	
	krad_x11_destroy_glx_window (krad_x11);

	krad_x11_destroy (krad_x11);

	krad_compositor->display_open = 0;

	return NULL;

}


static void krad_compositor_open_display (krad_compositor_t *krad_compositor) {

	if (krad_compositor->display_open == 1) {
		krad_compositor_close_display (krad_compositor);
	}

	krad_compositor->display_open = 1;
	pthread_create (&krad_compositor->display_thread, NULL, krad_compositor_display_thread, (void *)krad_compositor);

}


static void krad_compositor_close_display (krad_compositor_t *krad_compositor) {

	if (krad_compositor->display_open == 1) {
		krad_compositor->display_open = 2;
		pthread_join (krad_compositor->display_thread, NULL);
		krad_compositor->display_open = 0;
	}

}

uint64_t re_fps (uint64_t frame_num, int input_numerator, int input_denominator, int output_numerator, int output_denominator) {

	uint64_t b;
	uint64_t c;	


	b = output_numerator * (uint64_t)input_denominator;
	c = input_numerator * (uint64_t)output_denominator;

	return ((frame_num * b) / c);


}

void krad_compositor_process (krad_compositor_t *krad_compositor) {

	int p;
	
	krad_frame_t *composite_frame;
	int *pixels;
	krad_frame_t *frame[KRAD_COMPOSITOR_MAX_PORTS];	
	
	composite_frame = NULL;
	
	memset (frame, 0, sizeof (frame));
	
	if (krad_compositor->active_ports < 1) {
		return;
	}
	
	krad_compositor->frame_num++;
	
	if (krad_compositor->bug_filename != NULL) {
	
		if (strlen(krad_compositor->bug_filename)) {
			printk ("setting bug to %s %d %d\n", krad_compositor->bug_filename, 
			        krad_compositor->bug_x, krad_compositor->bug_y);
			kradgui_set_bug (krad_compositor->krad_gui, krad_compositor->bug_filename, 
							 krad_compositor->bug_x, krad_compositor->bug_y);
		} else {
			printk ("removing bug\n");
			kradgui_remove_bug (krad_compositor->krad_gui);
		}
		free (krad_compositor->bug_filename);
		krad_compositor->bug_filename = NULL;
	}
	
	/* Clear it off */
	krad_gui_clear (krad_compositor->krad_gui);

	pixels = (int *)krad_compositor->krad_gui->pixels;

	/* Composite Input Ports */

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].active == 1) && (krad_compositor->port[p].direction == INPUT)) {

			frame[p] = krad_compositor_port_pull_frame (&krad_compositor->port[p]);		

			if (frame[p] != NULL) {
				if ((krad_compositor->port->x == 0) && (krad_compositor->port->y == 0) &&
					(krad_compositor->port->width == krad_compositor->port->crop_width) &&
					(krad_compositor->port->height == krad_compositor->port->crop_height) &&
					(krad_compositor->port->crop_x == krad_compositor->port->x) &&
					(krad_compositor->port->crop_y == krad_compositor->port->y) &&
					(krad_compositor->port->rotation == 0))
				
				{
					memcpy ( pixels, frame[p]->pixels, krad_compositor->frame_byte_size );
				} else {
					cairo_save (krad_compositor->krad_gui->cr);
					if (krad_compositor->port->rotation != 0) {
						cairo_translate (krad_compositor->krad_gui->cr,
										 krad_compositor->port->crop_width / 2,
										 krad_compositor->port->crop_height / 2);
										 
						cairo_rotate (krad_compositor->krad_gui->cr,
									  krad_compositor->port->rotation * (M_PI/180.0));

						cairo_translate (krad_compositor->krad_gui->cr,
										 krad_compositor->port->crop_width / -2,
										 krad_compositor->port->crop_height / -2);
					}
					cairo_set_source_surface (krad_compositor->krad_gui->cr,
											  frame[p]->cst,
											  krad_compositor->port->x - krad_compositor->port->crop_x,
											  krad_compositor->port->y - krad_compositor->port->crop_y);
				
					cairo_rectangle (krad_compositor->krad_gui->cr,
									 krad_compositor->port->x,
									 krad_compositor->port->y,
									 krad_compositor->port->crop_width,
									 krad_compositor->port->crop_height);
									 
					cairo_fill (krad_compositor->krad_gui->cr);
					cairo_restore (krad_compositor->krad_gui->cr);
				}
				krad_framepool_unref_frame (frame[p]);
			}
		}
	}

	/* Render Overlayed Items */
	
	if (krad_compositor->hex_size > 0) {
		kradgui_render_hex (krad_compositor->krad_gui, krad_compositor->hex_x, krad_compositor->hex_y, 
		                    krad_compositor->hex_size);	
	}
	
	if (krad_compositor->render_vu_meters > 0) {
		kradgui_render_meter (krad_compositor->krad_gui, 110, krad_compositor->krad_gui->height - 30, 64,
		                      krad_compositor->krad_gui->output_current[0]);
		kradgui_render_meter (krad_compositor->krad_gui, krad_compositor->krad_gui->width - 110,
		                      krad_compositor->krad_gui->height - 30, 64, krad_compositor->krad_gui->output_current[1]);
	}
	
	

	kradgui_render (krad_compositor->krad_gui);
	
	/* Get a frame */
	
	do {
		composite_frame = krad_framepool_getframe (krad_compositor->krad_framepool);
		if (composite_frame == NULL) {
			printke ("This is very bad! Compositor wanted a frame but could not get one right away!");
			usleep (5000);
		}
	} while (composite_frame == NULL);	
	
	memcpy (composite_frame->pixels, krad_compositor->krad_gui->pixels, krad_compositor->frame_byte_size);

	/* Push out the composited frame */
	
	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].active == 1) && (krad_compositor->port[p].direction == OUTPUT)) {
			krad_compositor_port_push_frame (&krad_compositor->port[p], composite_frame);
		}
	}
	
	krad_framepool_unref_frame (composite_frame);
}

void krad_compositor_mjpeg_process (krad_compositor_t *krad_compositor) {

	int p;
	
	krad_frame_t *krad_frame;
	
	krad_frame = NULL;

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].active == 1) && (krad_compositor->port[p].direction == INPUT) &&
			(krad_compositor->port[p].mjpeg == 1)) {
				krad_frame = krad_compositor_port_pull_frame (&krad_compositor->port[p]);
			break;
		}
	}
	
	if (krad_frame == NULL) {
		return;
	}

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].active == 1) && (krad_compositor->port[p].direction == OUTPUT) &&
			(krad_compositor->port[p].mjpeg == 1)) {

			krad_compositor_port_push_frame (&krad_compositor->port[p], krad_frame);
			break;
		}
	}
	
	krad_framepool_unref_frame (krad_frame);

}

void krad_compositor_port_set_comp_params (krad_compositor_port_t *krad_compositor_port,
										   int width, int height, int x, int y, 
										   int crop_width, int crop_height,
										   int crop_x, int crop_y, float rotation) {
										   

	krad_compositor_port->width = width;
	krad_compositor_port->height = height;

	krad_compositor_port->crop_width = crop_width;
	krad_compositor_port->crop_height = crop_height;
	
	krad_compositor_port->crop_x = crop_x;
	krad_compositor_port->crop_y = crop_y;	

	krad_compositor_port->x = x;
	krad_compositor_port->y = y;

	krad_compositor_port->rotation = rotation;	

	krad_compositor_port->comp_params_updated = 1;
										   
}

void krad_compositor_port_set_io_params (krad_compositor_port_t *krad_compositor_port,
										 int frame_rate_numerator, int frame_rate_denominator,
										 int width, int height) {
										   
	krad_compositor_port->frame_rate_numerator = frame_rate_numerator;
	krad_compositor_port->frame_rate_denominator = frame_rate_denominator;
	krad_compositor_port->source_width = width;
	krad_compositor_port->source_height = height;

	krad_compositor_port->io_params_updated = 1;
										   
}										   

void krad_compositor_port_push_yuv_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame) {

	int rgb_stride_arr[3] = {4*krad_compositor_port->width, 0, 0};
	unsigned char *dst[4];
	
	if ((krad_compositor_port->io_params_updated) || (krad_compositor_port->comp_params_updated)){
		if (krad_compositor_port->sws_converter != NULL) {
			sws_freeContext ( krad_compositor_port->sws_converter );
			krad_compositor_port->sws_converter = NULL;
		}
		krad_compositor_port->io_params_updated = 0;
		krad_compositor_port->comp_params_updated = 0;
	}
	
	if (krad_compositor_port->sws_converter == NULL) {

		krad_compositor_port->sws_converter =
			sws_getContext ( krad_compositor_port->source_width,
							 krad_compositor_port->source_height,
							 krad_frame->format,
							 krad_compositor_port->width,
							 krad_compositor_port->height,
							 PIX_FMT_RGB32, 
							 SWS_BICUBIC,
							 NULL, NULL, NULL);

	}									 

	dst[0] = (unsigned char *)krad_frame->pixels;

	sws_scale (krad_compositor_port->sws_converter, (const uint8_t * const*)krad_frame->yuv_pixels,
			   krad_frame->yuv_strides, 0, krad_compositor_port->source_height, dst, rgb_stride_arr);

	krad_compositor_port_push_frame (krad_compositor_port, krad_frame);

}

krad_frame_t *krad_compositor_port_pull_yuv_frame (krad_compositor_port_t *krad_compositor_port,
												   uint8_t *yuv_pixels[4], int yuv_strides[4]) {

	krad_frame_t *krad_frame;
	
			
	//		converted_frame_num = re_fps (krad_compositor->frame_num,
	//									  krad_compositor->frame_rate_numerator,
	//									  krad_compositor->frame_rate_denominator,
	//									  25000,
	//										  1000);	
	
	
	if (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= sizeof(krad_frame_t *)) {
		krad_ringbuffer_read (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));
		
		
		
		
	int rgb_stride_arr[3] = {4*krad_compositor_port->krad_compositor->width, 0, 0};
	unsigned char *src[4];
	
	if (krad_compositor_port->io_params_updated) {
		if (krad_compositor_port->sws_converter != NULL) {
			sws_freeContext ( krad_compositor_port->sws_converter );
			krad_compositor_port->sws_converter = NULL;
		}
		krad_compositor_port->io_params_updated = 0;
	}
	
	if (krad_compositor_port->sws_converter == NULL) {

		krad_compositor_port->sws_converter =
			sws_getContext ( krad_compositor_port->krad_compositor->width,
							 krad_compositor_port->krad_compositor->height,
							 PIX_FMT_RGB32,
							 krad_compositor_port->width,
							 krad_compositor_port->height,
							 PIX_FMT_YUV420P, 
							 SWS_BICUBIC,
							 NULL, NULL, NULL);

	}									 

	src[0] = (unsigned char *)krad_frame->pixels;

	sws_scale (krad_compositor_port->sws_converter, (const uint8_t * const*)src,
			   rgb_stride_arr, 0, krad_compositor_port->krad_compositor->height, yuv_pixels, yuv_strides);
		
		return krad_frame;
	}

	return NULL;

}

void krad_compositor_port_push_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame) {

	krad_framepool_ref_frame (krad_frame);
	
	krad_ringbuffer_write (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));

}


krad_frame_t *krad_compositor_port_pull_frame (krad_compositor_port_t *krad_compositor_port) {

	krad_frame_t *krad_frame;
	
			
	//		converted_frame_num = re_fps (krad_compositor->frame_num,
	//									  krad_compositor->frame_rate_numerator,
	//									  krad_compositor->frame_rate_denominator,
	//									  25000,
	//										  1000);	
	
	
	if (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= sizeof(krad_frame_t *)) {
		krad_ringbuffer_read (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));
		return krad_frame;
	}

	return NULL;

}

int krad_compositor_port_frames_avail (krad_compositor_port_t *krad_compositor_port) {
	
	int frames;
	
	frames = krad_ringbuffer_read_space (krad_compositor_port->frame_ring);
	
	frames = frames / sizeof (krad_frame_t *);

	return frames;
}

void krad_compositor_alloc_resources (krad_compositor_t *krad_compositor) {

	if (krad_compositor->krad_framepool == NULL) {
		krad_compositor->krad_framepool = 
			krad_framepool_create ( krad_compositor->width, krad_compositor->height, DEFAULT_COMPOSITOR_BUFFER_FRAMES);
	}
	
	if (krad_compositor->krad_gui == NULL) {
		krad_compositor->krad_gui = kradgui_create_with_internal_surface (krad_compositor->width, krad_compositor->height);
		//krad_compositor->krad_gui = kradgui_create_with_external_surface (krad_compositor->width,
		//																  krad_compositor->height,
		//																  NULL);
		krad_compositor->krad_gui->clear = 0;
	}
		
}

krad_compositor_port_t *krad_compositor_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction,
													 int frame_rate_numerator, int frame_rate_denominator,
													 int width, int height) {

	krad_compositor_port_t *krad_compositor_port;

	int p;

	pthread_mutex_lock (&krad_compositor->settings_lock);

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if (krad_compositor->port[p].active == 0) {
			krad_compositor_port = &krad_compositor->port[p];
			krad_compositor_port->active = 2;
			break;
		}
	}
	
	if (krad_compositor_port == NULL) {
		return NULL;
	}
	
	krad_compositor_port->frame_rate_numerator = frame_rate_numerator;
	krad_compositor_port->frame_rate_denominator = frame_rate_denominator;
	krad_compositor_port->source_width = width;
	krad_compositor_port->source_height = height;
	krad_compositor_port->width = krad_compositor->width;
	krad_compositor_port->height = krad_compositor->height;	
	
	krad_compositor_port->x = 0;
	krad_compositor_port->y = 0;	
	krad_compositor_port->crop_x = 0;
	krad_compositor_port->crop_y = 0;
	krad_compositor_port->crop_width = krad_compositor->width;
	krad_compositor_port->crop_height = krad_compositor->height;
	
	krad_compositor_alloc_resources (krad_compositor);
	
	krad_compositor_port->krad_compositor = krad_compositor;
	
	strcpy (krad_compositor_port->sysname, sysname);	
	
	krad_compositor_port->direction = direction;
	
	krad_compositor_port->frame_ring = 
		krad_ringbuffer_create ( DEFAULT_COMPOSITOR_BUFFER_FRAMES * sizeof(krad_frame_t *) );
	
	krad_compositor_port->active = 1;
	
	krad_compositor->active_ports++;
	pthread_mutex_unlock (&krad_compositor->settings_lock);		
	
	return krad_compositor_port;

}

krad_compositor_port_t *krad_compositor_mjpeg_port_create (krad_compositor_t *krad_compositor,
														   char *sysname, int direction) {

	krad_compositor_port_t *krad_compositor_port;
	
	krad_compositor_port = krad_compositor_port_create (krad_compositor, sysname, direction,
														1,1,1,1);

	krad_compositor_port->mjpeg = 1;		

	return krad_compositor_port;	

}


void krad_compositor_port_destroy (krad_compositor_t *krad_compositor, krad_compositor_port_t *krad_compositor_port) {

	pthread_mutex_lock (&krad_compositor->settings_lock);	
	krad_compositor_port->active = 3;

	krad_ringbuffer_free ( krad_compositor_port->frame_ring );

	krad_compositor_port->active = 0;

	if (krad_compositor_port->sws_converter != NULL) {
		sws_freeContext ( krad_compositor_port->sws_converter );
		krad_compositor_port->sws_converter = NULL;
	}

	krad_compositor->active_ports--;
	pthread_mutex_unlock (&krad_compositor->settings_lock);	

}

void krad_compositor_destroy (krad_compositor_t *krad_compositor) {

	int p;

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if (krad_compositor->port[p].active == 1) {
			krad_compositor_port_destroy (krad_compositor, &krad_compositor->port[p]);
		}
	}

	if (krad_compositor->krad_framepool != NULL) {
		krad_framepool_destroy ( krad_compositor->krad_framepool );
	}

	if (krad_compositor->krad_gui != NULL) {
		kradgui_destroy (krad_compositor->krad_gui);
	}
	
	pthread_mutex_destroy (&krad_compositor->settings_lock);	

	free (krad_compositor->port);

	free (krad_compositor);

}

void krad_compositor_get_info (krad_compositor_t *krad_compositor, int *width, int *height) {

	pthread_mutex_lock (&krad_compositor->settings_lock);

	*width = krad_compositor->width;
	*height = krad_compositor->height;

	pthread_mutex_unlock (&krad_compositor->settings_lock);
}

void krad_compositor_get_info_NEW (krad_compositor_t *krad_compositor, int *width, int *height,
							   int *frame_rate_numerator, int *frame_rate_denominator) {

	pthread_mutex_lock (&krad_compositor->settings_lock);

	*width = krad_compositor->width;
	*height = krad_compositor->height;
	*frame_rate_numerator = krad_compositor->frame_rate_numerator;
	*frame_rate_denominator = krad_compositor->frame_rate_denominator;

	pthread_mutex_unlock (&krad_compositor->settings_lock);

}


krad_compositor_t *krad_compositor_create (int width, int height,
										   int frame_rate_numerator, int frame_rate_denominator) {

	krad_compositor_t *krad_compositor = calloc(1, sizeof(krad_compositor_t));

	krad_compositor->width = width;
	krad_compositor->height = height;

	krad_compositor->frame_byte_size = krad_compositor->width * krad_compositor->height * 4;

	krad_compositor->port = calloc(KRAD_COMPOSITOR_MAX_PORTS, sizeof(krad_compositor_port_t));
	
	pthread_mutex_init (&krad_compositor->settings_lock, NULL);
	
	krad_compositor_set_frame_rate (krad_compositor, frame_rate_numerator, frame_rate_denominator);
	
	krad_compositor->hex_x = 150;
	krad_compositor->hex_y = 100;
	krad_compositor->hex_size = 0;

	krad_compositor->bug_filename = NULL;	
	
	krad_compositor->render_vu_meters = 0;
	
	krad_compositor_alloc_resources (krad_compositor);
	
	krad_compositor_start_ticker (krad_compositor);
	
	return krad_compositor;

}


void *krad_compositor_ticker_thread (void *arg) {

	krad_compositor_t *krad_compositor = (krad_compositor_t *)arg;

	krad_compositor->krad_ticker = krad_ticker_create (krad_compositor->frame_rate_numerator,
													   krad_compositor->frame_rate_denominator);
													   
	krad_ticker_start (krad_compositor->krad_ticker);

	while (krad_compositor->ticker_running == 1) {
	
		krad_compositor_process (krad_compositor);
	
		krad_ticker_wait (krad_compositor->krad_ticker);

	}

	krad_ticker_destroy (krad_compositor->krad_ticker);


	return NULL;

}


void krad_compositor_start_ticker (krad_compositor_t *krad_compositor) {

	if (krad_compositor->ticker_running == 1) {
		krad_compositor_stop_ticker (krad_compositor);
	}

	krad_compositor->ticker_running = 1;
	pthread_create (&krad_compositor->ticker_thread, NULL, krad_compositor_ticker_thread, (void *)krad_compositor);

}


void krad_compositor_stop_ticker (krad_compositor_t *krad_compositor) {

	if (krad_compositor->ticker_running == 1) {
		krad_compositor->ticker_running = 2;
		pthread_join (krad_compositor->ticker_thread, NULL);
		krad_compositor->ticker_running = 0;
	}

}

void krad_compositor_unset_pusher (krad_compositor_t *krad_compositor) {
	if (krad_compositor->ticker_running == 1) {
		krad_compositor_stop_ticker (krad_compositor);
	}
	krad_compositor_start_ticker (krad_compositor);
	krad_compositor->pusher = 0;
}

void krad_compositor_set_pusher (krad_compositor_t *krad_compositor, krad_display_api_t pusher) {
	if (pusher == 0) {
		krad_compositor_unset_pusher (krad_compositor);
	} else {
		if (krad_compositor->ticker_running == 1) {
			krad_compositor_stop_ticker (krad_compositor);
		}	
		krad_compositor->pusher = pusher;
	}
}

int krad_compositor_has_pusher (krad_compositor_t *krad_compositor) {
	if (krad_compositor->pusher == 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}

krad_display_api_t krad_compositor_get_pusher (krad_compositor_t *krad_compositor) {
	return krad_compositor->pusher;
}

void krad_compositor_set_frame_rate (krad_compositor_t *krad_compositor,
									 int frame_rate_numerator, int frame_rate_denominator) {

	krad_compositor->frame_rate_numerator = frame_rate_numerator;
	krad_compositor->frame_rate_denominator = frame_rate_denominator;	
	
	if (krad_compositor->ticker_running == 1) {
		krad_compositor_stop_ticker (krad_compositor);
		krad_compositor_start_ticker (krad_compositor);
	}
	
}


void krad_compositor_set_peak (krad_compositor_t *krad_compositor, int channel, float value) {
	
	int c;
	int fps;
	float kick;
	
	kick = 0.0;
	
	c = channel;
	fps = 30;

	if (value >= krad_compositor->krad_gui->output_peak[c]) {
		if (value > 2.7f) {
			krad_compositor->krad_gui->output_peak[c] = value;
			kick = 
			((krad_compositor->krad_gui->output_peak[channel] - krad_compositor->krad_gui->output_current[c]) / 300.0);
		}
	} else {
		if (krad_compositor->krad_gui->output_peak[c] == krad_compositor->krad_gui->output_current[c]) {
			krad_compositor->krad_gui->output_peak[c] -= (0.9 * (60/fps));
			if (krad_compositor->krad_gui->output_peak[c] < 0.0f) {
				krad_compositor->krad_gui->output_peak[c] = 0.0f;
			}
			krad_compositor->krad_gui->output_current[c] = krad_compositor->krad_gui->output_peak[c];
		}
	}

	if (krad_compositor->krad_gui->output_peak[c] > krad_compositor->krad_gui->output_current[c]) {
		krad_compositor->krad_gui->output_current[c] = 
		(krad_compositor->krad_gui->output_current[c] + 1.4) * (1.3 + kick);
	}

	if (krad_compositor->krad_gui->output_peak[c] < krad_compositor->krad_gui->output_current[c]) {
		krad_compositor->krad_gui->output_current[c] = krad_compositor->krad_gui->output_peak[c];
	}
}

void krad_compositor_port_to_ebml ( krad_ipc_server_t *krad_ipc, krad_compositor_port_t *krad_compositor_port) {

	uint64_t port;

	krad_ebml_start_element (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_COMPOSITOR_PORT, &port);	

	krad_ipc_server_respond_number ( krad_ipc,
									 EBML_ID_KRAD_COMPOSITOR_PORT_SOURCE_WIDTH,
									 krad_compositor_port->source_width);
									 
	krad_ebml_finish_element (krad_ipc->current_client->krad_ebml2, port);

}

int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc ) {


	uint32_t ebml_id;
	
	uint32_t command;
	uint64_t ebml_data_size;
	uint64_t element;
	uint64_t response;	
	
	uint64_t numbers[32];		
	float floats[32];			
	
	char string[1024];
	
	int p;
	
	p = 0;
	string[0] = '\0';

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {
	
		case EBML_ID_KRAD_COMPOSITOR_CMD_UPDATE_PORT:

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_NUMBER) {
				numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			}

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_X) {
				numbers[1] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_Y) {
				numbers[2] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_WIDTH) {
				numbers[3] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_HEIGHT) {
				numbers[4] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_ROTATION) {
				floats[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			}
			
			
			krad_compositor_port_set_comp_params (&krad_compositor->port[numbers[0]],
										   		  numbers[3], numbers[4], numbers[1], numbers[2], 
												  krad_compositor->port[numbers[0]].crop_width,
												  krad_compositor->port[numbers[0]].crop_height,
												  krad_compositor->port[numbers[0]].crop_x,
												  krad_compositor->port[numbers[0]].crop_y,
												  floats[0]);

			break;		
	
		case EBML_ID_KRAD_COMPOSITOR_CMD_INFO:
			sprintf (string, "Compositor Resolution: %d x %d Frame Rate: %d / %d - %f",
					 krad_compositor->width, krad_compositor->height,
					 krad_compositor->frame_rate_numerator, krad_compositor->frame_rate_denominator,
					 ((float)krad_compositor->frame_rate_numerator / (float)krad_compositor->frame_rate_denominator));
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_MSG, &response);
			krad_ipc_server_respond_string ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_INFO, string);
			krad_ipc_server_response_finish ( krad_ipc, response);
		
			break;
			
		case EBML_ID_KRAD_COMPOSITOR_CMD_SET_MODE:

			break;
	
		case EBML_ID_KRAD_COMPOSITOR_CMD_LIST_PORTS:

			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_PORT_LIST, &element);	
			
			for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
				if (krad_compositor->port[p].active == 1) {
					//printf("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
					krad_compositor_port_to_ebml ( krad_ipc, &krad_compositor->port[p]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
				
			break;	

		case  EBML_ID_KRAD_COMPOSITOR_CMD_VU_MODE:

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_VU_ON) {
				//printf("hrm wtf3vu\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->render_vu_meters = krad_ebml_read_number (krad_ipc->current_client->krad_ebml,
																	   ebml_data_size);

			break;

	
		case EBML_ID_KRAD_COMPOSITOR_CMD_SET_BUG:

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_X) {
				//printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->bug_x = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_Y) {
				//printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->bug_y = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_FILENAME) {
				//printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}


			krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

			krad_compositor->bug_filename = strdup (string);

			/*
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					//printf("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
			*/		
			break;
	
		case EBML_ID_KRAD_COMPOSITOR_CMD_CLOSE_DISPLAY:
		
			krad_compositor_close_display (krad_compositor);		
			
			break;
		case EBML_ID_KRAD_COMPOSITOR_CMD_OPEN_DISPLAY:
			
			krad_compositor->display_width = krad_compositor->width;
			krad_compositor->display_height = krad_compositor->height;
			
			krad_compositor_open_display (krad_compositor);
			
			break;

		case EBML_ID_KRAD_COMPOSITOR_CMD_HEX_DEMO:

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_X) {
				//printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->hex_x = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_Y) {
				//printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->hex_y = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_SIZE) {
				//printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->hex_size = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);			

			/*
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					//printf("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
			*/		
			break;
	}

	return 0;
}

