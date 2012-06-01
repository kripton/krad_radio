#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "krad_x11.h"
#include "krad_gui.h"
#include "krad_system.h"



int test (char *title) {

	krad_x11_t *krad_x11;
	kradgui_t *kradgui;

	int count;
	int w;
	int h;
	
	w = 1280;
	h = 720;
	
	count = 0;
	
	usleep (500000);
	
	kradgui = kradgui_create_with_internal_surface (w, h);		
	krad_x11 = krad_x11_create ();
	usleep (500000);
	
	//krad_x11_test_capture(krad_x11);

	kradgui_test_screen (kradgui, title);
	
	krad_x11_create_glx_window (krad_x11, title, w, h, NULL);
	
	while ((count < 120) && (krad_x11->close_window == 0)) {
		//krad_x11_capture(krad_x11, NULL);

		kradgui_render (kradgui);
		memcpy (krad_x11->pixels, kradgui->data, w * h * 4);
		
		krad_x11_glx_render (krad_x11);
		count++;
		//printf("count is %d\n", count);
		usleep (50000);
	}
	
	krad_x11_destroy_glx_window (krad_x11);

	krad_x11_destroy (krad_x11);
	kradgui_destroy (kradgui);
	
	return 0;
	
}


int main () {
	
	int count;
	char title[64];
	
	krad_system_init ();
	krad_system_daemonize ();	
	
	count = 1;	
	
	while (count < 4) {
	
		sprintf (title, "test number %d", count);
	
		test (title);
	
		usleep (1000000);
	
		count++;
	
	}
	
	return 0;
	
}
