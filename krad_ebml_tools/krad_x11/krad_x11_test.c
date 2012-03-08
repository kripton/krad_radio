#include "krad_x11.h"




int main () {

	krad_x11_t *krad_x11;
	
	krad_x11 = krad_x11_create();

	krad_x11_test_capture(krad_x11);
	
	krad_x11_create_glx_window (krad_x11);

	krad_x11_destroy(krad_x11);

	
	return 0;
	
}
