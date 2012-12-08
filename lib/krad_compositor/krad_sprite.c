#include "krad_sprite.h"

void krad_sprite_destroy (krad_sprite_t *krad_sprite) {
	
	krad_sprite_reset (krad_sprite);
	free (krad_sprite);

}

void krad_sprite_destroy_arr (krad_sprite_t *krad_sprite, int count) {
	
	int s;
	
  s = 0;
	
	for (s = 0; s < count; s++) {
	  krad_sprite_reset (&krad_sprite[s]);
	}

	free (krad_sprite);

}

krad_sprite_t *krad_sprite_create_arr (int count) {

  int s;
  krad_sprite_t *krad_sprite;

  s = 0;

  if ((krad_sprite = calloc (count, sizeof (krad_sprite_t))) == NULL) {
    failfast ("Krad Sprite mem alloc fail");
  }
  
  for (s = 0; s < count; s++) {
    krad_sprite[s].krad_compositor_subunit = krad_compositor_subunit_create();
    krad_sprite_reset (&krad_sprite[s]);
  }
  
  return krad_sprite;

}

krad_sprite_t *krad_sprite_create () {
  return krad_sprite_create_arr (1);
}


krad_sprite_t *krad_sprite_create_from_file (char *filename) {

	krad_sprite_t *krad_sprite;

	krad_sprite = krad_sprite_create ();
	
	krad_sprite_open_file (krad_sprite, filename);
	
	return krad_sprite;

}

int krad_sprite_open_file (krad_sprite_t *krad_sprite, char *filename) {

	if (krad_sprite->sprite != NULL) {
		krad_sprite_reset (krad_sprite);
	}
	
	if ( filename != NULL ) {

		if (strstr (filename, "_frames_") != NULL) {
			krad_sprite->frames = atoi (strstr (filename, "_frames_") + 8);
		}
	
		if ((strstr (filename, ".png") != NULL) || (strstr (filename, ".PNG") != NULL)) {
			krad_sprite->sprite = cairo_image_surface_create_from_png ( filename );
		} else {
			if ((strstr (filename, ".jpg") != NULL) || (strstr (filename, ".JPG") != NULL)) {
			
				tjhandle jpeg_dec;
				int jpegsubsamp;
				int jpeg_size;
				unsigned char *argb_buffer;
				unsigned char *jpeg_buffer;
				int jpeg_fd;
				int stride;
				
				
				jpeg_buffer = malloc (10000000);
				if (jpeg_buffer == NULL) {
					return 0;
				}
				
				jpeg_fd = open (filename, O_RDONLY);
				
				if (jpeg_fd < 1) {
					free (jpeg_buffer);
					return 0;
				}
				
				jpeg_size = read (jpeg_fd, jpeg_buffer, 10000000);				
				
				if (jpeg_size == 10000000) {
					free (jpeg_buffer);
					close (jpeg_fd);
					return 0;
				}
				
				jpeg_dec = tjInitDecompress ();
				
				if (tjDecompressHeader2 (jpeg_dec, jpeg_buffer, jpeg_size, &krad_sprite->sheet_width, &krad_sprite->sheet_height, &jpegsubsamp)) {
					printke ("JPEG header decoding error: %s", tjGetErrorStr());
				} else {
					krad_sprite->sprite = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, krad_sprite->sheet_width, krad_sprite->sheet_height);
					stride = cairo_image_surface_get_stride (krad_sprite->sprite);
					argb_buffer = cairo_image_surface_get_data (krad_sprite->sprite);
					cairo_surface_flush (krad_sprite->sprite);
					if (tjDecompress2 ( jpeg_dec, jpeg_buffer, jpeg_size,
										argb_buffer, krad_sprite->sheet_width, stride, krad_sprite->sheet_height, TJPF_BGRA, 0 )) {
						printke ("JPEG decoding error: %s", tjGetErrorStr());
						cairo_surface_destroy ( krad_sprite->sprite );
						krad_sprite->sprite = NULL;
					} else {
						tjDestroy ( jpeg_dec );
						cairo_surface_mark_dirty (krad_sprite->sprite);
					}
				}
				
				free (jpeg_buffer);
				close (jpeg_fd);
			} else {
				krad_sprite->sprite = NULL;
				return 0;
			}
		}
		if (cairo_surface_status (krad_sprite->sprite) != CAIRO_STATUS_SUCCESS) {
			krad_sprite->sprite = NULL;
			return 0;
		}
		
		krad_sprite->sheet_width = cairo_image_surface_get_width ( krad_sprite->sprite );
		krad_sprite->sheet_height = cairo_image_surface_get_height ( krad_sprite->sprite );
		if (krad_sprite->frames > 1) {
      if (krad_sprite->frames >= 10) {
			  krad_sprite->krad_compositor_subunit->width = krad_sprite->sheet_width / 10;
        krad_sprite->krad_compositor_subunit->height = krad_sprite->sheet_height / ((krad_sprite->frames / 10) + MIN (1, (krad_sprite->frames % 10)));			  
			} else {
			  krad_sprite->krad_compositor_subunit->width = krad_sprite->sheet_width / krad_sprite->frames;
			  krad_sprite->krad_compositor_subunit->height = krad_sprite->sheet_height;
			}
		} else {
			krad_sprite->krad_compositor_subunit->width = krad_sprite->sheet_width;
			krad_sprite->krad_compositor_subunit->height = krad_sprite->sheet_height;			
		}
		krad_sprite->sprite_pattern = cairo_pattern_create_for_surface (krad_sprite->sprite);
		cairo_pattern_set_extend (krad_sprite->sprite_pattern, CAIRO_EXTEND_REPEAT);
		
		printk ("Loaded Sprite: %s Sheet Width: %d Frames: %d Width: %d Height: %d",
				filename, krad_sprite->sheet_width, krad_sprite->frames,
				krad_sprite->krad_compositor_subunit->width, krad_sprite->krad_compositor_subunit->height);
		strcpy(krad_sprite->filename, filename);
		krad_sprite->krad_compositor_subunit->opacity = 0.0f;
		krad_compositor_subunit_set_new_opacity (krad_sprite->krad_compositor_subunit, 1.0f);

	}
	
	return 1;
	
}

