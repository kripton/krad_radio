#include "krad_wayland.h"

int krad_wayland_test_frame (void *buffer, void *pointer) {

	int updated;
	
	updated = 1;
	
	
	return updated;
}

int main (int argc, char *argv[]) {

	krad_wayland_t *krad_wayland;
	
	
	krad_wayland = krad_wayland_create ();
	
	krad_wayland_set_frame_callback (krad_wayland, krad_wayland_test_frame, NULL);	
	
	krad_wayland_run (krad_wayland);
	
	krad_wayland_destroy (krad_wayland);	


	return 0;

}
