#include "krad_easing.h"

void krad_easing_destroy (krad_easing_t *krad_easing) {
	free (krad_easing);
}

krad_easing_t *krad_easing_create () {

	krad_easing_t *krad_easing = calloc (1, sizeof(krad_easing_t));
	
	return krad_easing;
}

krad_ease_t krad_ease_random () {

	return rand () % (LASTEASING - FIRSTEASING) + FIRSTEASING;

}



float krad_easing_process (krad_easing_t *krad_easing, float current, void **ptr) {

  float value;

  if (krad_easing->update == 1) {
    while (__sync_bool_compare_and_swap( &krad_easing->updating, 0, 1 ));
    krad_easing->krad_ease = krad_easing->new_krad_ease;
    krad_easing->duration = krad_easing->new_duration;
    krad_easing->elapsed_time = 0;
    krad_easing->start_value = current;
    krad_easing->target = krad_easing->new_target;
    krad_easing->change_amount = krad_easing->target - krad_easing->start_value;
    krad_easing->update = 0;
    while (__sync_bool_compare_and_swap( &krad_easing->updating, 1, 0 ));
  }

  if (krad_easing->duration > 0) {
    value = krad_ease (krad_easing->krad_ease,
                       krad_easing->elapsed_time,
                       krad_easing->start_value,
                       krad_easing->change_amount,
                       krad_easing->duration);
    krad_easing->elapsed_time++;
  } else {
    krad_easing->duration = 0;
    krad_easing->elapsed_time = 0;
  }
  
  if (ptr != NULL) {
    *ptr = NULL;
  }
  
  if (krad_easing->elapsed_time == krad_easing->duration) {
    value = krad_easing->target;
    while (__sync_bool_compare_and_swap( &krad_easing->updating, 0, 1 ));
    // FIXME probably should recheck elapsed_time == duration here but
    // ill leave it alone for now since it appears to work..
    if (ptr != NULL) {
      *ptr = krad_easing->ptr;
      //if (krad_easing->ptr != NULL) {
      //  printk ("easing delt it it!\n");
      //}
    }
    krad_easing->ptr = NULL;
    krad_easing->active = 0;
    while (__sync_bool_compare_and_swap( &krad_easing->updating, 1, 0 ));
  }

  return value;
}


void krad_easing_set_new_value (krad_easing_t *krad_easing, float target, int duration, krad_ease_t krad_ease, void *ptr) {
  while (__sync_bool_compare_and_swap( &krad_easing->updating, 0, 1 ));
  krad_easing->ptr = ptr;
  //if (ptr != NULL) {
  //  printk ("easing got it!\n");
  //}
  krad_easing->new_target = target;
  krad_easing->new_duration = duration;
  krad_easing->new_krad_ease = krad_ease;
  krad_easing->update = 1;
  krad_easing->active = 1;
  while (__sync_bool_compare_and_swap( &krad_easing->updating, 1, 0 ));
}

float krad_ease (krad_ease_t easing, float time_now, float start_pos, float change_amt, float duration) {
  
  float s, p, a;
  
	switch (easing) {
		case LINEAR:
			return change_amt*time_now/duration + start_pos;
		case EASEINSINE:
			return -change_amt * cos(time_now/duration * (M_PI/2.0f)) + change_amt + start_pos;
		case EASEOUTSINE:
			return change_amt * sin(time_now/duration * (M_PI/2.0f)) + start_pos;
		case EASEINOUTSINE:
			return -change_amt/2.0f * (cos(M_PI*time_now/duration) - 1.0f) + start_pos;
		case EASEINCUBIC:
			time_now /= duration;
			return change_amt*(time_now)*time_now*time_now + start_pos;
		case EASEOUTCUBIC:
			time_now /= duration;
			time_now -= 1.0f;
			return change_amt*((time_now)*time_now*time_now + 1.0f) + start_pos;
		case EASEINOUTCUBIC:
			time_now /= duration/2.0f;
			if ((time_now) < 1.0f) { 
				return change_amt/2.0f*time_now*time_now*time_now + start_pos;
			} else {
				time_now -= 2.0f;
				return change_amt/2.0f*((time_now)*time_now*time_now + 2.0f) + start_pos;
			}
    case EASEINOUTELASTIC:
      s = 1.70158;
      p = 0;
      a = change_amt;
      if (change_amt == 0) return start_pos;
      if (time_now == 0) return start_pos;
      if (time_now == duration) return start_pos + change_amt;
      time_now /= (duration/2.0f);
      if (time_now >= 2.0) return start_pos + change_amt;
          
      if (p == 0) p = duration * 0.3;
      if (a < fabsf(change_amt)) { 
        a = change_amt;  
        s= p/4; 
      } else {
        s = p/(2.0f*M_PI) * asin (change_amt/a);
      }
      if (time_now < 1) {
        time_now -= 1.0f;
        return -.5f*(a*pow(2,10*time_now) * sin( (time_now*duration-s)*(2.0f*M_PI)/p )) + start_pos;
      }
      time_now -= 1.0f;
      return a*pow(2,-10*time_now) * sin( (time_now*duration-s)*(2.0f*M_PI)/p )*.5f + change_amt + start_pos;
    default:
	    return change_amt*time_now/duration + start_pos;
  }
}