void krad_sprite_reset (krad_sprite_t *krad_sprite) {

	if (krad_sprite->sprite_pattern != NULL) {
		cairo_pattern_destroy ( krad_sprite->sprite_pattern );
		krad_sprite->sprite_pattern = NULL;
	}
	if (krad_sprite->sprite != NULL) {
		cairo_surface_destroy ( krad_sprite->sprite );
		krad_sprite->sprite = NULL;
	}
  
  krad_sprite->frame = 0;
  krad_sprite->frames = 1;

  krad_compositor_subunit_reset(krad_sprite->krad_compositor_subunit);
	
}


void krad_sprite_render_xy (krad_sprite_t *krad_sprite, cairo_t *cr, int x, int y) {

	krad_compositor_subunit_set_xy (krad_sprite->krad_compositor_subunit, x, y);
	krad_sprite_render (krad_sprite, cr);
}


void krad_sprite_tick (krad_sprite_t *krad_sprite) {

	krad_sprite->krad_compositor_subunit->tick++;

	if (krad_sprite->krad_compositor_subunit->tick >= krad_sprite->krad_compositor_subunit->tickrate) {
		krad_sprite->krad_compositor_subunit->tick = 0;
		krad_sprite->frame++;
		if (krad_sprite->frame == krad_sprite->frames) {
			krad_sprite->frame = 0;
		}
	}
	
	krad_compositor_subunit_update (krad_sprite->krad_compositor_subunit);	
}

void krad_sprite_render (krad_sprite_t *krad_sprite, cairo_t *cr) {
	
	cairo_save (cr);

	if ((krad_sprite->krad_compositor_subunit->xscale != 1.0f) || (krad_sprite->krad_compositor_subunit->yscale != 1.0f)) {
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->x, krad_sprite->krad_compositor_subunit->y);
		cairo_translate (cr, ((krad_sprite->krad_compositor_subunit->width / 2) * krad_sprite->krad_compositor_subunit->xscale),
						((krad_sprite->krad_compositor_subunit->height / 2) * krad_sprite->krad_compositor_subunit->yscale));
		cairo_scale (cr, krad_sprite->krad_compositor_subunit->xscale, krad_sprite->krad_compositor_subunit->yscale);
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->width / -2, krad_sprite->krad_compositor_subunit->height / -2);		
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->x * -1, krad_sprite->krad_compositor_subunit->y * -1);		
	}
	
	if (krad_sprite->krad_compositor_subunit->rotation != 0.0f) {
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->x, krad_sprite->krad_compositor_subunit->y);	
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->width / 2, krad_sprite->krad_compositor_subunit->height / 2);
		cairo_rotate (cr, krad_sprite->krad_compositor_subunit->rotation * (M_PI/180.0));
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->width / -2, krad_sprite->krad_compositor_subunit->height / -2);		
		cairo_translate (cr, krad_sprite->krad_compositor_subunit->x * -1, krad_sprite->krad_compositor_subunit->y * -1);
	}

	cairo_set_source_surface (cr,
							  krad_sprite->sprite,
							  krad_sprite->krad_compositor_subunit->x - (krad_sprite->krad_compositor_subunit->width * (krad_sprite->frame % 10)),
							  krad_sprite->krad_compositor_subunit->y - (krad_sprite->krad_compositor_subunit->height * (krad_sprite->frame / 10)));

	cairo_rectangle (cr,
					 krad_sprite->krad_compositor_subunit->x,
					 krad_sprite->krad_compositor_subunit->y,
					 krad_sprite->krad_compositor_subunit->width,
					 krad_sprite->krad_compositor_subunit->height);

	cairo_clip (cr);

	if (krad_sprite->krad_compositor_subunit->opacity == 1.0f) {
		cairo_paint ( cr );
	} else {
		cairo_paint_with_alpha ( cr, krad_sprite->krad_compositor_subunit->opacity );
	}
	
	cairo_restore (cr);
	
	krad_sprite_tick (krad_sprite);
	
}


krad_sprite_rep_t *krad_sprite_to_sprite_rep (krad_sprite_t *krad_sprite) {
  
  krad_sprite_rep_t *krad_sprite_rep = krad_compositor_sprite_rep_create();
  
  strcpy (krad_sprite_rep->filename, krad_sprite->filename);
  
  krad_sprite_rep->controls->x = krad_sprite->krad_compositor_subunit->x;
  krad_sprite_rep->controls->y = krad_sprite->krad_compositor_subunit->y;
  krad_sprite_rep->controls->z = krad_sprite->krad_compositor_subunit->z;
  
  krad_sprite_rep->controls->tickrate = krad_sprite->krad_compositor_subunit->tickrate;

  krad_sprite_rep->controls->width = krad_sprite->krad_compositor_subunit->width;
  krad_sprite_rep->controls->height = krad_sprite->krad_compositor_subunit->height;
    
  krad_sprite_rep->controls->xscale = krad_sprite->krad_compositor_subunit->xscale;
  krad_sprite_rep->controls->yscale = krad_sprite->krad_compositor_subunit->yscale;
    
  krad_sprite_rep->controls->rotation = krad_sprite->krad_compositor_subunit->rotation;
  krad_sprite_rep->controls->opacity = krad_sprite->krad_compositor_subunit->opacity;
   
  return krad_sprite_rep;
}
