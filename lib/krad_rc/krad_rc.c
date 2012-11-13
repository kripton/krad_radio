#include "krad_rc.h"


void krad_rc_destroy (krad_rc_t *krad_rc) {

	if (krad_rc->actual != NULL) {
		if (krad_rc->type == POLOLUMAESTRO) {
			krad_rc_pololu_maestro_destroy (krad_rc->actual);
			krad_rc->actual = NULL;
		}
	}

	free (krad_rc);
}


void krad_rc_command (krad_rc_t *krad_rc, void *command) {

	if (krad_rc->actual != NULL) {
		if (krad_rc->type == POLOLUMAESTRO) {
			krad_rc_pololu_maestro_command (krad_rc->actual, command);
		}
	}
}

void krad_rc_safe (krad_rc_t *krad_rc) {

	if (krad_rc->actual != NULL) {
		if (krad_rc->type == POLOLUMAESTRO) {
			krad_rc_pololu_maestro_safe (krad_rc->actual);
		}
	}
}


krad_rc_t *krad_rc_create (char *type, char *device) {

	krad_rc_t *krad_rc = calloc (1, sizeof(krad_rc_t));

	if (strcmp(type, "Pololu Maestro") == 0) {
	
		krad_rc->type = POLOLUMAESTRO;
		strcpy (krad_rc->device, device);
	
		krad_rc->actual = krad_rc_pololu_maestro_create (krad_rc->device);
	
		if (krad_rc->actual == NULL) {
			krad_rc_destroy (krad_rc);
		 	return NULL;
		}
		
	} else {
		krad_rc_destroy (krad_rc);
		return NULL;
	}
	
	krad_rc_safe (krad_rc);
	
	return krad_rc;
}
