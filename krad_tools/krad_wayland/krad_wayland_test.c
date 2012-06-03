#include "krad_wayland.h"



int main (int argc, char *argv[]) {

	krad_wayland_t *krad_wayland;
	
	
	krad_wayland = krad_wayland_create ();
	
	krad_wayland_run (krad_wayland);
	
	krad_wayland_destroy (krad_wayland);	


	return 0;

}
