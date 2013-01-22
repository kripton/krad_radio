#include "krad_radio.h"
//#include "krad_compositor.h"

#ifndef KRAD_VECTOR_H
#define KRAD_VECTOR_H


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>  
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <cairo/cairo.h>


#include "krad_system.h"



#define PRO_REEL_SIZE 26.67
#define PRO_REEL_SPEED 38.1
#define NORMAL_REEL_SIZE 17.78
#define NORMAL_REEL_SPEED 19.05
#define SLOW_NORMAL_REEL_SPEED 9.53

// device colors
#define BLUE 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0
#define BLUE_TRANS 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0, 0.255
#define BLUE_TRANS2 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0, 0.144 / 0.255 * 1.0
#define BLUE_TRANS3 0.0, 0.122 / 0.255 * 1.0, 0.112 / 0.255 * 1.0, 0.144 / 0.255 * 1.0
#define GREEN  0.001 / 0.255 * 1.0, 0.187 / 0.255 * 1.0, 0.0
#define LGREEN  0.001 / 0.255 * 1.0, 0.187 / 0.255 * 1.0, 0.0, 0.044 / 0.255 * 1.0
#define WHITE 0.222 / 0.255 * 1.0, 0.232 / 0.255 * 1.0, 0.233 / 0.255 * 1.0
#define WHITE_TRANS 0.222 / 0.255 * 1.0, 0.232 / 0.255 * 1.0, 0.233 / 0.255 * 1.0, 0.555
#define ORANGE  0.255 / 0.255 * 1.0, 0.080 / 0.255 * 1.0, 0.0
#define GREY  0.197 / 0.255 * 1.0, 0.203 / 0.255 * 1.0, 0.203 / 0.255   * 1.0
#define GREY2  0.037 / 0.255 * 1.0, 0.037 / 0.255 * 1.0, 0.038 / 0.255   * 1.0
#define BGCOLOR  0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.033 / 0.255   * 1.0
#define GREY3  0.103 / 0.255 * 1.0, 0.103 / 0.255 * 1.0, 0.124 / 0.255   * 1.0

#define BGCOLOR_TRANS  0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.144 / 0.255 * 1.0

#define BGCOLOR_CLR  0.0 / 0.255 * 1.0, 0.0 / 0.255 * 1.0, 0.0 / 0.255   * 1.0, 0.255 / 0.255   * 1.0


typedef struct krad_vector_St krad_vector_t;

struct krad_vector_St {
  krad_vector_type_t krad_vector_type;
  krad_compositor_subunit_t *krad_compositor_subunit;
};


krad_vector_t *krad_vector_create (krad_mixer_t *krad_mixer);
krad_vector_t *krad_vector_create_arr (krad_mixer_t *krad_mixer, int count);
void krad_vector_destroy (krad_vector_t *krad_vector);
void krad_vector_destroy_arr (krad_vector_t *krad_vector, int count);
void krad_vector_reset (krad_vector_t *krad_vector);
void krad_vector_set_xy (krad_vector_t *krad_vector, int x, int y);
void krad_vector_set_new_xy (krad_vector_t *krad_vector, int x, int y);

void krad_vector_set_vector (krad_vector_t *krad_vector, char *vector);
void krad_vector_set_scale (krad_vector_t *krad_vector, float scale);
void krad_vector_set_xscale (krad_vector_t *krad_vector, float scale);
void krad_vector_set_yscale (krad_vector_t *krad_vector, float scale);
void krad_vector_set_opacity (krad_vector_t *krad_vector, float opacity);
void krad_vector_set_rotation (krad_vector_t *krad_vector, float rotation);


void krad_vector_expand (krad_vector_t *krad_vector, cairo_t *cr, int width);

void krad_vector_set_font (krad_vector_t *krad_vector, char *font);

void krad_vector_set_new_scale (krad_vector_t *krad_vector, float scale);
void krad_vector_set_new_opacity (krad_vector_t *krad_vector, float opacity);
void krad_vector_set_new_rotation (krad_vector_t *krad_vector, float rotation);

void krad_vector_set_tickrate (krad_vector_t *krad_vector, int tickrate);
void krad_vector_render (krad_vector_t *krad_vector, cairo_t *cr);
void krad_vector_tick (krad_vector_t *krad_vector);
void krad_vector_render_xy (krad_vector_t *krad_vector, cairo_t *cr, int x, int y);

krad_vector_rep_t *krad_vector_to_vector_rep (krad_vector_t *krad_vector, krad_vector_rep_t *krad_vector_rep);

void krad_vector_render_meter (cairo_t *cr, int x, int y, int size, float pos, float opacity);
void krad_vector_render_hex (cairo_t *cr, int x, int y, int w, float r, float g, float b, float opacity);
void krad_vector_render_wheel (cairo_t *cr, int width, int height);
void krad_vector_render_grid (cairo_t *cr, int x, int y, int w, int h, int lines, float r, float g, float b, float opacity);
void krad_vector_render_cube (cairo_t *cr, int x, int y, int w, int h, float r, float g, float b, float opacity);
void krad_vector_render_curve (cairo_t *cr, int w, int h);
void krad_vector_render_circle (cairo_t *cr, int x, int y, float radius, float r, float g, float b, float opacity);
void krad_vector_render_rectangle (cairo_t *cr, int x, int y, float w, float h, float r, float g, float b, float opacity);
void krad_vector_render_triangle (cairo_t *cr, int x, int y, float w, float h, float r, float g, float b, float opacity);
void krad_vector_render_arrow (cairo_t *cr, int x, int y, float w, float h, float r, float g, float b, float opacity);
void krad_vector_render_viper (cairo_t * cr, int x, int y, int size, int direction);
void krad_vector_render_clock (cairo_t *cr, int x, int y, int width, int height, float opacity);
void krad_vector_render_blurred_rectangle (cairo_t *cr, int x, int y, int w, int h, float r, float g, float b, float a);

#endif
