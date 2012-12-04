#ifndef KRAD_SPRITE_H
#define KRAD_SPRITE_H

#include <turbojpeg.h>

typedef struct krad_sprite_St krad_sprite_t;

#include "krad_radio.h"

#define KRAD_SPRITE_DEFAULT_TICKRATE 4


struct krad_sprite_St {

  char filename[256];
  int frames;
  int frame;
  cairo_surface_t *sprite;
  cairo_pattern_t *sprite_pattern;
  int sheet_width;
	int sheet_height;
  
  krad_compositor_subunit_t *krad_compositor_subunit;
  
};

krad_sprite_t *krad_sprite_create ();
krad_sprite_t *krad_sprite_create_arr (int count);
void krad_sprite_destroy (krad_sprite_t *krad_sprite);
void krad_sprite_destroy_arr (krad_sprite_t *krad_sprite, int count);
krad_sprite_t *krad_sprite_create_from_file (char *filename);
void krad_sprite_reset (krad_sprite_t *krad_sprite);
int krad_sprite_open_file (krad_sprite_t *krad_sprite, char *filename);



//void krad_sprite_set_xy (krad_sprite_t *krad_sprite, int x, int y);
//void krad_sprite_set_new_xy (krad_sprite_t *krad_sprite, int x, int y);

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
