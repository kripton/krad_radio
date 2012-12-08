#include "krad_text.h"


void krad_text_destroy (krad_text_t *krad_text) {
	
	krad_text_reset (krad_text);
	
  pango_font_description_free (krad_text->font_description);	
	
	free (krad_text);

}

void krad_text_destroy_arr (krad_text_t *krad_text, int count) {
	
	int s;
	
	s = 0;
	
	for (s = 0; s < count; s++) {
	  krad_text_reset (&krad_text[s]);
	}

	free (krad_text);

}


krad_text_t *krad_text_create_arr (int count) {

  int s;
	krad_text_t *krad_text;

  s = 0;

	if ((krad_text = calloc (count, sizeof (krad_text_t))) == NULL) {
		failfast ("Krad Sprite mem alloc fail");
	}
	
  krad_text->font_description = pango_font_description_new ();	

  for (s = 0; s < count; s++) {
    krad_text[s].krad_compositor_subunit = krad_compositor_subunit_create();
    krad_text_reset (&krad_text[s]);
	}
	
	return krad_text;

}

krad_text_t *krad_text_create () {
	return krad_text_create_arr (1);
}

void krad_text_reset (krad_text_t *krad_text) {
	
	strcpy (krad_text->font, KRAD_TEXT_DEFAULT_FONT);
  
  krad_text->red = 0.255f;
	krad_text->green = 0.032;
	krad_text->blue = 0.032f;
  
  krad_text->new_red = krad_text->red;
	krad_text->new_green = krad_text->green;
	krad_text->new_blue = krad_text->blue;
  
  krad_compositor_subunit_reset (krad_text->krad_compositor_subunit);
	
	
  krad_compositor_subunit_set_tickrate(krad_text->krad_compositor_subunit, KRAD_TEXT_DEFAULT_TICKRATE);
  krad_compositor_subunit_set_xscale (krad_text->krad_compositor_subunit, 20.0f);
	krad_compositor_subunit_set_yscale (krad_text->krad_compositor_subunit, 20.0f);
	
	krad_text->krad_compositor_subunit->xscale = 20.0f;
	krad_text->krad_compositor_subunit->yscale = 20.0f;
	
}

void krad_text_set_text (krad_text_t *krad_text, char *text) {

	strcpy (krad_text->text_actual, text);
  

  krad_compositor_subunit_set_opacity(krad_text->krad_compositor_subunit, 0.0f);
  krad_compositor_subunit_set_new_opacity(krad_text->krad_compositor_subunit, krad_text->krad_compositor_subunit->opacity);

}

void krad_text_set_font (krad_text_t *krad_text, char *font) {
	strcpy (krad_text->font, font);
}

void krad_text_set_new_rgb (krad_text_t *krad_text, float red, float green, float blue) {

	krad_text->new_red = red * 0.001f;
	krad_text->new_green = green * 0.001f;
	krad_text->new_blue = blue * 0.001f;

}

void krad_text_set_rgb (krad_text_t *krad_text, float red, float green, float blue) {

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
	cairo_set_font_size (cr, krad_text->krad_compositor_subunit->xscale);
	cairo_set_source_rgba (cr,
						   krad_text->red / 0.255 * 1.0,
						   krad_text->green / 0.255 * 1.0,
						   krad_text->blue / 0.255 * 1.0,
						   krad_text->krad_compositor_subunit->opacity);	

	cairo_text_extents (cr, krad_text->text_actual, &extents);

	scale = krad_text->krad_compositor_subunit->xscale;
	
	while (extents.width < width) {
		scale += 1.0;
		cairo_set_font_size (cr, scale);
		cairo_text_extents (cr, krad_text->text_actual, &extents);
	}

  krad_compositor_subunit_set_xscale(krad_text->krad_compositor_subunit, scale);
  krad_compositor_subunit_set_yscale(krad_text->krad_compositor_subunit, scale);
  
}






void krad_text_render_xy (krad_text_t *krad_text, cairo_t *cr, int x, int y) {

	krad_compositor_subunit_set_xy (krad_text->krad_compositor_subunit, x, y);
	krad_text_render (krad_text, cr);
}


