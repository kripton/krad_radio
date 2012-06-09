#include "krad_wayland.h"

int krad_wayland_test_frame (void *buffer, void *pointer) {

	int updated;
	
	updated = 1;
	

	

	
	return updated;
}

int main (int argc, char *argv[]) {

	krad_wayland_t *krad_wayland;
	
	int width;
	int height;
	
	width = 960;
	height = 540;

	if (argc == 3) {
		width = atoi(argv[1]);
		height = atoi(argv[2]);
	}

	krad_wayland = krad_wayland_create ();

	krad_wayland->render_test_pattern = 1;

	krad_wayland_set_frame_callback (krad_wayland, krad_wayland_test_frame, NULL);	
	
	krad_wayland_open_window (krad_wayland, width, height);
	
	krad_wayland_destroy (krad_wayland);	


	return 0;

}
