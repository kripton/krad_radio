#include "krad_sprite.h"




krad_sprite_t *krad_sprite_create () {

	krad_sprite_t *krad_sprite;

	if ((krad_sprite = calloc (1, sizeof (krad_sprite_t))) == NULL) {
		failfast ("Krad Sprite mem alloc fail");
	}
	
	krad_sprite_reset (krad_sprite);
	
	return krad_sprite;

}

krad_sprite_t *krad_sprite_create_from_file (char *filename) {

	krad_sprite_t *krad_sprite;

	krad_sprite = krad_sprite_create ();
	
	krad_sprite_open_file (krad_sprite, filename);
	
	return krad_sprite;

}

void krad_sprite_open_file (krad_sprite_t *krad_sprite, char *filename) {

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
					return;
				}
				
				jpeg_fd = open (filename, O_RDONLY);
				
				if (jpeg_fd < 1) {
					free (jpeg_buffer);
					return;
				}
				
				jpeg_size = read (jpeg_fd, jpeg_buffer, 10000000);				
				
				if (jpeg_size == 10000000) {
					free (jpeg_buffer);
					close (jpeg_fd);
					return;
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
				return;
			}
		}
		if (cairo_surface_status (krad_sprite->sprite) != CAIRO_STATUS_SUCCESS) {
			krad_sprite->sprite = NULL;
			return;
		}
		
		krad_sprite->sheet_width = cairo_image_surface_get_width ( krad_sprite->sprite );
		krad_sprite->sheet_height = cairo_image_surface_get_height ( krad_sprite->sprite );
		if (krad_sprite->frames > 1) {
			krad_sprite->width = krad_sprite->sheet_width / 10;
			krad_sprite->height = krad_sprite->sheet_height / ((krad_sprite->frames / 10) + MIN (1, (krad_sprite->frames % 10)));
		} else {
			krad_sprite->width = krad_sprite->sheet_width;
			krad_sprite->height = krad_sprite->sheet_height;			
		}
		krad_sprite->sprite_pattern = cairo_pattern_create_for_surface (krad_sprite->sprite);
		cairo_pattern_set_extend (krad_sprite->sprite_pattern, CAIRO_EXTEND_REPEAT);
		
		printk ("Loaded Sprite: %s Sheet Width: %d Frames: %d Width: %d Height: %d",
				filename, krad_sprite->sheet_width, krad_sprite->frames,
				krad_sprite->width, krad_sprite->height);
		
		krad_sprite->opacity = 0.0f;
		krad_sprite_set_new_opacity (krad_sprite, 1.0f);

	}
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
	krad_sprite->width = 0;
	krad_sprite->height = 0;
	krad_sprite->x = 0;
	krad_sprite->y = 0;
	krad_sprite->tickrate = KRAD_SPRITE_DEFAULT_TICKRATE;
	krad_sprite->tick = 0;
	krad_sprite->frame = 0;
	krad_sprite->frames = 1;
	krad_sprite->xscale = 1.0f;
	krad_sprite->yscale = 1.0f;
	krad_sprite->opacity = 1.0f;
	krad_sprite->rotation = 0.0f;
	
	
	krad_sprite->new_x = krad_sprite->x;
	krad_sprite->new_y = krad_sprite->y;
	
	krad_sprite->new_xscale = krad_sprite->xscale;
	krad_sprite->new_yscale = krad_sprite->yscale;
	krad_sprite->new_opacity = krad_sprite->opacity;
	krad_sprite->new_rotation = krad_sprite->rotation;

	krad_sprite->start_x = krad_sprite->x;
	krad_sprite->start_y = krad_sprite->y;
	krad_sprite->change_x_amount = 0;
	krad_sprite->change_y_amount = 0;	
	krad_sprite->x_time = 0;
	krad_sprite->y_time = 0;
	
	krad_sprite->x_duration = 0;
	krad_sprite->y_duration = 0;

	krad_sprite->start_rotation = krad_sprite->rotation;
	krad_sprite->start_opacity = krad_sprite->opacity;
	krad_sprite->start_xscale = krad_sprite->xscale;
	krad_sprite->start_yscale = krad_sprite->yscale;	
	
	krad_sprite->rotation_change_amount = 0;
	krad_sprite->opacity_change_amount = 0;
	krad_sprite->xscale_change_amount = 0;
	krad_sprite->yscale_change_amount = 0;
	
	krad_sprite->rotation_time = 0;
	krad_sprite->opacity_time = 0;
	krad_sprite->xscale_time = 0;
	krad_sprite->yscale_time = 0;
	
	krad_sprite->rotation_duration = 0;
	krad_sprite->opacity_duration = 0;
	krad_sprite->xscale_duration = 0;
	krad_sprite->yscale_duration = 0;	
	
	krad_sprite->krad_ease_x = krad_ease_random();
	krad_sprite->krad_ease_y = krad_ease_random();
	krad_sprite->krad_ease_xscale = krad_ease_random();
	krad_sprite->krad_ease_yscale = krad_ease_random();
	krad_sprite->krad_ease_rotation = krad_ease_random();
	krad_sprite->krad_ease_opacity = krad_ease_random();
	
}

