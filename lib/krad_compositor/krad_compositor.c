#include "krad_compositor.h"

#ifdef KRAD_USE_WAYLAND

static void *krad_compositor_display_thread (void *arg);

typedef struct krad_compositor_wayland_display_St krad_compositor_wayland_display_t;

struct krad_compositor_wayland_display_St {
	krad_wayland_t *krad_wayland;
	krad_compositor_port_t *krad_compositor_port;
	void *buffer;
	int w;
	int h;
};


int krad_compositor_wayland_display_render_callback (void *pointer, uint32_t time) {

	krad_compositor_wayland_display_t *krad_compositor_wayland_display = (krad_compositor_wayland_display_t *)pointer;

  int ret;
  char buffer[1];
	int updated;
	krad_frame_t *krad_frame;
	
	updated = 0;

	krad_frame = krad_compositor_port_pull_frame (krad_compositor_wayland_display->krad_compositor_port);

	if (krad_frame != NULL) {
	
	  //FIXME do this first etc
    ret = read (krad_compositor_wayland_display->krad_compositor_port->socketpair[1], buffer, 1);
    if (ret != 1) {
      if (ret == 0) {
        printk ("Krad OTransponder: port read got EOF");
        return updated;
      }
      printk ("Krad OTransponder: port read unexpected read return value %d", ret);
    }
	
	
		memcpy (krad_compositor_wayland_display->buffer,
				krad_frame->pixels,
				krad_compositor_wayland_display->w * krad_compositor_wayland_display->h * 4);
		krad_framepool_unref_frame (krad_frame);
		updated = 1;
	} else {
		
	}


	return updated;
}

static void *krad_compositor_display_thread (void *arg) {

	krad_compositor_t *krad_compositor = (krad_compositor_t *)arg;

	krad_system_set_thread_name ("kr_wayland");

	krad_compositor_wayland_display_t *krad_compositor_wayland_display;
	
	krad_compositor_wayland_display = calloc (1, sizeof (krad_compositor_wayland_display_t));

	krad_compositor_wayland_display->krad_wayland = krad_wayland_create ();

	krad_compositor_get_resolution (krad_compositor,
							  &krad_compositor_wayland_display->w,
							  &krad_compositor_wayland_display->h);
	
	krad_compositor_wayland_display->krad_compositor_port = krad_compositor_port_create (krad_compositor, "WLOut", OUTPUT,
																						 krad_compositor_wayland_display->w,
																						 krad_compositor_wayland_display->h);

	//krad_wayland->render_test_pattern = 1;

	krad_wayland_set_frame_callback (krad_compositor_wayland_display->krad_wayland,
									 krad_compositor_wayland_display_render_callback,
									 krad_compositor_wayland_display);

	krad_wayland_prepare_window (krad_compositor_wayland_display->krad_wayland,
								 krad_compositor_wayland_display->w,
								 krad_compositor_wayland_display->h,
								 &krad_compositor_wayland_display->buffer);

	printk("Wayland display prepared");

	krad_wayland_open_window (krad_compositor_wayland_display->krad_wayland);

	printk("Wayland display running");

	while (krad_compositor->display_open == 1) {
		krad_wayland_iterate (krad_compositor_wayland_display->krad_wayland);
	}

  krad_wayland_close_window (krad_compositor_wayland_display->krad_wayland);

	krad_wayland_destroy (krad_compositor_wayland_display->krad_wayland);
	
	krad_compositor_port_destroy (krad_compositor, krad_compositor_wayland_display->krad_compositor_port);

	krad_compositor->display_open = 0;

	free (krad_compositor_wayland_display);

	return NULL;

}

#endif

void krad_compositor_open_display (krad_compositor_t *krad_compositor) {

#ifdef KRAD_USE_WAYLAND
	if (krad_compositor->display_open == 1) {
		krad_compositor_close_display (krad_compositor);
	}

	krad_compositor->display_open = 1;
	pthread_create (&krad_compositor->display_thread, NULL, krad_compositor_display_thread, (void *)krad_compositor);

#else
	printk("Wayland disabled: not opening a display");
#endif
}


void krad_compositor_close_display (krad_compositor_t *krad_compositor) {

#ifdef KRAD_USE_WAYLAND
	if (krad_compositor->display_open == 1) {
    printk("Wayland display closing");	
		krad_compositor->display_open = 2;
		pthread_join (krad_compositor->display_thread, NULL);
		krad_compositor->display_open = 0;
	  printk("Wayland display closed");
	} else {
	  printk("Wayland display wasn't open");
	}
#else
	printk("Wayland disabled: no need to close display");
#endif
}

void krad_compositor_add_text (krad_compositor_t *krad_compositor, krad_text_rep_t *krad_text_rep) {

	int s;
	krad_text_t *krad_text;
	
	krad_text = NULL;

	for (s = 0; s < KC_MAX_TEXTS; s++) {
		if (krad_compositor->krad_text[s].krad_compositor_subunit->active == 0) {
			krad_text = &krad_compositor->krad_text[s];
			krad_text->krad_compositor_subunit->active = 2;
      krad_text->krad_compositor_subunit->number = s;
			break;
		}
	}

  krad_compositor_subunit_set_xy (krad_text->krad_compositor_subunit, krad_text_rep->controls->x, krad_text_rep->controls->y);
  krad_compositor_subunit_set_z (krad_text->krad_compositor_subunit, krad_text_rep->controls->z);
  krad_compositor_subunit_set_scale (krad_text->krad_compositor_subunit, krad_text_rep->controls->xscale);
  krad_compositor_subunit_set_new_opacity (krad_text->krad_compositor_subunit, krad_text_rep->controls->opacity);
  krad_compositor_subunit_set_rotation (krad_text->krad_compositor_subunit, krad_text_rep->controls->rotation);
  krad_compositor_subunit_set_tickrate (krad_text->krad_compositor_subunit, krad_text_rep->controls->tickrate);

  krad_text_set_text (krad_text, krad_text_rep->text);	

	krad_text_set_font (krad_text, krad_text_rep->font);	

	krad_text_set_rgb (krad_text, krad_text_rep->red, krad_text_rep->green, krad_text_rep->blue);

	krad_text->krad_compositor_subunit->active = 1;
	krad_compositor->active_texts++;

}

void krad_compositor_set_text (krad_compositor_t *krad_compositor, krad_text_rep_t *krad_text_rep) {

	krad_text_t *krad_text;
	
	krad_text = NULL;

	krad_text = &krad_compositor->krad_text[krad_text_rep->controls->number];

	krad_compositor_subunit_set_new_xy (krad_text->krad_compositor_subunit, krad_text_rep->controls->x, krad_text_rep->controls->y);
	krad_compositor_subunit_set_new_scale (krad_text->krad_compositor_subunit, krad_text_rep->controls->xscale);
	krad_compositor_subunit_set_new_opacity (krad_text->krad_compositor_subunit, krad_text_rep->controls->opacity);
	krad_compositor_subunit_set_new_rotation (krad_text->krad_compositor_subunit, krad_text_rep->controls->rotation);
	krad_compositor_subunit_set_tickrate (krad_text->krad_compositor_subunit, krad_text_rep->controls->tickrate);

	krad_text_set_new_rgb (krad_text, krad_text_rep->red, krad_text_rep->green, krad_text_rep->blue);

}

