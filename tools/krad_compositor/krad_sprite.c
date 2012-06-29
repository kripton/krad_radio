#include "krad_sprite.h"




krad_sprite_t *krad_sprite_create () {

	krad_sprite_t *krad_sprite;

	if ((krad_sprite = calloc (1, sizeof (krad_sprite_t))) == NULL) {
		failfast ("Krad Sprite mem alloc fail");
	}
	
	krad_sprite->opacity = 1.0f;
	krad_sprite->xscale = 1.0f;
	krad_sprite->yscale = 1.0f;	
	krad_sprite->tickrate = KRAD_SPRITE_DEFAULT_TICKRATE;	
	krad_sprite->frames = 1;
	
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
	
		krad_sprite->sprite = cairo_image_surface_create_from_png ( filename );
		
		if (cairo_surface_status (krad_sprite->sprite) != CAIRO_STATUS_SUCCESS) {
			krad_sprite->sprite = NULL;
			return;
		}
		
		krad_sprite->sheet_width = cairo_image_surface_get_width ( krad_sprite->sprite );
		krad_sprite->sheet_height = cairo_image_surface_get_height ( krad_sprite->sprite );
		//krad_sprite->width = krad_sprite->sheet_width / MIN(krad_sprite->frames, 10);
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
	krad_sprite->tickrate = 5;
	krad_sprite->tick = 0;
	krad_sprite->frame = 0;
	krad_sprite->frames = 1;
	krad_sprite->xscale = 1.0f;
	krad_sprite->yscale = 1.0f;
	krad_sprite->opacity = 1.0f;
}

void krad_sprite_destroy (krad_sprite_t *krad_sprite) {
	
	krad_sprite_reset (krad_sprite);
	free (krad_sprite);

}

void krad_sprite_set_xy (krad_sprite_t *krad_sprite, int x, int y) {

	krad_sprite->x = x;
	krad_sprite->y = y;
}


void krad_sprite_set_scale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->xscale = scale;
	krad_sprite->yscale = scale;	
}

void krad_sprite_set_xscale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->xscale = scale;
}

void krad_sprite_set_yscale (krad_sprite_t *krad_sprite, float scale) {
	krad_sprite->yscale = scale;	
}

void krad_sprite_set_opacity (krad_sprite_t *krad_sprite, float opacity) {
	krad_sprite->opacity = opacity;
}

void krad_sprite_set_rotation (krad_sprite_t *krad_sprite, float rotation) {
	krad_sprite->rotation = rotation;
}

void krad_sprite_set_tickrate (krad_sprite_t *krad_sprite, int tickrate) {
	krad_sprite->tickrate = tickrate;
}

void krad_sprite_render_xy (krad_sprite_t *krad_sprite, cairo_t *cr, int x, int y) {

	krad_sprite_set_xy (krad_sprite, x, y);
	krad_sprite_render (krad_sprite, cr);
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
	
	//printk ("Tick: %d Frame: %d",
	//		krad_sprite->tick, krad_sprite->frame);
			
	
}