void krad_sprite_destroy (krad_sprite_t *krad_sprite) {
	
	krad_sprite_reset (krad_sprite);
	free (krad_sprite);

}

void krad_sprite_set_xy (krad_sprite_t *krad_sprite, int x, int y) {

	krad_sprite->x = x;
	krad_sprite->y = y;
	
	krad_sprite->new_x = krad_sprite->x;
	krad_sprite->new_y = krad_sprite->y;
	
}

void krad_sprite_set_scale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->xscale = scale;
	krad_sprite->yscale = scale;
	krad_sprite->new_xscale = krad_sprite->xscale;
	krad_sprite->new_yscale = krad_sprite->yscale;
}

void krad_sprite_set_xscale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->xscale = scale;
	krad_sprite->new_xscale = krad_sprite->xscale;
}

void krad_sprite_set_yscale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->yscale = scale;
	krad_sprite->new_yscale = krad_sprite->yscale;
}

void krad_sprite_set_opacity (krad_sprite_t *krad_sprite, float opacity) {
	krad_sprite->opacity = opacity;
	krad_sprite->new_opacity = krad_sprite->opacity;
}

void krad_sprite_set_rotation (krad_sprite_t *krad_sprite, float rotation) {
	krad_sprite->rotation = rotation;
	krad_sprite->new_rotation = krad_sprite->rotation;
}

void krad_sprite_set_tickrate (krad_sprite_t *krad_sprite, int tickrate) {
	krad_sprite->tickrate = tickrate;
}

void krad_sprite_set_new_xy (krad_sprite_t *krad_sprite, int x, int y) {
	krad_sprite->x_duration = rand() % 100 + 10;
	krad_sprite->x_time = 0;
	krad_sprite->y_duration = rand() % 100 + 10;
	krad_sprite->y_time = 0;
	krad_sprite->start_x = krad_sprite->x;
	krad_sprite->start_y = krad_sprite->y;
	krad_sprite->change_x_amount = x - krad_sprite->start_x;
	krad_sprite->change_y_amount = y - krad_sprite->start_y;
	krad_sprite->krad_ease_x = krad_ease_random();
	krad_sprite->krad_ease_y = krad_ease_random();		
	krad_sprite->new_x = x;
	krad_sprite->new_y = y;
}

void krad_sprite_set_new_scale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->xscale_duration = rand() % 100 + 10;
	krad_sprite->xscale_time = 0;
	krad_sprite->yscale_duration = rand() % 100 + 10;
	krad_sprite->yscale_time = 0;	
	krad_sprite->start_xscale = krad_sprite->xscale;
	krad_sprite->start_yscale = krad_sprite->yscale;
	krad_sprite->xscale_change_amount = scale - krad_sprite->start_xscale;
	krad_sprite->yscale_change_amount = scale - krad_sprite->start_yscale;
	krad_sprite->krad_ease_xscale = krad_ease_random();
	krad_sprite->krad_ease_yscale = krad_ease_random();		
	krad_sprite->new_xscale = scale;
	krad_sprite->new_yscale = scale;	
}

void krad_sprite_set_new_xscale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->xscale_duration = rand() % 100 + 10;
	krad_sprite->xscale_time = 0;
	krad_sprite->start_xscale = krad_sprite->xscale;
	krad_sprite->xscale_change_amount = scale - krad_sprite->start_xscale;
	krad_sprite->krad_ease_xscale = krad_ease_random();		
	krad_sprite->new_xscale = scale;
}

void krad_sprite_set_new_yscale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->yscale_duration = rand() % 100 + 10;
	krad_sprite->yscale_time = 0;
	krad_sprite->start_yscale = krad_sprite->yscale;
	krad_sprite->yscale_change_amount = scale - krad_sprite->start_yscale;
	krad_sprite->krad_ease_yscale = krad_ease_random();	
	krad_sprite->new_yscale = scale;	
}

void krad_sprite_set_new_opacity (krad_sprite_t *krad_sprite, float opacity) {
	krad_sprite->opacity_duration = rand() % 100 + 10;
	krad_sprite->opacity_time = 0;
	krad_sprite->start_opacity = krad_sprite->opacity;
	krad_sprite->opacity_change_amount = opacity - krad_sprite->start_opacity;
	krad_sprite->krad_ease_opacity = krad_ease_random();	
	krad_sprite->new_opacity = opacity;
}