void krad_compositor_remove_text (krad_compositor_t *krad_compositor, int num) {

	krad_text_t *krad_text;
	
	krad_text = NULL;

	krad_text = &krad_compositor->krad_text[num];

	if (krad_text->krad_compositor_subunit->active != 1) {
		return;
	}

	krad_text->krad_compositor_subunit->active = 3;

	usleep (100000);
	krad_text_reset (krad_text);
	
	krad_text->krad_compositor_subunit->active = 0;
	krad_compositor->active_texts--;

}


void krad_compositor_add_sprite (krad_compositor_t *krad_compositor, krad_sprite_rep_t *krad_sprite_rep) {

	int s;
	krad_sprite_t *krad_sprite;
	
	krad_sprite = NULL;

	if ((krad_sprite_rep->filename == NULL) || (strlen(krad_sprite_rep->filename) == 0)) {
	  return;
	}

  for (s = 0; s < KC_MAX_SPRITES; s++) {
	  if (krad_compositor->krad_sprite[s].krad_compositor_subunit->active == 0) {
		  krad_sprite = &krad_compositor->krad_sprite[s];
		  krad_sprite->krad_compositor_subunit->active = 2;
		  break;
	  }
  }
	
	if (krad_sprite_open_file (krad_sprite, krad_sprite_rep->filename)) {
	

	  krad_compositor_subunit_set_xy (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.x, krad_sprite_rep->controls.y);
    krad_compositor_subunit_set_z (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.z);
	  krad_compositor_subunit_set_scale (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.xscale);
    krad_compositor_subunit_set_new_opacity (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.opacity);
	  krad_compositor_subunit_set_rotation (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.rotation);
	  krad_compositor_subunit_set_tickrate (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.tickrate);

	  krad_sprite->krad_compositor_subunit->active = 1;
	  krad_compositor->active_sprites++;

	} else {
    krad_sprite->krad_compositor_subunit->active = 0;	  
	}

}

void krad_compositor_set_sprite (krad_compositor_t *krad_compositor, 
                                 krad_sprite_rep_t *krad_sprite_rep) {

	krad_sprite_t *krad_sprite;
	
	krad_sprite = NULL;

	krad_sprite = &krad_compositor->krad_sprite[krad_sprite_rep->controls.number];

	krad_compositor_subunit_set_new_xy (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.x, krad_sprite_rep->controls.y);
  krad_compositor_subunit_set_z (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.z);
	krad_compositor_subunit_set_new_scale (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.xscale);
	krad_compositor_subunit_set_new_opacity (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.opacity);
	krad_compositor_subunit_set_new_rotation (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.rotation);
	krad_compositor_subunit_set_tickrate (krad_sprite->krad_compositor_subunit, krad_sprite_rep->controls.tickrate);

}

void krad_compositor_remove_sprite (krad_compositor_t *krad_compositor, int num) {

	krad_sprite_t *krad_sprite;
	
	krad_sprite = NULL;

	krad_sprite = &krad_compositor->krad_sprite[num];

	if (krad_sprite->krad_compositor_subunit->active != 1) {
		return;
	}

	krad_sprite->krad_compositor_subunit->active = 3;

	usleep (100000);
	krad_sprite_reset (krad_sprite);
	
	krad_sprite->krad_compositor_subunit->active = 0;
	krad_compositor->active_sprites--;

}

void krad_compositor_unset_background (krad_compositor_t *krad_compositor) {
	if (krad_compositor->background->krad_compositor_subunit->active != 1) {
		return;
	}
	krad_compositor->background->krad_compositor_subunit->active = 3;
	usleep (100000);
	krad_sprite_reset (krad_compositor->background);
	krad_compositor->background->krad_compositor_subunit->active = 0;
}

void krad_compositor_set_background (krad_compositor_t *krad_compositor, char *filename) {
	krad_compositor_unset_background (krad_compositor);

	if ((filename == NULL) || (strlen(filename) == 0)) {
	  return;
	}
	
	if (krad_sprite_open_file (krad_compositor->background, filename)) {
	  krad_compositor->background->krad_compositor_subunit->active = 1;
	} else {
    krad_compositor->background->krad_compositor_subunit->active = 0;	  
	}
}

void krad_compositor_render_background (krad_compositor_t *krad_compositor) {

	if (krad_compositor->background->krad_compositor_subunit->active != 1) {
		return;
	}
  cairo_save (krad_compositor->cr);
  if ((krad_compositor->background->krad_compositor_subunit->width != krad_compositor->width) || (krad_compositor->background->krad_compositor_subunit->height != krad_compositor->height)) {
		cairo_set_source (krad_compositor->cr, krad_compositor->background->sprite_pattern);
  } else {
		cairo_set_source_surface ( krad_compositor->cr, krad_compositor->background->sprite, 0, 0 );
  }
	cairo_paint ( krad_compositor->cr );
  cairo_restore (krad_compositor->cr);
}

int krad_compositor_has_background (krad_compositor_t *krad_compositor) {
  if (krad_compositor->background->krad_compositor_subunit->active == 1) {
    return 1;
	}
  return 0;
}

