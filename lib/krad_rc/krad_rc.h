#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include "krad_rc_pololu_maestro.h"

typedef enum {
	UNKNOWN = 7000,
	POLOLUMAESTRO,
} krad_rc_type_t;


typedef struct krad_rc_St krad_rc_t;

struct krad_rc_St {

	krad_rc_type_t type;
	char device[512];
	void *actual;

};



void krad_rc_destroy (krad_rc_t *krad_rc);
void krad_rc_command (krad_rc_t *krad_rc, void *command);
void krad_rc_safe (krad_rc_t *krad_rc);
krad_rc_t *krad_rc_create (char *type, char *device);