void krad_sprite_set_new_rotation (krad_sprite_t *krad_sprite, float rotation) {
	krad_sprite->rotation_duration = rand() % 100 + 10;
	krad_sprite->rotation_time = 0;
	krad_sprite->start_rotation = krad_sprite->rotation;
	krad_sprite->rotation_change_amount = rotation - krad_sprite->start_rotation;
	krad_sprite->krad_ease_rotation = krad_ease_random();
	krad_sprite->new_rotation = rotation;
}

void krad_sprite_render_xy (krad_sprite_t *krad_sprite, cairo_t *cr, int x, int y) {

	krad_sprite_set_xy (krad_sprite, x, y);
	krad_sprite_render (krad_sprite, cr);
}


void krad_sprite_update (krad_sprite_t *krad_sprite) {

	if (krad_sprite->x_time != krad_sprite->x_duration) {
		krad_sprite->x = krad_ease (krad_sprite->krad_ease_x, krad_sprite->x_time++, krad_sprite->start_x, krad_sprite->change_x_amount, krad_sprite->x_duration);
	}	
	
	if (krad_sprite->y_time != krad_sprite->y_duration) {
		krad_sprite->y = krad_ease (krad_sprite->krad_ease_y, krad_sprite->y_time++, krad_sprite->start_y, krad_sprite->change_y_amount, krad_sprite->y_duration);
	}
	
	if (krad_sprite->rotation_time != krad_sprite->rotation_duration) {
		krad_sprite->rotation = krad_ease (krad_sprite->krad_ease_rotation, krad_sprite->rotation_time++, krad_sprite->start_rotation, krad_sprite->rotation_change_amount, krad_sprite->rotation_duration);
	}
	
	if (krad_sprite->opacity_time != krad_sprite->opacity_duration) {
		krad_sprite->opacity = krad_ease (krad_sprite->krad_ease_opacity, krad_sprite->opacity_time++, krad_sprite->start_opacity, krad_sprite->opacity_change_amount, krad_sprite->opacity_duration);
	}
	
	if (krad_sprite->xscale_time != krad_sprite->xscale_duration) {
		krad_sprite->xscale = krad_ease (krad_sprite->krad_ease_xscale, krad_sprite->xscale_time++, krad_sprite->start_xscale, krad_sprite->xscale_change_amount, krad_sprite->xscale_duration);
	}
	
	if (krad_sprite->yscale_time != krad_sprite->yscale_duration) {
		krad_sprite->yscale = krad_ease (krad_sprite->krad_ease_yscale, krad_sprite->yscale_time++, krad_sprite->start_yscale, krad_sprite->yscale_change_amount, krad_sprite->yscale_duration);
	}
	
}

void krad_sprite_tick (krad_sprite_t *krad_sprite) {

	krad_sprite->tick++;

	if (krad_sprite->tick >= krad_sprite->tickrate) {
		krad_sprite->tick = 0;
		krad_sprite->frame++;
		if (krad_sprite->frame == krad_sprite->frames) {
			krad_sprite->frame = 0;
		}
	}
	
	krad_sprite_update (krad_sprite);	
}

void krad_sprite_render (krad_sprite_t *krad_sprite, cairo_t *cr) {
	
	cairo_save (cr);

	if ((krad_sprite->xscale != 1.0f) || (krad_sprite->yscale != 1.0f)) {
		cairo_translate (cr, krad_sprite->x, krad_sprite->y);
		cairo_translate (cr, ((krad_sprite->width / 2) * krad_sprite->xscale),
						((krad_sprite->height / 2) * krad_sprite->yscale));
		cairo_scale (cr, krad_sprite->xscale, krad_sprite->yscale);
		cairo_translate (cr, krad_sprite->width / -2, krad_sprite->height / -2);		
		cairo_translate (cr, krad_sprite->x * -1, krad_sprite->y * -1);		
	}
	
	if (krad_sprite->rotation != 0.0f) {
		cairo_translate (cr, krad_sprite->x, krad_sprite->y);	
		cairo_translate (cr, krad_sprite->width / 2, krad_sprite->height / 2);
		cairo_rotate (cr, krad_sprite->rotation * (M_PI/180.0));
		cairo_translate (cr, krad_sprite->width / -2, krad_sprite->height / -2);		
		cairo_translate (cr, krad_sprite->x * -1, krad_sprite->y * -1);
	}

	cairo_set_source_surface (cr,
							  krad_sprite->sprite,
							  krad_sprite->x - (krad_sprite->width * (krad_sprite->frame % 10)),
							  krad_sprite->y - (krad_sprite->height * (krad_sprite->frame / 10)));

	cairo_rectangle (cr,
					 krad_sprite->x,
					 krad_sprite->y,
					 krad_sprite->width,
					 krad_sprite->height);

	cairo_clip (cr);

	if (krad_sprite->opacity == 1.0f) {
		cairo_paint ( cr );
	} else {
		cairo_paint_with_alpha ( cr, krad_sprite->opacity );
	}
	
	cairo_restore (cr);
	
	krad_sprite_tick (krad_sprite);
	
}