void krad_compositor_render_no_input_text (krad_compositor_t *krad_compositor) {
  cairo_save (krad_compositor->cr);
  cairo_set_source_rgb (krad_compositor->cr, GREY);
  cairo_select_font_face (krad_compositor->cr, "monospace",
					    CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (krad_compositor->cr, 96.0);
  cairo_move_to (krad_compositor->cr, krad_compositor->width/3.0,
		       krad_compositor->height/2);
  cairo_show_text (krad_compositor->cr, "NO INPUT");
  cairo_restore (krad_compositor->cr);
}

void krad_compositor_clear_frame (krad_compositor_t *krad_compositor) {
	cairo_save (krad_compositor->cr);
	cairo_set_source_rgba (krad_compositor->cr, BGCOLOR_CLR);
	cairo_set_operator (krad_compositor->cr, CAIRO_OPERATOR_SOURCE);	
	cairo_paint (krad_compositor->cr);
	cairo_restore (krad_compositor->cr);
}

void krad_compositor_videoport_render_frame (krad_compositor_t *krad_compositor, krad_compositor_port_t *port, krad_frame_t *frame) {

  if (port->krad_compositor_subunit->opacity == 0.0f) {

    krad_compositor_subunit_update (port->krad_compositor_subunit);  

	  return;
  }
  cairo_save (krad_compositor->cr);

  if (port->krad_compositor_subunit->rotation != 0.0f) {
	  cairo_translate (krad_compositor->cr, port->crop_width / 2, port->crop_height / 2);
	  cairo_rotate (krad_compositor->cr, port->krad_compositor_subunit->rotation * (M_PI/180.0));
	  cairo_translate (krad_compositor->cr, port->crop_width / -2, port->crop_height / -2);
  }

  cairo_set_source_surface (krad_compositor->cr, frame->cst,
                            port->krad_compositor_subunit->x - port->crop_x, port->krad_compositor_subunit->y - port->crop_y);

  cairo_rectangle (krad_compositor->cr,
                   port->krad_compositor_subunit->x, port->krad_compositor_subunit->y,
                   port->crop_width, port->crop_height);

  cairo_clip (krad_compositor->cr);

  if (port->krad_compositor_subunit->opacity == 1.0f) {
	  cairo_paint (krad_compositor->cr);
  } else {
	  cairo_paint_with_alpha (krad_compositor->cr, port->krad_compositor_subunit->opacity);
  }
  cairo_restore (krad_compositor->cr);
  
  krad_compositor_subunit_update (port->krad_compositor_subunit);
  
}

void krad_compositor_process (krad_compositor_t *krad_compositor) {

	int p;
	krad_frame_t *composite_frame;
	krad_frame_t *frame;	
	
	frame = NULL;	
	composite_frame = NULL;

	krad_compositor->timecode = round (1000000000 * krad_compositor->frame_num / krad_compositor->frame_rate_numerator * krad_compositor->frame_rate_denominator / 1000000);
	krad_compositor->frame_num++;
	
	if (krad_compositor->active_ports < 1) {
		krad_compositor->skipped_processes++;
		if (krad_compositor->skipped_processes == 200) {
			pthread_mutex_lock (&krad_compositor->settings_lock);		
			krad_compositor_free_resources (krad_compositor);
			pthread_mutex_unlock (&krad_compositor->settings_lock);
		}
		return;
	} else {
		krad_compositor->skipped_processes = 0;
	}

	/* Get a frame to composite on */
	
	do {
		composite_frame = krad_framepool_getframe (krad_compositor->krad_framepool);
		if (composite_frame == NULL) {
			printke ("This is very bad! Compositor wanted a frame but could not get one right away!");
			usleep (5000);
		}
	} while (composite_frame == NULL);	
	
  krad_compositor->cr = cairo_create (composite_frame->cst);

  /* Render background if exists */
	
	if (krad_compositor_has_background (krad_compositor)) {
    krad_compositor->no_input = 0;	
    krad_compositor_render_background (krad_compositor);
  } else {

    /* No background, so clear frame */
  	krad_compositor_clear_frame (krad_compositor);

    /* Handle situation of maybe having absolutly no input */  
	  if ((krad_compositor->active_input_ports == 0) &&
	      (krad_compositor->active_sprites == 0) &&
	      (krad_compositor->active_texts == 0)) {
			
	    krad_compositor->no_input++;

	    if ((krad_compositor->no_input > 140) && ((krad_compositor->frame_num % 30) > 15)) {
        krad_compositor_render_no_input_text (krad_compositor);
	    }
    } else {
      krad_compositor->no_input = 0;
    }
  }
  
	/* Composite Input Ports / Sprites / Texts */


	for (p = 0; p < KC_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].krad_compositor_subunit->active == 1) && (krad_compositor->port[p].direction == INPUT)) {
			frame = krad_compositor_port_pull_frame (&krad_compositor->port[p]);		
			if (frame != NULL) {
        krad_compositor_videoport_render_frame (krad_compositor, &krad_compositor->port[p], frame);
				krad_framepool_unref_frame (frame);
			}
		}
	}


	for (p = 0; p < KC_MAX_SPRITES; p++) {
		if (krad_compositor->krad_sprite[p].krad_compositor_subunit->active == 1) {
			krad_sprite_render (&krad_compositor->krad_sprite[p], krad_compositor->cr);
		}
	}
	

	for (p = 0; p < KC_MAX_TEXTS; p++) {
		if (krad_compositor->krad_text[p].krad_compositor_subunit->active == 1) {
			krad_text_render (&krad_compositor->krad_text[p], krad_compositor->cr);
		}
	}	
	
	/* Push out the composited frame */

	for (p = 0; p < KC_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].krad_compositor_subunit->active == 1) && (krad_compositor->port[p].direction == OUTPUT)) {
			krad_compositor_port_push_frame (&krad_compositor->port[p], composite_frame);
		}
	}
	
	if (krad_compositor->snapshot > 0) {
		krad_compositor_take_snapshot (krad_compositor, composite_frame, SNAPPNG);
	}
	
	if (krad_compositor->snapshot_jpeg > 0) {
		krad_compositor_take_snapshot (krad_compositor, composite_frame, SNAPJPEG);
	}	
	
	krad_framepool_unref_frame (composite_frame);

	if (krad_compositor->cr != NULL) {
    cairo_destroy (krad_compositor->cr);
	}

}

void krad_compositor_get_last_snapshot_name (krad_compositor_t *krad_compositor, char *filename) {
	
	if (filename == NULL) {
		return;
	}
	
	pthread_mutex_lock (&krad_compositor->last_snapshot_name_lock);

	strcpy (filename, krad_compositor->last_snapshot_name);
	
	pthread_mutex_unlock (&krad_compositor->last_snapshot_name_lock);

}

void krad_compositor_set_last_snapshot_name (krad_compositor_t *krad_compositor, char *filename) {

	
	pthread_mutex_lock (&krad_compositor->last_snapshot_name_lock);

	strcpy (krad_compositor->last_snapshot_name, filename);
	
	pthread_mutex_unlock (&krad_compositor->last_snapshot_name_lock);

}

