#include "krad_vector.h"
/*
void krad_vector_destroy (krad_vector_t *krad_vector) {
  
  krad_vector_reset (krad_vector);
  
  free (krad_vector);

}

void krad_vector_destroy_arr (krad_vector_t *krad_vector, int count) {
  
  int s;
  
  s = 0;
  
  for (s = 0; s < count; s++) {
    krad_vector_reset (&krad_vector[s]);
  }

  free (krad_vector);

}

krad_vector_t *krad_vector_create_arr (krad_mixer_t *krad_mixer, int count) {

  int s;
  krad_vector_t *krad_vector;

  s = 0;

  if ((krad_vector = calloc (count, sizeof (krad_vector_t))) == NULL) {
    failfast ("Krad Vector mem alloc fail");
  }

  for (s = 0; s < count; s++) {
    krad_vector[s].krad_compositor_subunit = krad_compositor_subunit_create();
    krad_vector_reset (&krad_vector[s]);
  }
  
  return krad_vector;

}

krad_vector_t *krad_vector_create (krad_mixer_t *krad_mixer) {
  return krad_vector_create_arr (krad_mixer, 1);
}

void krad_vector_reset (krad_vector_t *krad_vector) {
  
  krad_compositor_subunit_reset (krad_vector->krad_compositor_subunit);
  
}

void krad_vector_set_type (krad_vector_t *krad_vector, char *vector_type) {

  krad_vector->krad_vector_type = krad_string_to_vector_type (vector_type);
  krad_compositor_subunit_set_opacity(krad_vector->krad_compositor_subunit, 0.0f);
  krad_compositor_subunit_set_new_opacity(krad_vector->krad_compositor_subunit, krad_vector->krad_compositor_subunit->opacity, krad_vector->krad_compositor_subunit->duration);

}


void krad_vector_render_xy (krad_vector_t *krad_vector, cairo_t *cr, int x, int y) {

  krad_compositor_subunit_set_xy (krad_vector->krad_compositor_subunit, x, y);
  krad_vector_render (krad_vector, cr);
}

void krad_vector_tick (krad_vector_t *krad_vector) {

  krad_vector->krad_compositor_subunit->tick++;

  if (krad_vector->krad_compositor_subunit->tick >= krad_vector->krad_compositor_subunit->duration) {
    krad_vector->krad_compositor_subunit->tick = 0;
  }

  krad_compositor_subunit_update (krad_vector->krad_compositor_subunit);
  
}

void krad_vector_render (krad_vector_t *krad_vector, cairo_t *cr) {

  float peak;
  cairo_save (cr);

  if ((krad_vector->krad_compositor_subunit->xscale != 1.0f) || (krad_vector->krad_compositor_subunit->yscale != 1.0f)) {
    cairo_translate (cr, krad_vector->krad_compositor_subunit->x, krad_vector->krad_compositor_subunit->y);
    //cairo_translate (cr, ((krad_vector->krad_compositor_subunit->width / 2) * krad_vector->krad_compositor_subunit->xscale),
    //       ((krad_vector->krad_compositor_subunit->height / 2) * krad_vector->krad_compositor_subunit->yscale));
    cairo_translate (cr, krad_vector->krad_compositor_subunit->width / 2, krad_vector->krad_compositor_subunit->height / 2);
    cairo_scale (cr, krad_vector->krad_compositor_subunit->xscale, krad_vector->krad_compositor_subunit->yscale);
    cairo_translate (cr, ((krad_vector->krad_compositor_subunit->width / -2) * krad_vector->krad_compositor_subunit->xscale),
            ((krad_vector->krad_compositor_subunit->height / -2) * krad_vector->krad_compositor_subunit->yscale));
    //cairo_translate (cr, krad_vector->krad_compositor_subunit->width / -2, krad_vector->krad_compositor_subunit->height / -2);
    cairo_translate (cr, krad_vector->krad_compositor_subunit->x * -1, krad_vector->krad_compositor_subunit->y * -1);    
  }
  
  if (krad_vector->krad_compositor_subunit->rotation != 0.0f) {
    cairo_translate (cr, krad_vector->krad_compositor_subunit->x, krad_vector->krad_compositor_subunit->y);  
    cairo_translate (cr, krad_vector->krad_compositor_subunit->width / 2, krad_vector->krad_compositor_subunit->height / 2);
    cairo_rotate (cr, krad_vector->krad_compositor_subunit->rotation * (M_PI/180.0));
    cairo_translate (cr, krad_vector->krad_compositor_subunit->width / -2, krad_vector->krad_compositor_subunit->height / -2);    
    cairo_translate (cr, krad_vector->krad_compositor_subunit->x * -1, krad_vector->krad_compositor_subunit->y * -1);
  }


  
  switch ( krad_vector->krad_vector_type ) {

  case METER:
    peak = krad_mixer_portgroup_read_peak (krad_vector->krad_mixer_portgroup);
    peak = krad_mixer_peak_scale (peak);
    krad_vector_render_meter (cr, krad_vector->krad_compositor_subunit->x, krad_vector->krad_compositor_subunit->y, krad_vector->krad_compositor_subunit->yscale, peak, krad_vector->krad_compositor_subunit->opacity);
    break;
    
  case HEX:

    krad_vector_render_hex (cr, krad_vector->krad_compositor_subunit->x, 
                            krad_vector->krad_compositor_subunit->y, 
                            krad_vector->krad_compositor_subunit->width,
//                            0.3 * peak,
                            krad_vector->krad_compositor_subunit->red,
                            krad_vector->krad_compositor_subunit->green,
                            krad_vector->krad_compositor_subunit->blue,
                            krad_vector->krad_compositor_subunit->opacity);
    break;
    
    
  case WHEEL:
    krad_vector_render_wheel (cr, 20, 2);
    break;
    
  case CUBE:
    krad_vector_render_cube (cr, krad_vector->krad_compositor_subunit->x, 
                             krad_vector->krad_compositor_subunit->y, 
                             krad_vector->krad_compositor_subunit->width,
                             krad_vector->krad_compositor_subunit->height,
                             //0.1 * peak,
                             //0.1 * peak,
                             krad_vector->krad_compositor_subunit->red,
                             krad_vector->krad_compositor_subunit->green,
                             krad_vector->krad_compositor_subunit->blue,
                             krad_vector->krad_compositor_subunit->opacity);
  
   break;
  case CURVE:
    krad_vector_render_curve (cr, krad_vector->krad_compositor_subunit->x,
                              krad_vector->krad_compositor_subunit->y);
    break;
  
  case CIRCLE:
    krad_vector_render_circle (cr, krad_vector->krad_compositor_subunit->x,
                               krad_vector->krad_compositor_subunit->y,
                               krad_vector->krad_compositor_subunit->width,
                               krad_vector->krad_compositor_subunit->red,
                               krad_vector->krad_compositor_subunit->green,
                               krad_vector->krad_compositor_subunit->blue,
                               krad_vector->krad_compositor_subunit->opacity);
     break;
  case RECT:

    //krad_vector->krad_compositor_subunit->width = krad_vector->krad_compositor_subunit->xscale;
    //krad_vector->krad_compositor_subunit->height = krad_vector->krad_compositor_subunit->yscale;
    krad_vector_render_rectangle (cr, krad_vector->krad_compositor_subunit->x,
                                  krad_vector->krad_compositor_subunit->y,
                                  krad_vector->krad_compositor_subunit->width,
                                  krad_vector->krad_compositor_subunit->height,
                                  krad_vector->krad_compositor_subunit->red,
                                  krad_vector->krad_compositor_subunit->green,
                                  krad_vector->krad_compositor_subunit->blue,
                                  krad_vector->krad_compositor_subunit->opacity);
    break;

  case TRIANGLE:


    krad_vector_render_triangle (cr, krad_vector->krad_compositor_subunit->x - krad_vector->krad_compositor_subunit->xscale * sin (krad_vector->krad_compositor_subunit->rotation * M_PI/180.0),
                                 krad_vector->krad_compositor_subunit->y - krad_vector->krad_compositor_subunit->xscale * cos( 30 * M_PI/180.0) * cos (krad_vector->krad_compositor_subunit->rotation * M_PI/180.0),
                                 krad_vector->krad_compositor_subunit->width,
                                 krad_vector->krad_compositor_subunit->height,
                                 krad_vector->krad_compositor_subunit->red,
                                 krad_vector->krad_compositor_subunit->green,
                                 krad_vector->krad_compositor_subunit->blue,
                                 krad_vector->krad_compositor_subunit->opacity);
    break;

  case ARROW:

    krad_vector_render_arrow (cr, krad_vector->krad_compositor_subunit->x,
                                 krad_vector->krad_compositor_subunit->y,
                                 krad_vector->krad_compositor_subunit->width,
                                 krad_vector->krad_compositor_subunit->height,
                                 krad_vector->krad_compositor_subunit->red,
                                 krad_vector->krad_compositor_subunit->green,
                                 krad_vector->krad_compositor_subunit->blue,
                                 krad_vector->krad_compositor_subunit->opacity);
    break;

  case GRID:


    krad_vector_render_grid (cr, krad_vector->krad_compositor_subunit->x,
                             krad_vector->krad_compositor_subunit->y,
                             krad_vector->krad_compositor_subunit->width,
                             krad_vector->krad_compositor_subunit->height,
                             6,
                             krad_vector->krad_compositor_subunit->red,
                             krad_vector->krad_compositor_subunit->green,
                             krad_vector->krad_compositor_subunit->blue,
                             krad_vector->krad_compositor_subunit->opacity);
    break;

  case VIPER:
    krad_vector_render_viper (cr, krad_vector->krad_compositor_subunit->x,
                              krad_vector->krad_compositor_subunit->y,
                              (int) krad_vector->krad_compositor_subunit->xscale, (int) krad_vector->krad_compositor_subunit->yscale);
    break;

  case CLOCK:


    krad_vector_render_clock (cr, krad_vector->krad_compositor_subunit->x,
                              krad_vector->krad_compositor_subunit->y,
                              (int) krad_vector->krad_compositor_subunit->width,
                              (int) krad_vector->krad_compositor_subunit->height,
                               krad_vector->krad_compositor_subunit->opacity);
    break;

  case BLURRED_RECT:

    krad_vector_render_blurred_rectangle (cr, krad_vector->krad_compositor_subunit->x,
                                          krad_vector->krad_compositor_subunit->y,
                                          krad_vector->krad_compositor_subunit->width,
                                          krad_vector->krad_compositor_subunit->height,
                                          krad_vector->krad_compositor_subunit->red,
                                          krad_vector->krad_compositor_subunit->green,
                                          krad_vector->krad_compositor_subunit->blue,
                                          krad_vector->krad_compositor_subunit->opacity);
    break;
  }
  
    cairo_restore (cr);
    
    krad_vector_tick (krad_vector);
}
static void
patch_arc (cairo_pattern_t *pattern,
     double x, double y,
     double start, double end,
     double radius,
     double r, double g, double b, double a)
{
    double r_sin_A, r_cos_A;
    double r_sin_B, r_cos_B;
    double h;

    r_sin_A = radius * sin (start);
    r_cos_A = radius * cos (start);
    r_sin_B = radius * sin (end);
    r_cos_B = radius * cos (end);

    h = 4.0/3.0 * tan ((end - start) / 4.0);

    cairo_mesh_pattern_begin_patch (pattern);

    cairo_mesh_pattern_move_to (pattern, x, y);
    cairo_mesh_pattern_line_to (pattern,
        x + r_cos_A,
        y + r_sin_A);

    cairo_mesh_pattern_curve_to (pattern,
         x + r_cos_A - h * r_sin_A,
         y + r_sin_A + h * r_cos_A,
         x + r_cos_B + h * r_sin_B,
         y + r_sin_B - h * r_cos_B,
         x + r_cos_B,
         y + r_sin_B);

    cairo_mesh_pattern_line_to (pattern, x, y);

    cairo_mesh_pattern_set_corner_color_rgba (pattern, 0, r, g, b, a);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, 1, r, g, b, 0);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, 2, r, g, b, 0);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, 3, r, g, b, a);

    cairo_mesh_pattern_end_patch (pattern);
}

static void
patch_line (cairo_pattern_t *pattern,
      double x0, double y0,
      double x1, double y1,
      double radius,
      double r, double g, double b, double a)
{
    double dx = y1 - y0;
    double dy = x0 - x1;
    double len = radius / hypot (dx, dy);

    dx *= len;
    dy *= len;

    cairo_mesh_pattern_begin_patch (pattern);

    cairo_mesh_pattern_move_to (pattern, x0, y0);
    cairo_mesh_pattern_line_to (pattern, x0 + dx, y0 + dy);
    cairo_mesh_pattern_line_to (pattern, x1 + dx, y1 + dy);
    cairo_mesh_pattern_line_to (pattern, x1, y1);

    cairo_mesh_pattern_set_corner_color_rgba (pattern, 0, r, g, b, a);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, 1, r, g, b, 0);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, 2, r, g, b, 0);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, 3, r, g, b, a);

    cairo_mesh_pattern_end_patch (pattern);
}

static void
patch_rect (cairo_pattern_t *pattern,
      double x0, double y0,
      double x1, double y1,
      double radius,
      double r, double g, double b, double a)
{
    patch_arc  (pattern, x0, y0,   -M_PI, -M_PI/2, radius, r, g, b, a);
    patch_arc  (pattern, x1, y0, -M_PI/2,       0, radius, r, g, b, a);
    patch_arc  (pattern, x1, y1,       0,  M_PI/2, radius, r, g, b, a);
    patch_arc  (pattern, x0, y1,  M_PI/2,    M_PI, radius, r, g, b, a);

    patch_line (pattern, x0, y0, x1, y0, radius, r, g, b, a);
    patch_line (pattern, x1, y0, x1, y1, radius, r, g, b, a);
    patch_line (pattern, x1, y1, x0, y1, radius, r, g, b, a);
    patch_line (pattern, x0, y1, x0, y0, radius, r, g, b, a);
}

void krad_vector_render_blurred_rectangle (cairo_t *cr, int x, int y, int w, int h, float r, float g, float b, float a) {

    cairo_pattern_t *pattern;
    float radius = 60;

    pattern = cairo_pattern_create_mesh ();

    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_paint (cr);

    patch_rect (pattern, x - w /2.0, y - h / 2.0, x + w / 2.0, y + h / 2.0, radius, 0, 0, 0, a);

    cairo_set_source (cr, pattern);
    cairo_paint (cr);

}

krad_vector_rep_t *krad_vector_to_vector_rep (krad_vector_t *krad_vector, krad_vector_rep_t *krad_vector_rep) {
  
  krad_vector_rep_t *krad_vector_rep_ret;
  
  if (krad_vector_rep == NULL) {
    krad_vector_rep_ret = krad_compositor_vector_rep_create();
  } else {
    krad_vector_rep_ret = krad_vector_rep;
  }
  
  krad_vector_rep_ret->krad_vector_type = krad_vector->krad_vector_type;
  
  krad_vector_rep_ret->controls->red = krad_vector->krad_compositor_subunit->red;
  krad_vector_rep_ret->controls->green = krad_vector->krad_compositor_subunit->green;
  krad_vector_rep_ret->controls->blue = krad_vector->krad_compositor_subunit->blue;
  
  krad_vector_rep_ret->controls->x = krad_vector->krad_compositor_subunit->x;
  krad_vector_rep_ret->controls->y = krad_vector->krad_compositor_subunit->y;
  krad_vector_rep_ret->controls->z = krad_vector->krad_compositor_subunit->z;
  
  krad_vector_rep_ret->controls->duration = krad_vector->krad_compositor_subunit->duration;

  krad_vector_rep_ret->controls->width = krad_vector->krad_compositor_subunit->width;
  krad_vector_rep_ret->controls->height = krad_vector->krad_compositor_subunit->height;
    
  krad_vector_rep_ret->controls->xscale = krad_vector->krad_compositor_subunit->xscale;
  krad_vector_rep_ret->controls->yscale = krad_vector->krad_compositor_subunit->yscale;
    
  krad_vector_rep_ret->controls->rotation = krad_vector->krad_compositor_subunit->rotation;
  krad_vector_rep_ret->controls->opacity = krad_vector->krad_compositor_subunit->opacity;
   
  return krad_vector_rep_ret;
}

void krad_vector_render_meter (cairo_t *cr, int x, int y, int size, float pos, float opacity) {


  pos = pos * 1.8f - 90.0f;

  cairo_save(cr);

  cairo_new_path ( cr );

  cairo_translate (cr, x, y);
  cairo_set_line_width(cr, 0.05 * size);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_source_rgba(cr, GREY3, opacity);
  cairo_arc (cr, 0, 0, 0.8 * size, M_PI, 0);
  cairo_stroke (cr);

  cairo_set_source_rgba(cr, ORANGE, opacity);
  cairo_arc (cr, 0, 0, 0.65 * size, 1.8 * M_PI, 0);
  cairo_stroke (cr);

  cairo_arc (cr, size - 0.56 * size, -0.15 * size, 0.07 * size, 0, 2 * M_PI);
  cairo_fill(cr);

  cairo_save(cr);
  cairo_translate (cr, 0.05 * size, 0);
  cairo_rotate (cr, pos * (M_PI/180.0));  
  //
  cairo_pattern_t *pat;
  pat = cairo_pattern_create_linear (0, 0, 0.11 * size, 0);
  cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 0, 0);
  cairo_pattern_add_color_stop_rgba (pat, 0.3, 0, 0, 0, 0);  
  cairo_pattern_add_color_stop_rgba (pat, 0.4, 0, 0, 0, 1);  
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 0);
  cairo_set_source (cr, pat);
  cairo_rectangle (cr, 0, 0, 0.11 * size, -size);
  cairo_fill (cr);
  */
  /*
  cairo_set_source_rgba(cr, WHITE, opacity);
  cairo_move_to (cr, 0, 0);
  cairo_line_to (cr, 0, -size * 0.93);
  cairo_stroke (cr);

  cairo_restore(cr);
  cairo_set_source_rgba(cr, WHITE, opacity);
  cairo_arc (cr, 0.05 * size, 0, 0.1 * size, 0, 2 * M_PI);
  cairo_fill(cr);

  cairo_restore(cr);

}

void krad_vector_render_hex (cairo_t *cr, int x, int y, int w, float r, float g, float b, float opacity) {

  cairo_pattern_t *pat;

  static float hexrot = 0;

  cairo_save(cr);

  cairo_set_line_width(cr, 1);
  cairo_set_source_rgba(cr, r, g, b, opacity);

  int r1;
  float scale;

  scale = 2.5;

  r1 = ((w)/2 * sqrt(3));

  cairo_translate (cr, x, y);
  cairo_rotate (cr, hexrot * (M_PI/180.0));
  cairo_translate (cr, -(w/2), -r1);

  // draw radius 
  //cairo_move_to (cr, w/2, 0);
  //cairo_line_to (cr, w/2, r1);
  //cairo_stroke (cr);

  cairo_move_to (cr, 0, 0);
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  //cairo_stroke_preserve (cr);


  hexrot += 1.5;
  cairo_fill (cr);

  cairo_restore(cr);


//-----------------------

  cairo_save(cr);

  cairo_set_line_width(cr, 1.5);
  cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
  cairo_set_source_rgba(cr, GREY, opacity);


  cairo_translate (cr, x, y);
  cairo_rotate (cr, hexrot * (M_PI/180.0));
  cairo_translate (cr, -((w * scale)/2), -r1 * scale);
  cairo_scale(cr, scale, scale);
  //hexrot += 0.11;

  cairo_move_to (cr, 0, 0);
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 60 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);

  //cairo_stroke_preserve (cr);

  cairo_rotate (cr, 60 * (M_PI/180.0));


  // draw radius 
  //cairo_move_to (cr, w/2, 0);
  //cairo_line_to (cr, w/2, r1);
  //cairo_stroke_preserve (cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
  pat = cairo_pattern_create_radial (w/2, r1, 3, w/2, r1, r1*scale);
  cairo_pattern_add_color_stop_rgba (pat, 0, r, g, b, opacity);
  //cairo_pattern_add_color_stop_rgba (pat, 0.6, 0, 0, 1, 0.3);
  //cairo_pattern_add_color_stop_rgba (pat, 0.8, 0, 0, 1, 0.05);
  cairo_pattern_add_color_stop_rgba (pat, 0.4, 0, 0, 0, 0);
  cairo_set_source (cr, pat);

  cairo_fill (cr);
  cairo_pattern_destroy (pat);



  cairo_restore(cr);

}

void krad_vector_render_grid (cairo_t *cr, int x, int y, int w, int h, int lines, float r, float g, float b, float opacity) {

  int count;

  //srand(time(NULL));
  cairo_save(cr);
  cairo_translate (cr, x + w/2, y + h/2);

  cairo_translate (cr, -x + -w/2, -y + -h/2);
  cairo_set_line_width(cr, 10);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_source_rgba(cr, r, g, b, opacity);

  cairo_save(cr);

  for (count = 1; count != lines; count++) {

    cairo_move_to (cr, x + (w/lines) * count, y);
    cairo_line_to (cr, x + (w/lines) * count, y + h);
    cairo_stroke (cr);

  }

  cairo_restore(cr);

  for (count = 1; count != lines; count++) {

    cairo_move_to (cr, x, y + (h/lines) * count);
    cairo_line_to (cr, x + w, y + (h/lines) * count);
    cairo_stroke (cr);

  }


  cairo_rectangle (cr, x, y, w, h);
  cairo_stroke (cr);

  cairo_restore(cr);
}

void krad_vector_render_cube (cairo_t *cr, int x, int y, int w, int h, float r, float g, float b, float opacity) {

  static float shear = -0.579595f;
  static float scale = 0.86062;
  static float rot = 30.0f;

  int lines;

  lines = 5;

  cairo_matrix_t matrix_rot;
  cairo_matrix_t matrix_rot2;
  cairo_matrix_t matrix_rot3;

  cairo_matrix_t matrix_trans;
  cairo_matrix_t matrix_trans2;
  cairo_matrix_t matrix_trans3;

  cairo_matrix_t matrix_sher;
  cairo_matrix_t matrix_sher2;
  cairo_matrix_t matrix_sher3;

  cairo_matrix_t matrix_scal;
  cairo_matrix_t matrix_scal2;
  cairo_matrix_t matrix_scal3;
  cairo_matrix_t matrix_scal4;

  cairo_matrix_t matrix_scal_sher;
  cairo_matrix_t matrix_scal_sher_rot;
  cairo_matrix_t matrix_scal_sher_rot_trans;


  cairo_matrix_t matrix_scal_sher2;
  cairo_matrix_t matrix_scal_sher_rot2;
  cairo_matrix_t matrix_scal_sher_rot_trans2;

  cairo_matrix_t matrix_scal_sher3;
  cairo_matrix_t matrix_scal_sher_rot3;
  cairo_matrix_t matrix_scal_sher_rot_trans3;

  cairo_matrix_init_translate(&matrix_trans, x - 1, y + 0.5);
  cairo_matrix_init_translate(&matrix_trans2, x, y - h);
  cairo_matrix_init_translate(&matrix_trans3, x + 1, y + 0.5);

  cairo_matrix_init_scale(&matrix_scal, 1.0, scale);

  cairo_matrix_init_scale(&matrix_scal2, 1.0, 1.0);
  cairo_matrix_init_scale(&matrix_scal3, 1.0, 1.0);
  cairo_matrix_init_scale(&matrix_scal4, 1.0, 1.0);

  cairo_matrix_init_rotate(&matrix_rot, (90.0f) * (M_PI/180.0));
  cairo_matrix_init_rotate(&matrix_rot2, (rot) * (M_PI/180.0));
  cairo_matrix_init_rotate(&matrix_rot3, (-30.0f) * (M_PI/180.0));

  cairo_matrix_init(&matrix_sher, 1.0, 0.0, shear, 1.0, 0.0, 0.0);
  cairo_matrix_init(&matrix_sher2, 1.0, 0.0, shear, 1.0, 0.0, 0.0);
  cairo_matrix_init(&matrix_sher3, 1.0, 0.0, shear, 1.0, 0.0, 0.0);


  cairo_matrix_multiply (&matrix_scal_sher, &matrix_scal, &matrix_sher);
  cairo_matrix_multiply (&matrix_scal_sher2, &matrix_scal, &matrix_sher2);
  cairo_matrix_multiply (&matrix_scal_sher3, &matrix_scal, &matrix_sher3);


  cairo_matrix_multiply (&matrix_scal_sher_rot, &matrix_scal_sher, &matrix_rot);
  cairo_matrix_multiply (&matrix_scal_sher_rot2, &matrix_scal_sher2, &matrix_rot2);
  cairo_matrix_multiply (&matrix_scal_sher_rot3, &matrix_scal_sher3, &matrix_rot3);  

  cairo_matrix_multiply (&matrix_scal_sher_rot_trans, &matrix_scal_sher_rot, &matrix_trans);

  cairo_matrix_multiply (&matrix_scal_sher_rot_trans2, &matrix_scal_sher_rot2, &matrix_trans2);
  cairo_matrix_multiply (&matrix_scal_sher_rot_trans3, &matrix_scal_sher_rot3, &matrix_trans3);

  cairo_save(cr);
  cairo_set_line_width(cr, 1);
  cairo_set_source_rgba (cr, r, g, b, opacity);

  cairo_save(cr);

  cairo_transform(cr, &matrix_scal_sher_rot_trans2);
  //cairo_transform(cr, &matrix_scal4);
  cairo_set_source_rgba(cr, r, g, b, 0.044 / 0.255 * opacity);
  //krad_gui_render_grid (krad_gui, w, h, w, h, lines);
  cairo_stroke (cr);

  cairo_set_source_rgba (cr, r, g, b, opacity);
  cairo_rectangle (cr, 0, 0, w, h);
  //cairo_fill (cr);
  cairo_stroke (cr);

  cairo_restore(cr);
  cairo_save(cr);

  //cairo_set_source_rgb(cr, ORANGE);
  cairo_transform(cr, &matrix_scal_sher_rot_trans);
  //cairo_transform(cr, &matrix_scal2);
  //cairo_transform(cr, &matrix_scal4);

  cairo_set_source_rgba(cr, r, g, b, 0.044 / 0.255 * opacity);
  cairo_move_to(cr, 0, 0);
  cairo_line_to (cr, w, h);
  cairo_stroke (cr);

  cairo_set_source_rgba (cr, r, g, b, opacity);
  krad_vector_render_grid (cr, 0, 0, w, h, lines, r, g, b, opacity);

  cairo_rectangle (cr, 0, 0, w, h);
  //cairo_fill (cr);
  cairo_stroke (cr);

  cairo_restore(cr);
  cairo_save(cr);

  //cairo_set_source_rgb(cr, BLUE);
  cairo_transform(cr, &matrix_scal_sher_rot_trans3);
  //cairo_transform(cr, &matrix_scal3);
  //cairo_transform(cr, &matrix_scal4);

  cairo_set_source_rgba(cr, r, g, b, 0.044 / 0.255 * opacity);
  cairo_move_to(cr, 0, 0);
  cairo_line_to (cr, w, h);
  cairo_stroke (cr);


  cairo_set_source_rgba(cr, r, g, b, opacity);
  //krad_gui_render_grid (krad_gui, 0, 0, w, h, lines);
  cairo_rectangle (cr, 0, 0, w, h);
  //cairo_fill (cr);  
  cairo_stroke (cr);

  cairo_restore(cr);
  cairo_restore(cr);


  // other method
  if (0) {

    cairo_save(cr);

    cairo_transform(cr, &matrix_scal2);  
    cairo_transform(cr, &matrix_scal_sher_rot_trans2);

    static float h_test = 170;

    int t_x, t_y, t_w, t_h;
    int t2_x, t2_y, t2_w, t2_h;

    t_x = 500;
    t_y = 200;

    t_x = 0;
    t_y = 0;


    t_w = 200;
    t_h = 200;

    t2_w = t_w;
    t2_h = t_h;

    t2_x = (t_x + t2_w/2) - h_test;
    t2_y = (t_y + t2_h/2) - h_test;


    cairo_set_line_width(cr, 3);
    cairo_set_source_rgba(cr, ORANGE, opacity);


    cairo_move_to (cr, t_x, t_y);
    cairo_line_to (cr, t2_x, t2_y);  

    cairo_move_to (cr, t_x + t_w, t_y);
    cairo_line_to (cr, t2_x + t2_w, t2_y);  

    cairo_move_to (cr, t_x, t_y + t_h);
    cairo_line_to (cr, t2_x, t2_y + t2_h);  

    cairo_move_to (cr, t_x + t_w, t_y + t_h);
    cairo_line_to (cr, t2_x + t2_w, t2_y + t2_h);  

    cairo_stroke (cr);

    cairo_rectangle (cr, t_x, t_y, t_w, t_h);


    cairo_rectangle (cr, t2_x, t2_y, t2_w, t2_h);

    cairo_stroke (cr);

    cairo_restore(cr);

  }

}

void krad_vector_render_wheel (cairo_t *cr, int width, int height) {

  int diameter;
  int inner_diameter;
  static int wheel_angle = 0;
  int s;
  int num_spokes;
  int spoke_distance;

  num_spokes = 10;
  spoke_distance = 360 / num_spokes;


  diameter = (height/4 * 3.5) / 2;
  inner_diameter = diameter/8;

  cairo_save(cr);
  cairo_new_path (cr);
  // wheel
  cairo_set_line_width(cr, 5);
  cairo_set_source_rgb(cr, WHITE);
  cairo_translate (cr, width/3, height/2);
  cairo_arc (cr, 0.0, 0.0, inner_diameter, 0, 2*M_PI);
  cairo_stroke(cr);
  cairo_arc(cr, 0.0, 0.0, diameter, 0, 2 * M_PI);
  cairo_stroke(cr);

  // spokes
  cairo_set_source_rgb(cr, WHITE);
  cairo_set_line_width(cr, 35);

  cairo_rotate (cr, (wheel_angle % 360) * (M_PI/180.0));

  for (s = 0; s < num_spokes; s++) {
    cairo_rotate (cr, spoke_distance * (M_PI/180.0));
    cairo_move_to(cr, inner_diameter,  0.0);
    cairo_line_to (cr, diameter, 0.0);
    cairo_move_to(cr, 0.0, 0.0);
    cairo_stroke(cr);
  }

  wheel_angle += 1;

  cairo_restore(cr);

}

void krad_vector_render_curve (cairo_t *cr, int w, int h) {

  cairo_set_line_width(cr, 3.5);
  cairo_set_source_rgb(cr, BLUE);

  static float pointy_positioner = 0.0f;
  int pointy;
  int pointy_start = 15;
  int pointy_range = 83;
  float pointy_final;
  float pointy_final_adj;
  float speed = 0.021;
  
  pointy = pointy_start + (pointy_range / 2) + round(pointy_range * sin(pointy_positioner) / 2);

  pointy_positioner += speed;

  if (pointy_positioner >= 2 * M_PI) {
    pointy_positioner -= 2 * M_PI;
    //new_speed = 1;
  }
  
  pointy_final = pointy * 0.01f;
  
  pointy_final_adj = pointy_final / 3.0f;
  
  int point1_x, point1_y, point2_x, point2_y;
  
  point1_x =  (0.250 + pointy_final_adj) * w;
  point1_y =  0.25 * h;
  point2_x =  (0.75 - pointy_final_adj) * w;
  point2_y =  0.25 * h;
  
  point1_y =  pointy_final * h;
  point2_y =  pointy_final * h;
    
  ///printk ("f1 %f f2 %f\n", pointy_final, pointy_final_adj);
  cairo_move_to (cr, 20, 70);
  cairo_curve_to (cr, point1_x, point1_y, point2_x, point2_y, 0.85 * w, 0.85 * h);
  cairo_stroke (cr);
  
}

void krad_vector_render_circle (cairo_t *cr, int x, int y, float radius, float r, float g, float b, float opacity) {
  
  cairo_set_source_rgba(cr, r, g, b, opacity);
  cairo_arc (cr, x, y, radius, 0, 2*M_PI);
  cairo_fill (cr);
  
}

void krad_vector_render_rectangle (cairo_t *cr, int x, int y, float w, float h, float r, float g, float b, float opacity) {

  cairo_save(cr);

  cairo_set_source_rgba(cr, r, g, b, opacity);
//  printk ("scales: %f %f", w, h);
  cairo_set_line_width(cr, 0);
  
//    cairo_rectangle(cr, (double)x, (double)y, (double)w, (double)h);
    

  cairo_move_to (cr, x-w/2.0, y-h/2.0);
  cairo_rel_line_to (cr, w, 0);
  cairo_rel_line_to (cr, 0, h);
  cairo_rel_line_to (cr, -w, 0);
  cairo_close_path (cr);

  cairo_stroke_preserve(cr);

  cairo_fill(cr);

  cairo_restore(cr);

}

void krad_vector_render_triangle (cairo_t *cr, int x, int y, float w, float h, float r, float g, float b, float opacity) {

  cairo_save(cr);

  cairo_set_source_rgba(cr, r, g, b, opacity);
  cairo_set_line_width(cr, 0);

  cairo_move_to (cr, x, y);
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 120 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_rotate (cr, 120 * (M_PI/180.0));
  cairo_rel_line_to (cr, w, 0);
  cairo_close_path (cr);

  cairo_stroke_preserve(cr);

  cairo_fill(cr);

  cairo_restore(cr);

}


void krad_vector_render_arrow (cairo_t *cr, int x, int y, float w, float h, float r, float g, float b, float opacity) {

  cairo_save(cr);

  cairo_set_source_rgba(cr, r, g, b, opacity);
  cairo_set_line_width(cr, 0);

  cairo_move_to (cr, x, y);
  cairo_rel_line_to (cr, w, 0);
  cairo_rel_line_to (cr, 0, h);
  cairo_rel_line_to (cr, w/4, 0);
  cairo_rel_line_to (cr, -3*w/4, w);
  cairo_rel_line_to (cr, -3*w/4, -w);
  cairo_rel_line_to (cr, w/4, 0);
  cairo_rel_line_to (cr, 0, -h);

  cairo_close_path (cr);

  cairo_stroke_preserve(cr);

  cairo_fill(cr);

  cairo_restore(cr);

}

void krad_vector_render_viper (cairo_t * cr, int x, int y, int size, int direction) {

  cairo_save(cr);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);

  cairo_set_line_width(cr, size/7);
  cairo_set_source_rgb(cr, WHITE);
  cairo_translate (cr, x, y);

  cairo_arc (cr, 0.0, 0.0, size/2, 0, 2*M_PI);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, GREEN);

  cairo_arc (cr, 0.0, -size/5, size/14, 0, 2*M_PI);
  cairo_stroke(cr);

  cairo_rotate (cr, 40 * (M_PI/180.0));
  cairo_arc (cr, size/3.8, 0.0, size/14, 0, 2*M_PI);
  cairo_stroke(cr);
  cairo_move_to(cr, size/1.8,  0.0);
  cairo_rel_line_to (cr, size/2.3, 0.0);
  cairo_stroke(cr);

  cairo_rotate (cr, 100 * (M_PI/180.0));
  cairo_arc (cr, size/3.8, 0.0, size/14, 0, 2*M_PI);
  cairo_stroke(cr);
  cairo_move_to(cr, size/1.8,  0.0);
  cairo_rel_line_to (cr, size/2.3, 0.0);
  cairo_stroke(cr);

  cairo_rotate (cr, 130 * (M_PI/180.0));
  cairo_move_to(cr, size/1.8,  0.0);
  cairo_rel_line_to (cr, size/2.3, 0.0);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, WHITE);
  cairo_rotate (cr, (180 + direction - 90) * (M_PI/180.0));
  cairo_move_to(cr, size/1.6,  size/6);
  cairo_rel_line_to (cr, size/6, -size/6);
  cairo_rel_line_to (cr, -size/6, -size/6);
  //cairo_rel_line_to(cr, size/1.6,  size/6);
  cairo_fill(cr);

  cairo_set_line_width(cr, size/7);
  cairo_rotate (cr, 180 * (M_PI/180.0));
  cairo_move_to(cr, size/1.6, 0.0);
  cairo_rel_line_to (cr, size/8, 0.0);
  cairo_stroke(cr);

  cairo_restore(cr);

}

void krad_vector_render_clock (cairo_t *cr, int x, int y, int width, int height, float opacity) {

  double m_line_width = 0.051;
  double m_radius = 0.42;
  int i;

  cairo_translate(cr, x, y);

  // scale to unit square and translate (0, 0) to be (0.5, 0.5), i.e.
  // the center of the window

  cairo_scale(cr, width, height);
  cairo_translate(cr, 0.5, 0.5);
  cairo_set_line_width(cr, m_line_width);
  cairo_set_source_rgba(cr,  0.117, 0.337, 0.612, opacity);
 // cairo_save(cr) ;
 // cairo_set_source_rgba(cr, 0.337, 0.612, 0.117, 0.9*opacity);   // green
 // cairo_paint(cr) ;
 // cairo_restore(cr) ;
  cairo_arc(cr, 0, 0, m_radius, 0, 2 * M_PI);
  cairo_save(cr) ;
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8*opacity);
  cairo_fill_preserve(cr) ;
  cairo_restore(cr) ;
  cairo_stroke_preserve(cr) ;
  cairo_clip(cr) ;

  //clock ticks
  for (i = 0; i < 12; i++)
  {
    double inset = 0.05;

    cairo_save(cr) ;
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    if(i % 3 != 0)
    {
      inset *= 0.8;
      cairo_set_line_width(cr, 0.03);
    }

    cairo_move_to(cr,
      (m_radius - inset) * cos (i * M_PI / 6),
      (m_radius - inset) * sin (i * M_PI / 6));
    cairo_line_to (cr,
      m_radius * cos (i * M_PI / 6),
      m_radius * sin (i * M_PI / 6));
    cairo_stroke(cr);
    cairo_restore(cr);
  }

  // store the current time
  time_t rawtime;
  time(&rawtime);
  struct tm * timeinfo = localtime (&rawtime);

  // compute the angles of the indicators of our clock
  double minutes = timeinfo->tm_min * M_PI / 30;
  double hours = timeinfo->tm_hour * M_PI / 6;
  double seconds= timeinfo->tm_sec * M_PI / 30;

  cairo_save(cr) ;
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  // draw the seconds hand
  cairo_save(cr) ;
  cairo_set_line_width(cr, m_line_width / 3);
  cairo_set_source_rgba(cr, 0.612, 0.117,0.117, 0.8*opacity); // gray
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, sin(seconds) * (m_radius * 0.9),
    -cos(seconds) * (m_radius * 0.9));
  cairo_stroke(cr) ;
  cairo_restore(cr) ;

  // draw the minutes hand
  cairo_set_source_rgba(cr, 0.117, 0.337, 0.612, 0.9*opacity);   // blue
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, sin(minutes + seconds / 60) * (m_radius * 0.8),
    -cos(minutes + seconds / 60) * (m_radius * 0.8));
  cairo_stroke(cr) ;

  // draw the hours hand
  cairo_set_source_rgba(cr, 0.117, 0.337, 0.612, 0.9*opacity);   // green
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, sin(hours + minutes / 12.0) * (m_radius * 0.5),
    -cos(hours + minutes / 12.0) * (m_radius * 0.5));
  cairo_stroke(cr) ;
  cairo_restore(cr) ;

  // draw a little dot in the middle
  cairo_arc(cr, 0, 0, m_line_width / 3.0, 0, 2 * M_PI);
  cairo_fill(cr) ;


}
*/
