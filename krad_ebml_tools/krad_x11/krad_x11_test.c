#include "krad_x11.h"




int main () {

	krad_x11_t *krad_x11;
	int count;
	
	count = 0;
	
	krad_x11 = krad_x11_create();

	//krad_x11_test_capture(krad_x11);
	
	krad_x11_create_glx_window (krad_x11, "bongo", 640, 480, NULL);
	
	while ((count < 120) && (krad_x11->close_window == 0)) {
		//krad_x11_glx_wait (krad_x11);
		krad_x11_glx_render (krad_x11);
		count++;
		printf("count is %d\n", count);
	}
	
	krad_x11_destroy_glx_window (krad_x11);

	krad_x11_destroy (krad_x11);

	
	return 0;
	
}