void *krad_compositor_snapshot_thread (void *arg) {

	krad_compositor_snapshot_t *krad_compositor_snapshot = (krad_compositor_snapshot_t *)arg;
	
	tjhandle jpeg;
	int jpeg_fd;	
	unsigned char *jpeg_buf;
	long unsigned int jpeg_size;
	int ret;

	jpeg_buf = NULL;

	if (krad_compositor_snapshot->jpeg == 1) {

		jpeg = tjInitCompress ();

		ret = tjCompress2 (jpeg,
							 (unsigned char *)krad_compositor_snapshot->krad_frame->pixels,
							 krad_compositor_snapshot->width,
							 krad_compositor_snapshot->width * 4,
							 krad_compositor_snapshot->height,
							 TJPF_BGRA,
							 &jpeg_buf,
							 &jpeg_size,
							 TJSAMP_444,
							 90,
							 0);
		if (ret == 0) {
			jpeg_fd = open (krad_compositor_snapshot->filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (jpeg_fd > 0) {
				while (ret != jpeg_size) {
					// FIXME need ability to fail here
					ret += write (jpeg_fd, jpeg_buf + ret, jpeg_size - ret);
				}
				close (jpeg_fd);
			}
			tjFree (jpeg_buf);
		} else {
			printke("Krad Compositor: JPEG Snapshot error: %s", tjGetErrorStr());
		}
		tjDestroy ( jpeg );
	} else {
		//FIXME need to monitor for fail
		cairo_surface_write_to_png (krad_compositor_snapshot->krad_frame->cst, krad_compositor_snapshot->filename);
	}
	krad_framepool_unref_frame (krad_compositor_snapshot->krad_frame);
	
	krad_compositor_set_last_snapshot_name (krad_compositor_snapshot->krad_compositor, krad_compositor_snapshot->filename);
	
	free (krad_compositor_snapshot);

	return NULL;

}

void krad_compositor_take_snapshot (krad_compositor_t *krad_compositor, krad_frame_t *krad_frame, krad_snapshot_fmt_t format) {
		
	krad_compositor_snapshot_t *krad_compositor_snapshot;
	char *ext;

	if (krad_compositor->dir == NULL) {	
		return;
	}

	krad_framepool_ref_frame (krad_frame);
		
	krad_compositor_snapshot = calloc (1, sizeof (krad_compositor_snapshot_t));

	if (format == SNAPPNG) {
		krad_compositor->snapshot--;
		ext = "png";
	} else {
		krad_compositor->snapshot_jpeg--;
		krad_compositor_snapshot->jpeg = 1;
		ext = "jpg";
	}
	
	krad_compositor_snapshot->krad_frame = krad_frame;
	krad_compositor_snapshot->krad_compositor = krad_compositor;
	krad_compositor_snapshot->width = krad_compositor->width;
	krad_compositor_snapshot->height = krad_compositor->height;	
	
	sprintf (krad_compositor_snapshot->filename,
			 "%s/snapshot_%"PRIu64"_%"PRIu64".%s",
			 krad_compositor->dir,
			 ktime(),
			 krad_compositor->frame_num,
			 ext);
	
	pthread_create (&krad_compositor->snapshot_thread,
					NULL,
					krad_compositor_snapshot_thread,
				    (void *)krad_compositor_snapshot);

	pthread_detach (krad_compositor->snapshot_thread);

}


void krad_compositor_passthru_process (krad_compositor_t *krad_compositor) {

	int p;
	
	krad_frame_t *krad_frame;
	
	krad_frame = NULL;


	for (p = 0; p < KC_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].krad_compositor_subunit->active == 1) && (krad_compositor->port[p].direction == INPUT) &&
			(krad_compositor->port[p].passthru == 1)) {
				krad_frame = krad_compositor_port_pull_frame (&krad_compositor->port[p]);
			break;
		}
	}
	
	if (krad_frame == NULL) {
		return;
	}


	for (p = 0; p < KC_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].krad_compositor_subunit->active == 1) && (krad_compositor->port[p].direction == OUTPUT) &&
			(krad_compositor->port[p].passthru == 1)) {

			krad_compositor_port_push_frame (&krad_compositor->port[p], krad_frame);
			break;
		}
	}
	
	krad_framepool_unref_frame (krad_frame);

}

void krad_compositor_port_set_comp_params (krad_compositor_port_t *krad_compositor_port,
										   int x, int y, int width, int height, 
										   int crop_x, int crop_y,
										   int crop_width, int crop_height, float opacity, float rotation) {
										   
	printkd ("comp compy params func called");
 
  if ((x != krad_compositor_port->krad_compositor_subunit->x) || (y != krad_compositor_port->krad_compositor_subunit->y)) {
    krad_compositor_subunit_set_new_xy (krad_compositor_port->krad_compositor_subunit, x, y);
  }

  krad_compositor_port->krad_compositor_subunit->width = width;
  krad_compositor_port->krad_compositor_subunit->height = height;

  krad_compositor_port->crop_x = crop_x;
  krad_compositor_port->crop_y = crop_y;

  krad_compositor_port->crop_width = crop_width;
  krad_compositor_port->crop_height = crop_height;

  if (opacity != krad_compositor_port->krad_compositor_subunit->opacity) {
    krad_compositor_subunit_set_new_opacity (krad_compositor_port->krad_compositor_subunit, opacity);
  }

  if (rotation != krad_compositor_port->krad_compositor_subunit->rotation) {
    krad_compositor_subunit_set_new_rotation (krad_compositor_port->krad_compositor_subunit, rotation);
  }  

  krad_compositor_port->comp_params_updated = 1;

}

void krad_compositor_port_set_io_params (krad_compositor_port_t *krad_compositor_port,
										 int width, int height) {

  
  int x;
  int y;
    
  x = 0;
  y = 0;
  
	printkd ("comp io params func called");
										 
	krad_compositor_port->source_width = width;
	krad_compositor_port->source_height = height;

	krad_compositor_aspect_scale (krad_compositor_port->source_width, krad_compositor_port->source_height,
								  krad_compositor_port->krad_compositor->width, krad_compositor_port->krad_compositor->height,
								  &krad_compositor_port->krad_compositor_subunit->width, &krad_compositor_port->krad_compositor_subunit->height);

	krad_compositor_port->crop_width = krad_compositor_port->krad_compositor_subunit->width;
	krad_compositor_port->crop_height = krad_compositor_port->krad_compositor_subunit->height;

  if (krad_compositor_port->krad_compositor_subunit->width < krad_compositor_port->krad_compositor->width) {
    x = (krad_compositor_port->krad_compositor->width - krad_compositor_port->krad_compositor_subunit->width) / 2;
  }
  
  if (krad_compositor_port->krad_compositor_subunit->height < krad_compositor_port->krad_compositor->height) {
    y = (krad_compositor_port->krad_compositor->height - krad_compositor_port->krad_compositor_subunit->height) / 2;
  }

  krad_compositor_subunit_set_xy (krad_compositor_port->krad_compositor_subunit, x, y);

  krad_compositor_port->io_params_updated = 1;

}

void krad_compositor_port_push_yuv_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame) {

  int dststride;
  
  if (krad_compositor_port->krad_compositor->width > krad_compositor_port->krad_compositor_subunit->width) {
    dststride = krad_compositor_port->krad_compositor->width;
  } else {
    dststride = krad_compositor_port->krad_compositor_subunit->width;
  }

	int rgb_stride_arr[3] = {4*dststride, 0, 0};
	unsigned char *dst[4];
	
	if ((krad_compositor_port->io_params_updated) || (krad_compositor_port->comp_params_updated)) {
		if (krad_compositor_port->sws_converter != NULL) {
			sws_freeContext ( krad_compositor_port->sws_converter );
			krad_compositor_port->sws_converter = NULL;
		}
		krad_compositor_port->io_params_updated = 0;
		krad_compositor_port->comp_params_updated = 0;
		printkd ("I knew about the update");
	}
	
	if (krad_compositor_port->yuv_color_depth != krad_frame->format) {
		if (krad_compositor_port->sws_converter != NULL) {
			sws_freeContext ( krad_compositor_port->sws_converter );
			krad_compositor_port->sws_converter = NULL;
		}
		krad_compositor_port->yuv_color_depth = krad_frame->format;
	}	
	
	if (krad_compositor_port->sws_converter == NULL) {

		krad_compositor_port->sws_converter =
			sws_getContext ( krad_compositor_port->source_width,
							 krad_compositor_port->source_height,
							 krad_compositor_port->yuv_color_depth,
							 krad_compositor_port->krad_compositor_subunit->width,
							 krad_compositor_port->krad_compositor_subunit->height,
							 PIX_FMT_RGB32, 
							 SWS_BICUBIC,
							 NULL, NULL, NULL);
			
		printk ("compositor port scaling now: %dx%d [%dx%d]-> %dx%d",
				krad_compositor_port->source_width,
				krad_compositor_port->source_height,
				krad_compositor_port->crop_width,
				krad_compositor_port->crop_height,				
				krad_compositor_port->krad_compositor_subunit->width,
				krad_compositor_port->krad_compositor_subunit->height);				 
							 

	}									 

	dst[0] = (unsigned char *)krad_frame->pixels;

	sws_scale (krad_compositor_port->sws_converter,
			  (const uint8_t * const*)krad_frame->yuv_pixels,
			   krad_frame->yuv_strides, 0, krad_compositor_port->source_height, dst, rgb_stride_arr);

	krad_compositor_port_push_frame (krad_compositor_port, krad_frame);

}

