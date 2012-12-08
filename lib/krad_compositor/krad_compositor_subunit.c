#include "krad_compositor_subunit.h"

krad_compositor_subunit_t *krad_compositor_subunit_create () {

	krad_compositor_subunit_t *krad_compositor_subunit;

	if ((krad_compositor_subunit = calloc (1, sizeof (krad_compositor_subunit_t))) == NULL) {
		failfast ("Krad compositor_subunit mem alloc fail");
	}
	
	krad_compositor_subunit_reset (krad_compositor_subunit);
	
	return krad_compositor_subunit;

}

void krad_compositor_subunit_reset (krad_compositor_subunit_t *krad_compositor_subunit) {
	
	
	krad_compositor_subunit->width = 0;
	krad_compositor_subunit->height = 0;
	krad_compositor_subunit->x = 0;
	krad_compositor_subunit->y = 0;
  krad_compositor_subunit->z = 0;
  
	krad_compositor_subunit->tickrate = KRAD_COMPOSITOR_SUBUNIT_DEFAULT_TICKRATE;
	krad_compositor_subunit->tick = 0;

	krad_compositor_subunit->xscale = 1.0f;
	krad_compositor_subunit->yscale = 1.0f;
	krad_compositor_subunit->opacity = 1.0f;
	krad_compositor_subunit->rotation = 0.0f;
	
	
	krad_compositor_subunit->new_x = krad_compositor_subunit->x;
	krad_compositor_subunit->new_y = krad_compositor_subunit->y;
	
	krad_compositor_subunit->new_xscale = krad_compositor_subunit->xscale;
	krad_compositor_subunit->new_yscale = krad_compositor_subunit->yscale;
	krad_compositor_subunit->new_opacity = krad_compositor_subunit->opacity;
	krad_compositor_subunit->new_rotation = krad_compositor_subunit->rotation;

	krad_compositor_subunit->start_x = krad_compositor_subunit->x;
	krad_compositor_subunit->start_y = krad_compositor_subunit->y;
	krad_compositor_subunit->change_x_amount = 0;
	krad_compositor_subunit->change_y_amount = 0;	
	krad_compositor_subunit->x_time = 0;
	krad_compositor_subunit->y_time = 0;
	
	krad_compositor_subunit->x_duration = 0;
	krad_compositor_subunit->y_duration = 0;

	krad_compositor_subunit->start_rotation = krad_compositor_subunit->rotation;
	krad_compositor_subunit->start_opacity = krad_compositor_subunit->opacity;
	krad_compositor_subunit->start_xscale = krad_compositor_subunit->xscale;
	krad_compositor_subunit->start_yscale = krad_compositor_subunit->yscale;	
	
	krad_compositor_subunit->rotation_change_amount = 0;
	krad_compositor_subunit->opacity_change_amount = 0;
	krad_compositor_subunit->xscale_change_amount = 0;
	krad_compositor_subunit->yscale_change_amount = 0;
	
	krad_compositor_subunit->rotation_time = 0;
	krad_compositor_subunit->opacity_time = 0;
	krad_compositor_subunit->xscale_time = 0;
	krad_compositor_subunit->yscale_time = 0;
	
	krad_compositor_subunit->rotation_duration = 0;
	krad_compositor_subunit->opacity_duration = 0;
	krad_compositor_subunit->xscale_duration = 0;
	krad_compositor_subunit->yscale_duration = 0;	
	
	krad_compositor_subunit->krad_ease_x = krad_ease_random();
	krad_compositor_subunit->krad_ease_y = krad_ease_random();
	krad_compositor_subunit->krad_ease_xscale = krad_ease_random();
	krad_compositor_subunit->krad_ease_yscale = krad_ease_random();
	krad_compositor_subunit->krad_ease_rotation = krad_ease_random();
	krad_compositor_subunit->krad_ease_opacity = krad_ease_random();
	
}

void krad_compositor_subunit_destroy (krad_compositor_subunit_t *krad_compositor_subunit) {
	
	krad_compositor_subunit_reset (krad_compositor_subunit);
	free (krad_compositor_subunit);

}

void krad_compositor_subunit_set_xy (krad_compositor_subunit_t *krad_compositor_subunit, int x, int y) {

	krad_compositor_subunit->x = x;
	krad_compositor_subunit->y = y;
	
	krad_compositor_subunit->new_x = krad_compositor_subunit->x;
	krad_compositor_subunit->new_y = krad_compositor_subunit->y;
	
}

void krad_compositor_subunit_set_z (krad_compositor_subunit_t *krad_compositor_subunit, int z) {

  krad_compositor_subunit->z = z;

}

