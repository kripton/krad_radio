#ifndef KRAD_EASING_H
#define KRAD_EASING_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>


typedef enum {
	LINEAR = 33999,
	FIRSTEASING = LINEAR,
	EASEINSINE,
	EASEOUTSINE,
	EASEINOUTSINE,	
	EASEINCUBIC,
	EASEOUTCUBIC,
	EASEINOUTCUBIC,
  EASEINOUTELASTIC,
	LASTEASING = EASEINOUTELASTIC,
/*	EASEINCIRC,
	EASEOUTCIRC,
	EASEINOUTCIRC,
	EASEINBACK,
	EASEOUTBACK,
	EASEINOUTBACK,
	EASEINBOUNCE,
	EASEOUTBOUNCE,
	EASEINOUTBOUNCE,
	EASEINEXPO,
	EASEOUTEXPO,
	EASEINOUTEXPO,
	EASEINELASTIC,
	EASEOUTELASTIC,
	EASEINOUTELASTIC,
	EASEINQUAD,
	EASEOUTQUAD,
	EASEINOUTQUAD,
	EASEINQUART,
	EASEOUTQUART,
	EASEINOUTQUART,
	EASEINQUINT,
	EASEOUTQUINT,
	EASEINOUTQUINT,
*/
} krad_ease_t;

typedef struct krad_easing_St krad_easing_t;

struct krad_easing_St {
  int updating;
  int update;
  int active;
  float target;
  float new_target;
  float start_value;
  float change_amount;
  int elapsed_time;
  int duration;
  int new_duration;
  krad_ease_t krad_ease;
  krad_ease_t new_krad_ease;
};

krad_ease_t krad_ease_random ();
float krad_ease (krad_ease_t easing, float time_now, float start_pos, float change_amt, float duration);
void krad_easing_set_new_value (krad_easing_t *krad_easing, float target, int duration, krad_ease_t krad_ease);
float krad_easing_process(krad_easing_t *krad_easing, float current);
void krad_easing_destroy (krad_easing_t *krad_easing);
krad_easing_t *krad_easing_create ();

#endif