krad_frame_t *krad_compositor_port_pull_yuv_frame (krad_compositor_port_t *krad_compositor_port,
												   uint8_t *yuv_pixels[4], int yuv_strides[4], int color_depth) {

	krad_frame_t *krad_frame;	
	
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
	
	if (krad_compositor_port->yuv_color_depth != color_depth) {
		if (krad_compositor_port->sws_converter != NULL) {
			sws_freeContext ( krad_compositor_port->sws_converter );
			krad_compositor_port->sws_converter = NULL;
		}
		krad_compositor_port->yuv_color_depth = color_depth;
	}
	
	if (krad_compositor_port->sws_converter == NULL) {

		krad_compositor_port->sws_converter =
			sws_getContext ( krad_compositor_port->krad_compositor->width,
							 krad_compositor_port->krad_compositor->height,
							 PIX_FMT_RGB32,
							 krad_compositor_port->krad_compositor_subunit->width,
							 krad_compositor_port->krad_compositor_subunit->height,
							 krad_compositor_port->yuv_color_depth, 
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

void krad_compositor_port_push_rgba_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame) {


	int output_rgb_stride_arr[4] = {4*krad_compositor_port->krad_compositor->width, 0, 0, 0};
	int input_rgb_stride_arr[4] = {4*krad_compositor_port->source_width, 0, 0, 0};	
	unsigned char *dst[4];
	unsigned char *src[4];	
	krad_frame_t *scaled_frame;	
	
	krad_frame->format = PIX_FMT_RGB32;
	
	if ((krad_compositor_port->source_width != krad_compositor_port->krad_compositor_subunit->width) ||
		(krad_compositor_port->source_height != krad_compositor_port->krad_compositor_subunit->height)) {
	
		if ((krad_compositor_port->io_params_updated) || (krad_compositor_port->comp_params_updated)) {
			if (krad_compositor_port->sws_converter != NULL) {
				sws_freeContext ( krad_compositor_port->sws_converter );
				krad_compositor_port->sws_converter = NULL;
			}
			krad_compositor_port->io_params_updated = 0;
			krad_compositor_port->comp_params_updated = 0;
			printkd ("I knew about the rgb update");
		}
	
		if (krad_compositor_port->sws_converter == NULL) {

			krad_compositor_port->sws_converter =
				sws_getContext ( krad_compositor_port->source_width,
								 krad_compositor_port->source_height,
								 krad_frame->format,
								 krad_compositor_port->krad_compositor_subunit->width,
								 krad_compositor_port->krad_compositor_subunit->height,
								 PIX_FMT_RGB32, 
								 SWS_BICUBIC,
								 NULL, NULL, NULL);
				
			if (krad_compositor_port->sws_converter == NULL) {
				failfast ("Krad Compositor: fraked");
			}
								 
			printk ("set scaling to w %d h %d sw %d sh %d",
					krad_compositor_port->krad_compositor_subunit->width,
					krad_compositor_port->krad_compositor_subunit->height,
					krad_compositor_port->source_width,
					krad_compositor_port->source_height);							 
								 

		}
		

    while (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= (sizeof(krad_frame_t *) * 30)) {
      usleep (18000);
      //kludge to not buffer more than 1 handfull? of frames ahead for fast sources
    }
		
		scaled_frame = krad_framepool_getframe (krad_compositor_port->krad_compositor->krad_framepool);					 

		if (scaled_frame == NULL) {
			failfast ("Krad Compositor: out of frames");
		}

		src[0] = (unsigned char *)krad_frame->pixels;
		dst[0] = (unsigned char *)scaled_frame->pixels;

		sws_scale (krad_compositor_port->sws_converter, (const uint8_t * const*)src,
				   input_rgb_stride_arr, 0, krad_compositor_port->source_height, dst, output_rgb_stride_arr);
				   

		krad_compositor_port_push_frame (krad_compositor_port, scaled_frame);
		
		krad_framepool_unref_frame (scaled_frame);
	
	} else {

		krad_compositor_port_push_frame (krad_compositor_port, krad_frame);

	}

}

void krad_compositor_port_push_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame) {

  int wrote;

	krad_framepool_ref_frame (krad_frame);
	
	krad_ringbuffer_write (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));

  if (krad_compositor_port->direction == OUTPUT) {
    wrote = write (krad_compositor_port->socketpair[0], "0", 1);
    if (wrote != 1) {
      printk ("Krad Compositor: port write unexpected write return value %d", wrote);
    }
  }

}


krad_frame_t *krad_compositor_port_pull_frame_local (krad_compositor_port_t *krad_compositor_port) {

	int ret;
	int wrote;
	char buf[1];
	
	static int frames = 0;
	
	ret = 0;
	wrote = 0;
	buf[0] = 0;

	cairo_surface_flush (krad_compositor_port->local_frame->cst);

	wrote = write (krad_compositor_port->msg_sd, buf, 1);

	if (wrote == 1) {
		ret = read (krad_compositor_port->msg_sd, buf, 1);
		if (ret == 1) {
			frames++;
			
			cairo_surface_mark_dirty (krad_compositor_port->local_frame->cst);
			
			if (frames == 5) {
				cairo_surface_write_to_png (krad_compositor_port->local_frame->cst, "/home/oneman/kode/kaf2.png");
			}
		
			return krad_compositor_port->local_frame;
		}
	}
	
	return NULL;
}


krad_frame_t *krad_compositor_port_pull_frame (krad_compositor_port_t *krad_compositor_port) {

	krad_frame_t *krad_frame;
	
	if (krad_compositor_port->local == 1) {
		return krad_compositor_port_pull_frame_local (krad_compositor_port);
	}
	
	if ((krad_compositor_port->direction == INPUT) && (krad_compositor_port->passthru == 0)) {

		if (krad_compositor_port->last_frame != NULL) {

			//printk ("%llu -- %llu", krad_compositor_port->start_timecode + krad_compositor_port->last_frame->timecode,
			//krad_compositor_port->krad_compositor->timecode);

			if ((krad_compositor_port->start_timecode + krad_compositor_port->last_frame->timecode) < 
				krad_compositor_port->krad_compositor->timecode) {

				if (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= sizeof(krad_frame_t *)) {
					krad_ringbuffer_read (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));

					krad_framepool_unref_frame (krad_compositor_port->last_frame);	
					krad_framepool_ref_frame (krad_frame);
					krad_compositor_port->last_frame = krad_frame;
		
					return krad_frame;
				} else {
					krad_framepool_ref_frame (krad_compositor_port->last_frame);
					return krad_compositor_port->last_frame;
				}

			} else {
				krad_framepool_ref_frame (krad_compositor_port->last_frame);
				return krad_compositor_port->last_frame;
			}
		} else {
		
			if (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= sizeof(krad_frame_t *)) {
				krad_ringbuffer_read (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));
	
				krad_compositor_port->start_timecode = krad_compositor_port->krad_compositor->timecode;
	
				krad_framepool_ref_frame (krad_frame);
				krad_compositor_port->last_frame = krad_frame;
	
				return krad_frame;
			}
		
		
		}
		
	} else {
		
		if (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= sizeof(krad_frame_t *)) {
			krad_ringbuffer_read (krad_compositor_port->frame_ring, (char *)&krad_frame, sizeof(krad_frame_t *));			
			return krad_frame;
		}
	}

	return NULL;

}