void krad_compositor_subunit_set_new_xy (krad_compositor_subunit_t *krad_compositor_subunit, int x, int y) {
	krad_compositor_subunit->x_duration = 60;
	krad_compositor_subunit->x_time = 0;
	krad_compositor_subunit->y_duration = 60;
	krad_compositor_subunit->y_time = 0;
	krad_compositor_subunit->start_x = krad_compositor_subunit->x;
	krad_compositor_subunit->start_y = krad_compositor_subunit->y;
	krad_compositor_subunit->change_x_amount = x - krad_compositor_subunit->start_x;
	krad_compositor_subunit->change_y_amount = y - krad_compositor_subunit->start_y;
	krad_compositor_subunit->krad_ease_x = EASEINOUTSINE;
	krad_compositor_subunit->krad_ease_y = EASEINOUTSINE;		
	krad_compositor_subunit->new_x = x;
	krad_compositor_subunit->new_y = y;
}

void krad_compositor_subunit_set_scale (krad_compositor_subunit_t *krad_compositor_subunit, float scale) {
  krad_compositor_subunit->xscale = scale;
	krad_compositor_subunit->yscale = scale;
	krad_compositor_subunit->new_xscale = krad_compositor_subunit->xscale;
	krad_compositor_subunit->new_yscale = krad_compositor_subunit->yscale;
}

void krad_compositor_subunit_set_xscale (krad_compositor_subunit_t *krad_compositor_subunit, float scale) {
  krad_compositor_subunit->xscale = scale;
  krad_compositor_subunit->new_xscale = krad_compositor_subunit->xscale;
}

void krad_compositor_subunit_set_yscale (krad_compositor_subunit_t *krad_compositor_subunit, float scale) {
  krad_compositor_subunit->yscale = scale;
  krad_compositor_subunit->new_yscale = krad_compositor_subunit->yscale;
}

void krad_compositor_subunit_set_opacity (krad_compositor_subunit_t *krad_compositor_subunit, float opacity) {
  krad_compositor_subunit->opacity = opacity;
  krad_compositor_subunit->new_opacity = krad_compositor_subunit->opacity;
}

void krad_compositor_subunit_set_rotation (krad_compositor_subunit_t *krad_compositor_subunit, float rotation) {
  krad_compositor_subunit->rotation = rotation;
  krad_compositor_subunit->new_rotation = krad_compositor_subunit->rotation;
}

void krad_compositor_subunit_set_tickrate (krad_compositor_subunit_t *krad_compositor_subunit, int tickrate) {
  krad_compositor_subunit->tickrate = tickrate;
}

void krad_compositor_subunit_set_new_scale (krad_compositor_subunit_t *krad_compositor_subunit, float scale) {
	krad_compositor_subunit->xscale_duration = rand() % 100 + 10;
	krad_compositor_subunit->xscale_time = 0;
	krad_compositor_subunit->yscale_duration = rand() % 100 + 10;
	krad_compositor_subunit->yscale_time = 0;	
	krad_compositor_subunit->start_xscale = krad_compositor_subunit->xscale;
	krad_compositor_subunit->start_yscale = krad_compositor_subunit->yscale;
	krad_compositor_subunit->xscale_change_amount = scale - krad_compositor_subunit->start_xscale;
	krad_compositor_subunit->yscale_change_amount = scale - krad_compositor_subunit->start_yscale;
	krad_compositor_subunit->krad_ease_xscale = krad_ease_random();
	krad_compositor_subunit->krad_ease_yscale = krad_ease_random();		
	krad_compositor_subunit->new_xscale = scale;
	krad_compositor_subunit->new_yscale = scale;	
}

void krad_compositor_subunit_set_new_xscale (krad_compositor_subunit_t *krad_compositor_subunit, float scale) {
	krad_compositor_subunit->xscale_duration = rand() % 100 + 10;
	krad_compositor_subunit->xscale_time = 0;
	krad_compositor_subunit->start_xscale = krad_compositor_subunit->xscale;
	krad_compositor_subunit->xscale_change_amount = scale - krad_compositor_subunit->start_xscale;
	krad_compositor_subunit->krad_ease_xscale = krad_ease_random();		
	krad_compositor_subunit->new_xscale = scale;
}

void krad_compositor_subunit_set_new_yscale (krad_compositor_subunit_t *krad_compositor_subunit, float scale) {
	krad_compositor_subunit->yscale_duration = rand() % 100 + 10;
	krad_compositor_subunit->yscale_time = 0;
	krad_compositor_subunit->start_yscale = krad_compositor_subunit->yscale;
	krad_compositor_subunit->yscale_change_amount = scale - krad_compositor_subunit->start_yscale;
	krad_compositor_subunit->krad_ease_yscale = krad_ease_random();	
	krad_compositor_subunit->new_yscale = scale;	
}

