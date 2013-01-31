#include "krad_perspective.h"

void sub_vec2 ( krad_position_t* r, krad_position_t* a, krad_position_t* b ) {
  r->x = a->x - b->x;
  r->y = a->y - b->y;
}

void add_vec2 ( krad_position_t* r, krad_position_t* a, krad_position_t* b ) {
  r->x = a->x + b->x;
  r->y = a->y + b->y;
}

void mul_vec2 ( krad_position_t* r, krad_position_t* a, double scalar ) {
  r->x = a->x * scalar;
  r->y = a->y * scalar;
}

void get_pixel_position ( krad_position_t* r, krad_position_t* t, krad_position_t* b, krad_position_t* tl, krad_position_t* bl,krad_position_t* in ) {

  krad_position_t t_x;
  krad_position_t b_x;
  krad_position_t k;

  mul_vec2 ( &t_x, t, in->x );
  mul_vec2 ( &b_x, b, in->x );

  add_vec2 ( &t_x, &t_x, tl );
  add_vec2 ( &b_x, &b_x, bl );

  sub_vec2 ( &k, &b_x, &t_x );
  mul_vec2 ( &k, &k, in->y );

  add_vec2 ( r, &k, &t_x );
}


krad_perspective_t *krad_perspective_create (int width, int height) {

  krad_perspective_t *krad_perspective = (krad_perspective_t *)calloc (1, sizeof(krad_perspective_t));
  
  int rx;
  int ry;
  int x;
  int y;
  int w;
  int h;
  krad_position_t top;
  krad_position_t bot;
  krad_position_t r;
  krad_position_t in;

  krad_perspective->w = width;
  krad_perspective->h = height;
  
  w = krad_perspective->w;
  h = krad_perspective->h;  
  
  krad_perspective->tl.x = 0.0;
  krad_perspective->tl.y = 0.0;

  krad_perspective->tr.x = 1.0;
  krad_perspective->tr.y = 0.0;

  krad_perspective->bl.x = 0.0;
  krad_perspective->bl.y = 1.0;

  krad_perspective->br.x = 1.0;
  krad_perspective->br.y = 1.0;

  krad_perspective->map = (int *)calloc (1, krad_perspective->w * krad_perspective->h * 4);
  
  sub_vec2( &top, &krad_perspective->tr, &krad_perspective->tl );
  sub_vec2( &bot, &krad_perspective->br, &krad_perspective->bl );

  for( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      in.x = (double)x / (double)w;
      in.y = (double)y / (double)h;
      get_pixel_position( &r, &top, &bot, &krad_perspective->tl, &krad_perspective->bl, &in );
      rx = lrint(r.x * (float)w);
      ry = lrint(r.y * (float)h);
      if ( rx < 0 || rx >= w || ry < 0 || ry >= h ) {
        continue;
      }
      krad_perspective->map[x + w * y] = rx + w * ry;
    }
  }

  return krad_perspective;
}

void perspective_destroy (krad_perspective_t *krad_perspective) {
  free (krad_perspective->map);
  free (krad_perspective);
}

void krad_perspective_process (krad_perspective_t *krad_perspective, uint32_t *inframe, uint32_t *outframe) {

  uint32_t* dst = outframe;
  uint32_t* src = inframe;
  
  int w = krad_perspective->w;
  int h = krad_perspective->h;

  int x;
  int y;  
  
  for( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
        dst[x + w * y] = src[krad_perspective->map[x + w * y]];
    }
  }
}

