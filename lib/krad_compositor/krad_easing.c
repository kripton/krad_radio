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

float krad_ease (krad_ease_t easing, float time_now, float start_pos, float change_amt, float duration) {

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
		default:
			return change_amt*time_now/duration + start_pos;
	}
}
