#include "krad_text.h"


krad_text_t *krad_text_create () {

	krad_text_t *krad_text;

	if ((krad_text = calloc (1, sizeof (krad_text_t))) == NULL) {
		failfast ("Krad Sprite mem alloc fail");
	}
	
	krad_text->opacity = 1.0f;
	krad_text->xscale = 20.0f;
	krad_text->yscale = 20.0f;
	krad_text->rotation = 0.0f;	
	krad_text->tickrate = KRAD_TEXT_DEFAULT_TICKRATE;
	
	krad_text->new_xscale = krad_text->xscale;
	krad_text->new_yscale = krad_text->yscale;
	krad_text->new_opacity = krad_text->opacity;
	krad_text->new_rotation = krad_text->rotation;	
	
	return krad_text;

}


void krad_text_destroy (krad_text_t *krad_text) {
	krad_text_reset (krad_text);
	free (krad_text);

}

void krad_text_reset (krad_text_t *krad_text) {

	if (krad_text->text_pattern != NULL) {
		cairo_pattern_destroy ( krad_text->text_pattern );
		krad_text->text_pattern = NULL;
	}
	if (krad_text->text != NULL) {
		cairo_surface_destroy ( krad_text->text );
		krad_text->text = NULL;
	}
	krad_text->width = 0;
	krad_text->height = 0;
	krad_text->x = 0;
	krad_text->y = 0;
	krad_text->tickrate = 5;
	krad_text->tick = 0;
	krad_text->xscale = 20.0f;
	krad_text->yscale = 20.0f;
	krad_text->opacity = 1.0f;
	krad_text->rotation = 0.0f;
	
	krad_text->xscale = 20.0f;
	krad_text->yscale = 20.0f;
	krad_text->opacity = 1.0f;
	
	krad_text->new_x = krad_text->x;
	krad_text->new_y = krad_text->y;
	
	krad_text->new_xscale = krad_text->xscale;
	krad_text->new_yscale = krad_text->yscale;
	krad_text->new_opacity = krad_text->opacity;
	krad_text->new_rotation = krad_text->rotation;	
	
}

void krad_text_set_text (krad_text_t *krad_text, char *text) {

	strcpy (krad_text->text_actual, text);

}

void krad_text_set_xy (krad_text_t *krad_text, int x, int y) {

	krad_text->x = x;
	krad_text->y = y;
	krad_text->new_x = x;
	krad_text->new_y = y;
	krad_text->last_x = x;
	krad_text->last_y = y;
	
}


void krad_text_set_new_xy (krad_text_t *krad_text, int x, int y) {

	krad_text->last_x = krad_text->x;
	krad_text->last_y = krad_text->y;
	krad_text->new_x = x;
	krad_text->new_y = y;
}


void krad_text_set_scale (krad_text_t *krad_text, float scale) {
	krad_text->xscale = scale;
	krad_text->yscale = scale;
	krad_text->new_xscale = scale;
	krad_text->new_yscale = scale;	
}

void krad_text_set_xscale (krad_text_t *krad_text, float scale) {
	krad_text->xscale = scale;
	krad_text->new_xscale = scale;	
}

void krad_text_set_yscale (krad_text_t *krad_text, float scale) {
	krad_text->yscale = scale;
	krad_text->new_yscale = scale;
}

void krad_text_set_opacity (krad_text_t *krad_text, float opacity) {
	krad_text->opacity = opacity;
	krad_text->new_opacity = opacity;	
}

void krad_text_set_rotation (krad_text_t *krad_text, float rotation) {
	krad_text->rotation = rotation;
	krad_text->new_rotation = rotation;	
}


void krad_text_set_new_scale (krad_text_t *krad_text, float scale) {
	krad_text->new_xscale = scale;
	krad_text->new_yscale = scale;	
}

void krad_text_set_new_xscale (krad_text_t *krad_text, float scale) {
	krad_text->new_xscale = scale;
}

void krad_text_set_new_yscale (krad_text_t *krad_text, float scale) {
	krad_text->new_yscale = scale;	
}

void krad_text_set_new_opacity (krad_text_t *krad_text, float opacity) {
	krad_text->new_opacity = opacity;
}

void krad_text_set_new_rotation (krad_text_t *krad_text, float rotation) {
	krad_text->new_rotation = rotation;
}


void krad_text_set_tickrate (krad_text_t *krad_text, int tickrate) {
	krad_text->tickrate = tickrate;
}

void krad_text_render_xy (krad_text_t *krad_text, cairo_t *cr, int x, int y) {

	krad_text_set_xy (krad_text, x, y);
	krad_text_render (krad_text, cr);
}

void krad_text_move (krad_text_t *krad_text, int amount) {

	if (krad_text->new_x != krad_text->x) {
		if (krad_text->new_x > krad_text->x) {
			if (krad_text->x + amount >= krad_text->new_x) {
				krad_text->x = krad_text->new_x;
			} else {
				krad_text->x = krad_text->x + amount;
			}
		} else {
			if (krad_text->x - amount <= krad_text->new_x) {
				krad_text->x = krad_text->new_x;
			} else {
				krad_text->x = krad_text->x - amount;
			}
		}
	}
	
	if (krad_text->new_y != krad_text->y) {
		if (krad_text->new_y > krad_text->y) {
			if (krad_text->y + amount >= krad_text->new_y) {
				krad_text->y = krad_text->new_y;
			} else {
				krad_text->y = krad_text->y + amount;
			}
		} else {
			if (krad_text->y - amount <= krad_text->new_y) {
				krad_text->y = krad_text->new_y;
			} else {
				krad_text->y = krad_text->y - amount;
			}
		}
	}	

}