int krad_compositor_port_frames_avail (krad_compositor_port_t *krad_compositor_port) {
	
	int frames;
	
	frames = krad_ringbuffer_read_space (krad_compositor_port->frame_ring);
	
	frames = frames / sizeof (krad_frame_t *);

	return frames;
}

void krad_compositor_free_resources (krad_compositor_t *krad_compositor) {

	if (krad_compositor->mask_cst != NULL) {
		cairo_surface_destroy (krad_compositor->mask_cst);
		krad_compositor->mask_cst = NULL;
		if (krad_compositor->mask_cr != NULL) {
			cairo_destroy (krad_compositor->mask_cr);
			krad_compositor->mask_cr = NULL;
		}
	}

	if (krad_compositor->krad_framepool != NULL) {
		krad_framepool_destroy ( krad_compositor->krad_framepool );
		krad_compositor->krad_framepool = NULL;
	}
	
  printk ("Krad Compositor: Free'd Resources");
	
}

void krad_compositor_alloc_resources (krad_compositor_t *krad_compositor) {

  printk ("Krad Compositor: Allocing Resources");
  printk ("Krad Compositor: Cairo Version: %s", cairo_version_string());

	if (krad_compositor->mask_cst == NULL) {
		krad_compositor->mask_cst = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
																krad_compositor->width,
																krad_compositor->height);
		krad_compositor->mask_cr = cairo_create (krad_compositor->mask_cst);															
	}

	if (krad_compositor->krad_framepool == NULL) {
		krad_compositor->krad_framepool = 
			krad_framepool_create ( krad_compositor->width, krad_compositor->height, DEFAULT_COMPOSITOR_BUFFER_FRAMES);
	}
	
  krad_compositor->krad_text_rep_ipc = krad_compositor_text_rep_create();
  
}

void krad_compositor_aspect_scale (int width, int height,
								   int avail_width, int avail_height,
								   int *new_width, int *new_heigth) {
	
	double scaleX, scaleY, scale;

	scaleX = (float)avail_width  / width;  
	scaleY = (float)avail_height / height;  
	scale = MIN ( scaleX, scaleY );
	
	*new_width = width * scale;
	*new_heigth = height * scale;
	
	printk ("Source: %d x %d Max: %d x %d Aspect Constrained: %d x %d",
			width, height,
			avail_width, avail_height,
			*new_width, *new_heigth);
	
}

int krad_compositor_port_get_fd (krad_compositor_port_t *krad_compositor_port) {
  return krad_compositor_port->socketpair[1];
}

krad_compositor_port_t *krad_compositor_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction,
													 int width, int height) {
							
	return krad_compositor_port_create_full (krad_compositor, sysname, direction,
											 width, height, 0, 0);
													 
}

krad_compositor_port_t *krad_compositor_port_create_full (krad_compositor_t *krad_compositor, char *sysname, int direction,
													 int width, int height, int holdlock, int local) {

	krad_compositor_port_t *krad_compositor_port;

	int p;
  int x;
  int y;
  
  x = 0;
  y = 0;
	
  pthread_mutex_lock (&krad_compositor->settings_lock);


	for (p = 0; p < KC_MAX_PORTS; p++) {
		if (krad_compositor->port[p].krad_compositor_subunit->active == 0) {
			krad_compositor_port = &krad_compositor->port[p];
      if (krad_compositor_port->krad_compositor_subunit) {
        krad_compositor_subunit_destroy (krad_compositor_port->krad_compositor_subunit);
      }
      
      krad_compositor_port->krad_compositor_subunit = krad_compositor_subunit_create ();
        
			krad_compositor_port->krad_compositor_subunit->active = 2;
			break;
		}
	}
	
	if (krad_compositor_port == NULL) {
		return NULL;
	}
	
	krad_compositor_port->krad_compositor = krad_compositor;
	krad_compositor_port->local = local;
	krad_compositor_port->direction = direction;	
	strcpy (krad_compositor_port->sysname, sysname);	
	krad_compositor_port->start_timecode = 1;

	krad_compositor_port->crop_x = 0;
	krad_compositor_port->crop_y = 0;
	
  if (krad_compositor_port->direction == INPUT) {
    krad_compositor_port->source_width = width;
    krad_compositor_port->source_height = height;
    krad_compositor_aspect_scale (krad_compositor_port->source_width, krad_compositor_port->source_height,
                                  krad_compositor->width, krad_compositor->height,
                                  &krad_compositor_port->krad_compositor_subunit->width, &krad_compositor_port->krad_compositor_subunit->height);
									  
    if (krad_compositor_port->krad_compositor_subunit->width < krad_compositor_port->krad_compositor->width) {
      x = (krad_compositor_port->krad_compositor->width - krad_compositor_port->krad_compositor_subunit->width) / 2;
    }
	
	  if (krad_compositor_port->krad_compositor_subunit->height < krad_compositor_port->krad_compositor->height) {
      y = (krad_compositor_port->krad_compositor->height - krad_compositor_port->krad_compositor_subunit->height) / 2;
    }
    
    krad_compositor_subunit_set_xy (krad_compositor_port->krad_compositor_subunit, x, y);  
    
		krad_compositor_port->crop_width = krad_compositor_port->krad_compositor_subunit->width;
		krad_compositor_port->crop_height = krad_compositor_port->krad_compositor_subunit->height;
			
	} else {
	
	  //OUTPUT
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, krad_compositor_port->socketpair)) {
      printk ("Krad Compositor: subunit could not create socketpair errno: %d", errno);
      return NULL;
    }
	
		krad_compositor_port->source_width = krad_compositor->width;
		krad_compositor_port->source_height = krad_compositor->height;
		krad_compositor_port->krad_compositor_subunit->width = width;
		krad_compositor_port->krad_compositor_subunit->height = height;
		krad_compositor_port->crop_width = krad_compositor->width;
		krad_compositor_port->crop_height = krad_compositor->height;
		krad_compositor_port->yuv_color_depth = PIX_FMT_YUV420P;
	}
	
	if (strstr(krad_compositor_port->sysname, "passthru") != NULL) {
		krad_compositor_port->passthru = 1;
		krad_compositor->active_passthru_ports++;
	
		krad_compositor_port->frame_ring = 
		krad_ringbuffer_create ( DEFAULT_COMPOSITOR_BUFFER_FRAMES * 5 * sizeof(krad_frame_t *) );
		if (krad_compositor_port->frame_ring == NULL) {
			failfast ("oh dearringp im out of mem");
		}
	} else {

		krad_compositor_alloc_resources (krad_compositor);

		krad_compositor_port->frame_ring = 
		krad_ringbuffer_create ( DEFAULT_COMPOSITOR_BUFFER_FRAMES * sizeof(krad_frame_t *) );

		if (krad_compositor_port->local == 0) {
			krad_compositor_port->frame_ring = 
			krad_ringbuffer_create ( DEFAULT_COMPOSITOR_BUFFER_FRAMES * sizeof(krad_frame_t *) );
			if (krad_compositor_port->frame_ring == NULL) {
				failfast ("oh dearring im out of mem");
			}
		} else {
			krad_compositor_port->frame_ring = NULL;
		}

		krad_compositor->active_ports++;
		if (krad_compositor_port->direction == INPUT) {
			krad_compositor->active_input_ports++;
		}
		if (krad_compositor_port->direction == OUTPUT) {
			krad_compositor->active_output_ports++;
		}
	}

	if (holdlock == 0) {
		krad_compositor_port->krad_compositor_subunit->active = 1;
		pthread_mutex_unlock (&krad_compositor->settings_lock);		
	}
	
	return krad_compositor_port;

}

