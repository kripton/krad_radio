#ifndef KRAD_COMPOSITOR_SUBUNIT_H
#define KRAD_COMPOSITOR_SUBUNIT_H

typedef struct krad_compositor_subunit_St krad_compositor_subunit_t;


typedef enum {
  TEXT,
  SPRITE,
  VECTOR,
  VIDEO_PORT
} compositor_subunit_type_t;

#include "krad_radio.h"

#define KRAD_COMPOSITOR_SUBUNIT_DEFAULT_TICKRATE 4

struct krad_compositor_subunit_St {

  compositor_subunit_type_t type;
  
  int active;
  int number;
  
	int x;
	int y;
	int z;	
	
	int tickrate;
	int tick;
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

krad_compositor_subunit_t *krad_compositor_subunit_create ();
void krad_compositor_subunit_destroy (krad_compositor_subunit_t *krad_compositor_subunit);
void krad_compositor_subunit_reset (krad_compositor_subunit_t *krad_compositor_subunit);

void krad_compositor_subunit_set_xy (krad_compositor_subunit_t *krad_compositor_subunit, int x, int y);
void krad_compositor_subunit_set_new_xy (krad_compositor_subunit_t *krad_compositor_subunit, int x, int y);

void krad_compositor_subunit_set_z (krad_compositor_subunit_t *krad_compositor_subunit, int z);

void krad_compositor_subunit_set_scale (krad_compositor_subunit_t *krad_compositor_subunit, float scale);
void krad_compositor_subunit_set_xscale (krad_compositor_subunit_t *krad_compositor_subunit, float scale);
void krad_compositor_subunit_set_yscale (krad_compositor_subunit_t *krad_compositor_subunit, float scale);
void krad_compositor_subunit_set_opacity (krad_compositor_subunit_t *krad_compositor_subunit, float opacity);
void krad_compositor_subunit_set_rotation (krad_compositor_subunit_t *krad_compositor_subunit, float rotation);

void krad_compositor_subunit_set_tickrate (krad_compositor_subunit_t *krad_compositor_subunit, int tickrate); 
void krad_compositor_subunit_set_new_scale (krad_compositor_subunit_t *krad_compositor_subunit, float scale);
void krad_compositor_subunit_set_new_opacity (krad_compositor_subunit_t *krad_compositor_subunit, float opacity);
void krad_compositor_subunit_set_new_rotation (krad_compositor_subunit_t *krad_compositor_subunit, float rotation);

void krad_compositor_subunit_update (krad_compositor_subunit_t *krad_compositor_subunit);
#endif // KRAD_COMPOSITOR_SUBUNIT_H