void krad_text_tick (krad_text_t *krad_text) {

	krad_text->tick++;

	krad_text_move (krad_text, krad_text->tickrate);

	if (krad_text->tick >= krad_text->tickrate) {
		krad_text->tick = 0;

	}
	
	
	if (krad_text->new_rotation != krad_text->rotation) {
		if (krad_text->new_rotation > krad_text->rotation) {
			if (krad_text->rotation + 1.0f >= krad_text->new_rotation) {
				krad_text->rotation = krad_text->new_rotation;
			} else {
				krad_text->rotation = krad_text->rotation + 1.0f;
			}
		} else {
			if (krad_text->rotation - 1.0f <= krad_text->new_rotation) {
				krad_text->rotation = krad_text->new_rotation;
			} else {
				krad_text->rotation = krad_text->rotation - 1.0f;
			}
		}
	}
	
	if (krad_text->new_opacity != krad_text->opacity) {
		if (krad_text->new_opacity > krad_text->opacity) {
			if (krad_text->opacity + 2.0f >= krad_text->new_opacity) {
				krad_text->opacity = krad_text->new_opacity;
			} else {
				krad_text->opacity = krad_text->opacity + 2.0f;
			}
		} else {
			if (krad_text->opacity - 2.0f <= krad_text->new_opacity) {
				krad_text->opacity = krad_text->new_opacity;
			} else {
				krad_text->opacity = krad_text->opacity - 2.0f;
			}
		}
	}
	
	if (krad_text->new_xscale != krad_text->xscale) {
		if (krad_text->new_xscale > krad_text->xscale) {
			if (krad_text->xscale + 2.0f >= krad_text->new_xscale) {
				krad_text->xscale = krad_text->new_xscale;
			} else {
				krad_text->xscale = krad_text->xscale + 2.0f;
			}
		} else {
			if (krad_text->xscale - 2.0f <= krad_text->new_xscale) {
				krad_text->xscale = krad_text->new_xscale;
			} else {
				krad_text->xscale = krad_text->xscale - 2.0f;
			}
		}
	}
	
	if (krad_text->new_yscale != krad_text->yscale) {
		if (krad_text->new_yscale > krad_text->yscale) {
			if (krad_text->yscale + 2.0f >= krad_text->new_yscale) {
				krad_text->yscale = krad_text->new_yscale;
			} else {
				krad_text->yscale = krad_text->yscale + 2.0f;
			}
		} else {
			if (krad_text->yscale - 2.0f <= krad_text->new_yscale) {
				krad_text->yscale = krad_text->new_yscale;
			} else {
				krad_text->yscale = krad_text->yscale - 2.0f;
			}
		}
	}	
	
}

void krad_text_render (krad_text_t *krad_text, cairo_t *cr) {
	
	cairo_save (cr);

	/*
	if ((krad_text->xscale != 1.0f) || (krad_text->yscale != 1.0f)) {
		cairo_translate (cr, krad_text->x, krad_text->y);
		cairo_translate (cr, ((krad_text->width / 2) * krad_text->xscale),
						((krad_text->height / 2) * krad_text->yscale));
		cairo_scale (cr, krad_text->xscale, krad_text->yscale);
		cairo_translate (cr, krad_text->width / -2, krad_text->height / -2);		
		cairo_translate (cr, krad_text->x * -1, krad_text->y * -1);		
	}

	cairo_set_source_surface (cr,
							  krad_text->text,
							  krad_text->x - krad_text->width,
							  krad_text->y - krad_text->height);

	cairo_rectangle (cr,
					 krad_text->x,
					 krad_text->y,
					 krad_text->width,
					 krad_text->height);

	cairo_clip (cr);

	if (krad_text->opacity == 1.0f) {
		cairo_paint ( cr );
	} else {
		cairo_paint_with_alpha ( cr, krad_text->opacity );
	}
	*/
	
	if (krad_text->rotation != 0.0f) {
		cairo_translate (cr, krad_text->x, krad_text->y);	
		cairo_translate (cr, krad_text->width / 2, krad_text->height / 2);
		cairo_rotate (cr, krad_text->rotation * (M_PI/180.0));
		cairo_translate (cr, krad_text->width / -2, krad_text->height / -2);		
		cairo_translate (cr, krad_text->x * -1, krad_text->y * -1);
	}	
	
	cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, krad_text->xscale);
	cairo_set_source_rgb (cr, ORANGE);
	
	cairo_move_to (cr, krad_text->x, krad_text->y);
	cairo_show_text (cr, krad_text->text_actual);
	
	cairo_stroke (cr);
	
	cairo_restore (cr);
	
	krad_text_tick (krad_text);
	
	//printk ("Tick: %d Frame: %d",
	//		krad_text->tick, krad_text->frame);
			
	
}
