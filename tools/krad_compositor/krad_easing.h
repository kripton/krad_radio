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
	LASTEASING = EASEINOUTCUBIC,
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

	int x;

};

krad_ease_t krad_ease_random ();
float krad_ease (krad_ease_t easing, float time_now, float start_pos, float change_amt, float duration);

void krad_easing_destroy (krad_easing_t *krad_easing);
krad_easing_t *krad_easing_create ();
