#include "krad_wayland.h"



void krad_wayland_destroy(krad_wayland_t *krad_wayland) {

	free(krad_wayland);

}

krad_wayland_t *krad_wayland_create() {

	krad_wayland_t *krad_wayland = calloc(1, sizeof(krad_wayland_t));


	
	return krad_wayland;

}
