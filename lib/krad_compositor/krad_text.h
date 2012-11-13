

#include "krad_radio.h"

#ifndef KRAD_TEXT_H
#define KRAD_TEXT_H

#define KRAD_TEXT_DEFAULT_TICKRATE 4
#define KRAD_TEXT_DEFAULT_FONT "sans"

typedef struct krad_text_St krad_text_t;

struct krad_text_St {

	int active;

	int x;
	int y;
	int z;
	
	int new_x;
	int new_y;
	int last_x;
	int last_y;
	
	//float alpha;
	
	int tickrate;
	int tick;

	char font[128];
	char text_actual[1024];

	cairo_surface_t *text;
	cairo_pattern_t *text_pattern;

	int width;
	int height;

	float rotation;
	float opacity;
	float xscale;
	float yscale;
	
	float new_rotation;
	float new_opacity;
	float new_xscale;
	float new_yscale;
		
	float red;
	float blue;
	float green;
		
	float new_red;
	float new_blue;
	float new_green;	
		
};


krad_text_t *krad_text_create ();
void krad_text_destroy (krad_text_t *krad_text);
void krad_text_reset (krad_text_t *krad_text);
void krad_text_set_xy (krad_text_t *krad_text, int x, int y);
void krad_text_set_new_xy (krad_text_t *krad_text, int x, int y);

void krad_text_set_text (krad_text_t *krad_text, char *text);
void krad_text_set_scale (krad_text_t *krad_text, float scale);
void krad_text_set_xscale (krad_text_t *krad_text, float scale);
void krad_text_set_yscale (krad_text_t *krad_text, float scale);
void krad_text_set_opacity (krad_text_t *krad_text, float opacity);
void krad_text_set_rotation (krad_text_t *krad_text, float rotation);


void krad_text_expand (krad_text_t *krad_text, cairo_t *cr, int width);

void krad_text_set_font (krad_text_t *krad_text, char *font);

void krad_text_set_new_scale (krad_text_t *krad_text, float scale);
void krad_text_set_new_opacity (krad_text_t *krad_text, float opacity);
void krad_text_set_new_rotation (krad_text_t *krad_text, float rotation);
void krad_text_set_rgb (krad_text_t *krad_text, int red, int green, int blue);
void krad_text_set_new_rgb (krad_text_t *krad_text, int red, int green, int blue);

void krad_text_set_tickrate (krad_text_t *krad_text, int tickrate);
void krad_text_render (krad_text_t *krad_text, cairo_t *cr);
void krad_text_tick (krad_text_t *krad_text);
void krad_text_render_xy (krad_text_t *krad_text, cairo_t *cr, int x, int y);


#endif