krad_compositor_port_t *krad_compositor_passthru_port_create (krad_compositor_t *krad_compositor,
														   char *sysname, int direction) {

	krad_compositor_port_t *krad_compositor_port;
	krad_compositor_port = krad_compositor_port_create (krad_compositor, sysname, direction,
														1,1);
	return krad_compositor_port;	
}


krad_compositor_port_t *krad_compositor_local_port_create (krad_compositor_t *krad_compositor,
														   char *sysname, int direction, int shm_sd, int msg_sd) {

	krad_compositor_port_t *krad_compositor_port;
	krad_compositor_port = krad_compositor_port_create_full (krad_compositor, sysname, direction,
														krad_compositor->width, krad_compositor->height, 1, 1);

	krad_compositor_port->shm_sd = 0;
	krad_compositor_port->msg_sd = 0;	
	krad_compositor_port->local_buffer = NULL;
	krad_compositor_port->local_buffer_size = 960 * 540 * 4 * 2;
	
	krad_compositor_port->shm_sd = shm_sd;
	krad_compositor_port->msg_sd = msg_sd;
	
	krad_compositor_port->local_buffer = mmap (NULL, krad_compositor_port->local_buffer_size,
											   PROT_READ | PROT_WRITE, MAP_SHARED,
											   krad_compositor_port->shm_sd, 0);

	if ((krad_compositor_port->local_buffer != NULL) && (krad_compositor_port->shm_sd != 0) &&
		(krad_compositor_port->msg_sd != 0)) {
		
		krad_compositor_port->local_frame = calloc (1, sizeof(krad_frame_t));
		if (krad_compositor_port->local_frame == NULL) {
			failfast ("oh dear im out of mem");
		}
		
		krad_compositor_port->local_frame->pixels = (int *)krad_compositor_port->local_buffer;
		
		krad_compositor_port->local_frame->cst =
			cairo_image_surface_create_for_data ((unsigned char *)krad_compositor_port->local_buffer,
												 CAIRO_FORMAT_ARGB32,
												 960,
												 540,
												 960 * 4);
	
		krad_compositor_port->local_frame->cr = cairo_create (krad_compositor_port->local_frame->cst);		
		
		
		
		krad_compositor_port->krad_compositor_subunit->active = 1;
		pthread_mutex_unlock (&krad_compositor->settings_lock);
	} else {
		printke ("Krad Compositor: failed to create local port");
		krad_compositor_port_destroy_unlocked (krad_compositor, krad_compositor_port);
		pthread_mutex_unlock (&krad_compositor->settings_lock);
		return NULL;
	}

	return krad_compositor_port;	
}

void krad_compositor_port_destroy_unlocked (krad_compositor_t *krad_compositor, krad_compositor_port_t *krad_compositor_port) {

	krad_compositor_port->krad_compositor_subunit->active = 3;
	
	//FIXME kludge to wait for nonuse
	
	usleep (45000);

	if (strstr(krad_compositor_port->sysname, "passthru") != NULL) {
		krad_compositor->active_passthru_ports--;
	} else {
		if (krad_compositor_port->direction == INPUT) {
			krad_compositor->active_input_ports--;
		}
		if (krad_compositor_port->direction == OUTPUT) {
			krad_compositor->active_output_ports--;
      close (krad_compositor_port->socketpair[0]);
      close (krad_compositor_port->socketpair[1]);			
		}
 	}
 	
 	if (krad_compositor_port->local == 1) {
		if (krad_compositor_port->msg_sd != 0) {
			close (krad_compositor_port->msg_sd);
			krad_compositor_port->msg_sd = 0;	
		}
		if (krad_compositor_port->shm_sd != 0) {
			close (krad_compositor_port->shm_sd);
			krad_compositor_port->shm_sd = 0;
		}
		if (krad_compositor_port->local_buffer != NULL) {
			munmap (krad_compositor_port->local_buffer, krad_compositor_port->local_buffer_size);
			krad_compositor_port->local_buffer_size = 0;
			krad_compositor_port->local_buffer = NULL;
		}
		
		if (krad_compositor_port->local_frame != NULL) {
			cairo_destroy (krad_compositor_port->local_frame->cr);
			cairo_surface_destroy (krad_compositor_port->local_frame->cst);
			free (krad_compositor_port->local_frame);
		}
		krad_compositor_port->local = 0;
 	}

	if (krad_compositor_port->frame_ring != NULL) {
		krad_ringbuffer_free ( krad_compositor_port->frame_ring );
		krad_compositor_port->frame_ring = NULL;
	}
	krad_compositor_port->start_timecode = 0;
	krad_compositor_port->krad_compositor_subunit->active = 0;

	if (krad_compositor_port->sws_converter != NULL) {
		sws_freeContext ( krad_compositor_port->sws_converter );
		krad_compositor_port->sws_converter = NULL;
	}

	if (krad_compositor_port->last_frame != NULL) {
		krad_framepool_unref_frame (krad_compositor_port->last_frame);
		krad_compositor_port->last_frame = NULL;
	}

	if (strstr(krad_compositor_port->sysname, "passthru") != NULL) { 
		krad_compositor_port->passthru = 0;
	} else {
		krad_compositor->active_ports--;
	}


}

void krad_compositor_port_destroy (krad_compositor_t *krad_compositor, krad_compositor_port_t *krad_compositor_port) {

	pthread_mutex_lock (&krad_compositor->settings_lock);
	
	krad_compositor_port_destroy_unlocked (krad_compositor, krad_compositor_port);
	
	pthread_mutex_unlock (&krad_compositor->settings_lock);
}

