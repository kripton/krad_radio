#include "krad_text.h"


krad_text_t *krad_text_create () {

	krad_text_t *krad_text;

	if ((krad_text = calloc (1, sizeof (krad_text_t))) == NULL) {
		failfast ("Krad Sprite mem alloc fail");
	}
	
	krad_text_reset (krad_text);
	
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
	
	strcpy (krad_text->font, KRAD_TEXT_DEFAULT_FONT);
	
	krad_text->width = 0;
	krad_text->height = 0;
	krad_text->x = 0;
	krad_text->y = 0;
	krad_text->tickrate = KRAD_TEXT_DEFAULT_TICKRATE;
	krad_text->tick = 0;
	krad_text->xscale = 20.0f;
	krad_text->yscale = 20.0f;
	krad_text->opacity = 1.0f;
	krad_text->rotation = 0.0f;
	
	krad_text->xscale = 20.0f;
	krad_text->yscale = 20.0f;
	
	krad_text->red = 0.255f;
	krad_text->green = 0.032;
	krad_text->blue = 0.032f;
	
	krad_text->new_red = krad_text->red;
	krad_text->new_green = krad_text->green;
	krad_text->new_blue = krad_text->blue;
	
	krad_text->new_x = krad_text->x;
	krad_text->new_y = krad_text->y;
	
	krad_text->new_xscale = krad_text->xscale;
	krad_text->new_yscale = krad_text->yscale;
	krad_text->new_opacity = krad_text->opacity;
	krad_text->new_rotation = krad_text->rotation;	
	
}

void krad_text_set_text (krad_text_t *krad_text, char *text) {

	strcpy (krad_text->text_actual, text);

	krad_text->new_opacity = krad_text->opacity;
	krad_text->opacity = 0.0f;

}

void krad_text_set_font (krad_text_t *krad_text, char *font) {
	strcpy (krad_text->font, font);
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

void krad_text_set_new_rgb (krad_text_t *krad_text, int red, int green, int blue) {

	krad_text->new_red = red * 0.001f;
	krad_text->new_green = green * 0.001f;
	krad_text->new_blue = blue * 0.001f;

}

void krad_text_set_rgb (krad_text_t *krad_text, int red, int green, int blue) {

	krad_text->red = red * 0.001f;
	krad_text->new_red = krad_text->red;
	
	krad_text->green = green * 0.001f;
	krad_text->new_green = krad_text->green;
	
	krad_text->blue = blue * 0.001f;
	krad_text->new_blue = krad_text->blue;
}

void krad_text_expand (krad_text_t *krad_text, cairo_t *cr, int width) {

	float scale;
	cairo_text_extents_t extents;

	cairo_select_font_face (cr, krad_text->font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, krad_text->xscale);
	cairo_set_source_rgba (cr,
						   krad_text->red / 0.255 * 1.0,
						   krad_text->green / 0.255 * 1.0,
						   krad_text->blue / 0.255 * 1.0,
						   krad_text->opacity);	

	cairo_text_extents (cr, krad_text->text_actual, &extents);

	scale = krad_text->xscale;
	
	while (extents.width < width) {
		scale += 1.0;
		cairo_set_font_size (cr, scale);
		cairo_text_extents (cr, krad_text->text_actual, &extents);
	}


	krad_text->xscale = scale;
	krad_text->yscale = scale;
	krad_text->new_xscale = scale;
	krad_text->new_yscale = scale;	
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
	
	if (krad_text->new_red != krad_text->red) {
		if (krad_text->new_red > krad_text->red) {
			if (krad_text->red + 0.003f >= krad_text->new_red) {
				krad_text->red = krad_text->new_red;
			} else {
				krad_text->red = krad_text->red + 0.003f;
			}
		} else {
			if (krad_text->red - 0.003f <= krad_text->new_red) {
				krad_text->red = krad_text->new_red;
			} else {
				krad_text->red = krad_text->red - 0.003f;
			}
		}
	}
	
	if (krad_text->new_green != krad_text->green) {
		if (krad_text->new_green > krad_text->green) {
			if (krad_text->green + 0.003f >= krad_text->new_green) {
				krad_text->green = krad_text->new_green;
			} else {
				krad_text->green = krad_text->green + 0.003f;
			}
		} else {
			if (krad_text->green - 0.003f <= krad_text->new_green) {
				krad_text->green = krad_text->new_green;
			} else {
				krad_text->green = krad_text->green - 0.003f;
			}
		}
	}
	
	if (krad_text->new_blue != krad_text->blue) {
		if (krad_text->new_blue > krad_text->blue) {
			if (krad_text->blue + 0.003f >= krad_text->new_blue) {
				krad_text->blue = krad_text->new_blue;
			} else {
				krad_text->blue = krad_text->blue + 0.003f;
			}
		} else {
			if (krad_text->blue - 0.003f <= krad_text->new_blue) {
				krad_text->blue = krad_text->new_blue;
			} else {
				krad_text->blue = krad_text->blue - 0.003f;
			}
		}
	}

	
	if (krad_text->new_rotation != krad_text->rotation) {
		if (krad_text->new_rotation > krad_text->rotation) {
			if (krad_text->rotation + 1.8f >= krad_text->new_rotation) {
				krad_text->rotation = krad_text->new_rotation;
			} else {
				krad_text->rotation = krad_text->rotation + 1.8f;
			}
		} else {
			if (krad_text->rotation - 1.8f <= krad_text->new_rotation) {
				krad_text->rotation = krad_text->new_rotation;
			} else {
				krad_text->rotation = krad_text->rotation - 1.8f;
			}
		}
	}
	
	if (krad_text->new_opacity != krad_text->opacity) {
		if (krad_text->new_opacity > krad_text->opacity) {
			if (krad_text->opacity + 0.02f >= krad_text->new_opacity) {
				krad_text->opacity = krad_text->new_opacity;
			} else {
				krad_text->opacity = krad_text->opacity + 0.02f;
			}
		} else {
			if (krad_text->opacity - 0.02f <= krad_text->new_opacity) {
				krad_text->opacity = krad_text->new_opacity;
			} else {
				krad_text->opacity = krad_text->opacity - 0.02f;
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

void krad_text_prepare (krad_text_t *krad_text, cairo_t *cr) {

	if (krad_text->rotation != 0.0f) {
		cairo_translate (cr, krad_text->x, krad_text->y);	
		cairo_translate (cr, krad_text->width / 2, krad_text->height / 2);
		cairo_rotate (cr, krad_text->rotation * (M_PI/180.0));
		cairo_translate (cr, krad_text->width / -2, krad_text->height / -2);		
		cairo_translate (cr, krad_text->x * -1, krad_text->y * -1);
	}	
	
	cairo_select_font_face (cr, krad_text->font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, krad_text->xscale);
	cairo_set_source_rgba (cr,
						   krad_text->red / 0.255 * 1.0,
						   krad_text->green / 0.255 * 1.0,
						   krad_text->blue / 0.255 * 1.0,
						   krad_text->opacity);
	
	cairo_move_to (cr, krad_text->x, krad_text->y);
	cairo_show_text (cr, krad_text->text_actual);

}

void krad_text_render (krad_text_t *krad_text, cairo_t *cr) {
	
	cairo_save (cr);
	
	krad_text_prepare (krad_text, cr);
	
	cairo_stroke (cr);
	
	cairo_restore (cr);
	
	krad_text_tick (krad_text);			

}