void krad_compositor_subunit_set_new_opacity (krad_compositor_subunit_t *krad_compositor_subunit, float opacity) {
	krad_compositor_subunit->opacity_duration = 60;
	krad_compositor_subunit->opacity_time = 0;
	krad_compositor_subunit->start_opacity = krad_compositor_subunit->opacity;
	krad_compositor_subunit->opacity_change_amount = opacity - krad_compositor_subunit->start_opacity;
	krad_compositor_subunit->krad_ease_opacity = EASEINOUTSINE;	
	krad_compositor_subunit->new_opacity = opacity;
}

void krad_compositor_subunit_set_new_rotation (krad_compositor_subunit_t *krad_compositor_subunit, float rotation) {
	krad_compositor_subunit->rotation_duration = 60;
	krad_compositor_subunit->rotation_time = 0;
	krad_compositor_subunit->start_rotation = krad_compositor_subunit->rotation;
	krad_compositor_subunit->rotation_change_amount = rotation - krad_compositor_subunit->start_rotation;
	krad_compositor_subunit->krad_ease_rotation = EASEINOUTSINE;
	krad_compositor_subunit->new_rotation = rotation;
}


void krad_compositor_subunit_update (krad_compositor_subunit_t *krad_compositor_subunit) {

	if (krad_compositor_subunit->x_time != krad_compositor_subunit->x_duration) {
		krad_compositor_subunit->x = krad_ease (krad_compositor_subunit->krad_ease_x, krad_compositor_subunit->x_time++, krad_compositor_subunit->start_x, krad_compositor_subunit->change_x_amount, krad_compositor_subunit->x_duration);

    if (krad_compositor_subunit->x_time == krad_compositor_subunit->x_duration) {
      krad_compositor_subunit->x = krad_compositor_subunit->new_x;
    }
  }	
	
	if (krad_compositor_subunit->y_time != krad_compositor_subunit->y_duration) {
		krad_compositor_subunit->y = krad_ease (krad_compositor_subunit->krad_ease_y, krad_compositor_subunit->y_time++, krad_compositor_subunit->start_y, krad_compositor_subunit->change_y_amount, krad_compositor_subunit->y_duration);
    if (krad_compositor_subunit->y_time == krad_compositor_subunit->y_duration) {
      krad_compositor_subunit->y = krad_compositor_subunit->new_y;
    }
  }
	
	if (krad_compositor_subunit->rotation_time != krad_compositor_subunit->rotation_duration) {
		krad_compositor_subunit->rotation = krad_ease (krad_compositor_subunit->krad_ease_rotation, krad_compositor_subunit->rotation_time++, krad_compositor_subunit->start_rotation, krad_compositor_subunit->rotation_change_amount, krad_compositor_subunit->rotation_duration);
    if (krad_compositor_subunit->rotation_time == krad_compositor_subunit->rotation_duration) {
      krad_compositor_subunit->rotation = krad_compositor_subunit->new_rotation;
    }
  }
	
	if (krad_compositor_subunit->opacity_time != krad_compositor_subunit->opacity_duration) {
		krad_compositor_subunit->opacity = krad_ease (krad_compositor_subunit->krad_ease_opacity, krad_compositor_subunit->opacity_time++, krad_compositor_subunit->start_opacity, krad_compositor_subunit->opacity_change_amount, krad_compositor_subunit->opacity_duration);
    if (krad_compositor_subunit->opacity_time == krad_compositor_subunit->opacity_duration) {
      krad_compositor_subunit->opacity = krad_compositor_subunit->new_opacity;
    }
  }
	
	if (krad_compositor_subunit->xscale_time != krad_compositor_subunit->xscale_duration) {
		krad_compositor_subunit->xscale = krad_ease (krad_compositor_subunit->krad_ease_xscale, krad_compositor_subunit->xscale_time++, krad_compositor_subunit->start_xscale, krad_compositor_subunit->xscale_change_amount, krad_compositor_subunit->xscale_duration);
    if (krad_compositor_subunit->xscale_time == krad_compositor_subunit->xscale_duration) {
      krad_compositor_subunit->xscale = krad_compositor_subunit->new_xscale;
    }
  }
	
	if (krad_compositor_subunit->yscale_time != krad_compositor_subunit->yscale_duration) {
		krad_compositor_subunit->yscale = krad_ease (krad_compositor_subunit->krad_ease_yscale, krad_compositor_subunit->yscale_time++, krad_compositor_subunit->start_yscale, krad_compositor_subunit->yscale_change_amount, krad_compositor_subunit->yscale_duration);
    if (krad_compositor_subunit->yscale_time == krad_compositor_subunit->yscale_duration) {
      krad_compositor_subunit->yscale = krad_compositor_subunit->new_yscale;
    }
  }
}