void krad_compositor_destroy (krad_compositor_t *krad_compositor) {

	int i;
	
  printk ("Krad Compositor: Destroy Started");

#ifdef KRAD_USE_WAYLAND
  if (krad_compositor->display_open == 1) {
    krad_compositor_close_display (krad_compositor);
  }
#endif

	for (i = 0; i < KC_MAX_PORTS; i++) {
		if (krad_compositor->port[i].krad_compositor_subunit->active == 1) {
			krad_compositor_port_destroy (krad_compositor, &krad_compositor->port[i]);
		}
	}
	
  krad_compositor_stop_ticker (krad_compositor);

	krad_compositor_free_resources (krad_compositor);
	
	pthread_mutex_destroy (&krad_compositor->settings_lock);
	pthread_mutex_destroy (&krad_compositor->last_snapshot_name_lock);
	
	free (krad_compositor->port);
	
  krad_sprite_reset (krad_compositor->background);
	free (krad_compositor->background);


  krad_sprite_destroy_arr (krad_compositor->krad_sprite, KC_MAX_SPRITES);
  krad_text_destroy_arr (krad_compositor->krad_text, KC_MAX_TEXTS);

	free (krad_compositor);
	
  printk ("Krad Compositor: Destroy Complete");

}

void krad_compositor_get_resolution (krad_compositor_t *krad_compositor, int *width, int *height) {

	pthread_mutex_lock (&krad_compositor->settings_lock);

	*width = krad_compositor->width;
	*height = krad_compositor->height;

	pthread_mutex_unlock (&krad_compositor->settings_lock);
}

void krad_compositor_get_frame_rate (krad_compositor_t *krad_compositor,
									 int *frame_rate_numerator, int *frame_rate_denominator) {

	pthread_mutex_lock (&krad_compositor->settings_lock);
	
	*frame_rate_numerator = krad_compositor->frame_rate_numerator;
	*frame_rate_denominator = krad_compositor->frame_rate_denominator;

	pthread_mutex_unlock (&krad_compositor->settings_lock);

}

void krad_compositor_update_resolution (krad_compositor_t *krad_compositor, int width, int height) {

	pthread_mutex_lock (&krad_compositor->settings_lock);
	if ((krad_compositor->active_input_ports == 0) && (krad_compositor->active_output_ports == 0)) {
		krad_compositor_set_resolution (krad_compositor, width, height);
		krad_compositor_free_resources (krad_compositor);
	}
	pthread_mutex_unlock (&krad_compositor->settings_lock);	
	
}

void krad_compositor_set_resolution (krad_compositor_t *krad_compositor, int width, int height) {

	krad_compositor->width = width;
	krad_compositor->height = height;

	krad_compositor->frame_byte_size = krad_compositor->width * krad_compositor->height * 4;

}


krad_compositor_t *krad_compositor_create (int width, int height,
										   int frame_rate_numerator, int frame_rate_denominator) {

	int i;

	krad_compositor_t *krad_compositor = calloc(1, sizeof(krad_compositor_t));

	krad_compositor_set_resolution (krad_compositor, width, height);

	krad_compositor->background = krad_sprite_create ();

	krad_compositor->port = calloc(KC_MAX_PORTS, sizeof(krad_compositor_port_t));
	
  for (i = 0; i < KC_MAX_PORTS; i++) {
    krad_compositor->port[i].krad_compositor_subunit = krad_compositor_subunit_create();
    krad_compositor->port[i].krad_compositor_subunit->address.path.unit = KR_COMPOSITOR;
    krad_compositor->port[i].krad_compositor_subunit->address.path.subunit.compositor_subunit = KR_VIDEOPORT;
    krad_compositor->port[i].krad_compositor_subunit->address.id.number = i;
  }
  
	krad_compositor->krad_sprite = krad_sprite_create_arr (KC_MAX_SPRITES);
	krad_compositor->krad_text = krad_text_create_arr (KC_MAX_TEXTS);	
	
	for (i = 0; i < KC_MAX_TEXTS; i++) {
		krad_text_reset (&krad_compositor->krad_text[i]);
	}
	
	pthread_mutex_init (&krad_compositor->settings_lock, NULL);
	pthread_mutex_init (&krad_compositor->last_snapshot_name_lock, NULL);
	
	krad_compositor_set_frame_rate (krad_compositor, frame_rate_numerator, frame_rate_denominator);

  krad_compositor->address.path.unit = KR_COMPOSITOR;
  krad_compositor->address.path.subunit.mixer_subunit = KR_UNIT;

	return krad_compositor;

}

void krad_compositor_ticker_thread_cleanup (void *arg) {

	krad_compositor_t *krad_compositor = (krad_compositor_t *)arg;
	
  if (krad_compositor->krad_ticker != NULL) {
	  krad_ticker_destroy (krad_compositor->krad_ticker);
    krad_compositor->krad_ticker = NULL;
    printk ("Krad Compositor: Synthetic Timer Destroyed");
  }
}

void *krad_compositor_ticker_thread (void *arg) {

	krad_compositor_t *krad_compositor = (krad_compositor_t *)arg;

	krad_system_set_thread_name ("kr_compositor");
  pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
	krad_compositor->krad_ticker = krad_ticker_create (krad_compositor->frame_rate_numerator,
													   krad_compositor->frame_rate_denominator);
  pthread_cleanup_push (krad_compositor_ticker_thread_cleanup, krad_compositor);
  pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	krad_ticker_start_at (krad_compositor->krad_ticker, krad_compositor->start_time);

	while (krad_compositor->ticker_running == 1) {
    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
		krad_compositor_process (krad_compositor);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
		krad_ticker_wait (krad_compositor->krad_ticker);
	}
	
	pthread_cleanup_pop (1);
	
	return NULL;
}

void krad_compositor_start_ticker (krad_compositor_t *krad_compositor) {

	if (krad_compositor->ticker_running == 1) {
		krad_compositor_stop_ticker (krad_compositor);
	}
  clock_gettime (CLOCK_MONOTONIC, &krad_compositor->start_time);
	krad_compositor->ticker_running = 1;
	pthread_create (&krad_compositor->ticker_thread, NULL, krad_compositor_ticker_thread, (void *)krad_compositor);

}

void krad_compositor_start_ticker_at (krad_compositor_t *krad_compositor, struct timespec start_time) {

	if (krad_compositor->ticker_running == 1) {
		krad_compositor_stop_ticker (krad_compositor);
	}
	memcpy (&krad_compositor->start_time, &start_time, sizeof(struct timespec));
	krad_compositor->ticker_running = 1;
	pthread_create (&krad_compositor->ticker_thread, NULL, krad_compositor_ticker_thread, (void *)krad_compositor);

}


void krad_compositor_stop_ticker (krad_compositor_t *krad_compositor) {

	if (krad_compositor->ticker_running == 1) {
		krad_compositor->ticker_running = 2;
    pthread_cancel (krad_compositor->ticker_thread);
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
		return 0;
	} else {
		return 1;
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
		krad_compositor->frame_num = 0;
		krad_compositor_start_ticker (krad_compositor);
	} else {
		krad_compositor->frame_num = 0;	
	}
	
}

void krad_compositor_set_dir (krad_compositor_t *krad_compositor, char *dir) {

	if (krad_compositor->dir != NULL) {
		free (krad_compositor->dir);
	}

	krad_compositor->dir = strdup (dir);

}

void krad_compositor_set_krad_mixer (krad_compositor_t *krad_compositor, krad_mixer_t *krad_mixer) {
	krad_compositor->krad_mixer = krad_mixer;
}

void krad_compositor_unset_krad_mixer (krad_compositor_t *krad_compositor) {
	krad_compositor->krad_mixer = NULL;
}