void krad_text_tick (krad_text_t *krad_text) {

	krad_text->krad_compositor_subunit->tick++;

	if (krad_text->krad_compositor_subunit->tick >= krad_text->krad_compositor_subunit->tickrate) {
		krad_text->krad_compositor_subunit->tick = 0;
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

  krad_compositor_subunit_update (krad_text->krad_compositor_subunit);
	
}

void krad_text_prepare (krad_text_t *krad_text, cairo_t *cr) {

	if (krad_text->krad_compositor_subunit->rotation != 0.0f) {
		cairo_translate (cr, krad_text->krad_compositor_subunit->x, krad_text->krad_compositor_subunit->y);	
		cairo_translate (cr, krad_text->krad_compositor_subunit->width / 2, krad_text->krad_compositor_subunit->height / 2);
		cairo_rotate (cr, krad_text->krad_compositor_subunit->rotation * (M_PI/180.0));
		cairo_translate (cr, krad_text->krad_compositor_subunit->width / -2, krad_text->krad_compositor_subunit->height / -2);		
		cairo_translate (cr, krad_text->krad_compositor_subunit->x * -1, krad_text->krad_compositor_subunit->y * -1);
	}	
	
	/*
	cairo_select_font_face (cr, krad_text->font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, krad_text->krad_compositor_subunit->xscale);
	cairo_set_source_rgba (cr,
						   krad_text->red / 0.255 * 1.0,
						   krad_text->green / 0.255 * 1.0,
						   krad_text->blue / 0.255 * 1.0,
						   krad_text->krad_compositor_subunit->opacity);
	
	cairo_move_to (cr, krad_text->krad_compositor_subunit->x, krad_text->krad_compositor_subunit->y);
	cairo_show_text (cr, krad_text->text_actual);
  */
  
  
  pango_font_description_set_family (krad_text->font_description, krad_text->font);
  pango_font_description_set_weight (krad_text->font_description, PANGO_WEIGHT_BOLD);
  pango_font_description_set_absolute_size (krad_text->font_description, krad_text->krad_compositor_subunit->xscale * PANGO_SCALE);

  krad_text->layout = pango_cairo_create_layout (cr);

  pango_layout_set_font_description (krad_text->layout, krad_text->font_description);
  pango_layout_set_text (krad_text->layout, krad_text->text_actual, -1);

	cairo_set_source_rgba (cr,
						   krad_text->red / 0.255 * 1.0,
						   krad_text->green / 0.255 * 1.0,
						   krad_text->blue / 0.255 * 1.0,
						   krad_text->krad_compositor_subunit->opacity);
	
	cairo_move_to (cr, krad_text->krad_compositor_subunit->x, krad_text->krad_compositor_subunit->y);
  pango_cairo_show_layout (cr, krad_text->layout);

  g_object_unref (krad_text->layout);

}

void krad_text_render (krad_text_t *krad_text, cairo_t *cr) {
	
	cairo_save (cr);
	
	krad_text_prepare (krad_text, cr);
	
	cairo_stroke (cr);
	
	cairo_restore (cr);
	
	krad_text_tick (krad_text);			

}

krad_text_rep_t *krad_text_to_text_rep (krad_text_t *krad_text, krad_text_rep_t *krad_text_rep) {
  
  krad_text_rep_t *krad_text_rep_ret;
  
  if (krad_text_rep == NULL) {
    krad_text_rep_ret = krad_compositor_text_rep_create();
  } else {
    krad_text_rep_ret = krad_text_rep;
  }
  
  strncpy (krad_text_rep_ret->text, krad_text->text_actual, 1023);
  strncpy (krad_text_rep_ret->font, krad_text->font, 127);
  
  krad_text_rep_ret->red = krad_text->red;
  krad_text_rep_ret->green = krad_text->green;
  krad_text_rep_ret->blue = krad_text->blue;
  
  krad_text_rep_ret->controls->x = krad_text->krad_compositor_subunit->x;
  krad_text_rep_ret->controls->y = krad_text->krad_compositor_subunit->y;
  krad_text_rep_ret->controls->z = krad_text->krad_compositor_subunit->z;
  
  krad_text_rep_ret->controls->tickrate = krad_text->krad_compositor_subunit->tickrate;

  krad_text_rep_ret->controls->width = krad_text->krad_compositor_subunit->width;
  krad_text_rep_ret->controls->height = krad_text->krad_compositor_subunit->height;
    
  krad_text_rep_ret->controls->xscale = krad_text->krad_compositor_subunit->xscale;
  krad_text_rep_ret->controls->yscale = krad_text->krad_compositor_subunit->yscale;
    
  krad_text_rep_ret->controls->rotation = krad_text->krad_compositor_subunit->rotation;
  krad_text_rep_ret->controls->opacity = krad_text->krad_compositor_subunit->opacity;
   
  return krad_text_rep_ret;
}
