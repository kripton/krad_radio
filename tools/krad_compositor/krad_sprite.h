#ifndef KRAD_SPRITE_H
#define KRAD_SPRITE_H

#include <turbojpeg.h>

typedef struct krad_sprite_St krad_sprite_t;

#include "krad_radio.h"

#define KRAD_SPRITE_DEFAULT_TICKRATE 4


struct krad_sprite_St {

	int active;

	int x;
	int y;
	int z;	
	
	int tickrate;
	int tick;
	int frames;
	int frame;

	cairo_surface_t *sprite;
	cairo_pattern_t *sprite_pattern;

	int sheet_width;
	int sheet_height;
	int width;
	int height;

	float rotation;
	float opacity;
	float xscale;
	float yscale;
	
	int new_x;
	int new_y;
	int start_x;
	int start_y;
	int change_x_amount;
	int change_y_amount;	
	int x_time;
	int y_time;
	
	int x_duration;
	int y_duration;

	float new_rotation;
	float new_opacity;
	float new_xscale;
	float new_yscale;
	
	float start_rotation;
	float start_opacity;
	float start_xscale;
	float start_yscale;	
	
	float rotation_change_amount;
	float opacity_change_amount;
	float xscale_change_amount;
	float yscale_change_amount;
	
	int rotation_time;
	int opacity_time;
	int xscale_time;
	int yscale_time;
	
	int rotation_duration;
	int opacity_duration;
	int xscale_duration;
	int yscale_duration;	
	
	krad_ease_t krad_ease_x;
	krad_ease_t krad_ease_y;
	krad_ease_t krad_ease_xscale;
	krad_ease_t krad_ease_yscale;
	krad_ease_t krad_ease_rotation;
	krad_ease_t krad_ease_opacity;
	
};


krad_sprite_t *krad_sprite_create ();
void krad_sprite_destroy (krad_sprite_t *krad_sprite);
krad_sprite_t *krad_sprite_create_from_file (char *filename);
void krad_sprite_reset (krad_sprite_t *krad_sprite);
void krad_sprite_open_file (krad_sprite_t *krad_sprite, char *filename);
void krad_sprite_set_xy (krad_sprite_t *krad_sprite, int x, int y);
void krad_sprite_set_new_xy (krad_sprite_t *krad_sprite, int x, int y);

void krad_sprite_set_scale (krad_sprite_t *krad_sprite, float scale);
void krad_sprite_set_xscale (krad_sprite_t *krad_sprite, float scale);
void krad_sprite_set_yscale (krad_sprite_t *krad_sprite, float scale);
void krad_sprite_set_opacity (krad_sprite_t *krad_sprite, float opacity);
void krad_sprite_set_rotation (krad_sprite_t *krad_sprite, float rotation);

void krad_sprite_set_new_scale (krad_sprite_t *krad_sprite, float scale);
void krad_sprite_set_new_xscale (krad_sprite_t *krad_sprite, float scale);
void krad_sprite_set_new_yscale (krad_sprite_t *krad_sprite, float scale);
void krad_sprite_set_new_opacity (krad_sprite_t *krad_sprite, float opacity);
void krad_sprite_set_new_rotation (krad_sprite_t *krad_sprite, float rotation);

void krad_sprite_set_tickrate (krad_sprite_t *krad_sprite, int tickrate);
void krad_sprite_render (krad_sprite_t *krad_sprite, cairo_t *cr);
void krad_sprite_tick (krad_sprite_t *krad_sprite);
void krad_sprite_render_xy (krad_sprite_t *krad_sprite, cairo_t *cr, int x, int y);


#endif
