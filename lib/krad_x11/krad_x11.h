#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>

#ifdef KRAD_USE_X11

#include <X11/Xlib-xcb.h>
//#include <X11/Xatom.h>
#include <xcb/xcb.h>
//#include <xcb/xcb_atom.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>
#include <xcb/shm.h>

#endif


#include <cairo.h>

#include "krad_system.h"

#define KRAD_X11_XCB_ONLY 0


typedef struct krad_x11_St krad_x11_t;

struct krad_x11_St {

	int screen_number;
	int pixels_size;
	unsigned char *pixels;

	uint64_t frames;
	
	int width;
	int height;

	int screen_width;
	int screen_height;
	int fullscreen;

	int *krad_x11_shutdown;

	// capture stuff
#ifdef KRAD_USE_X11	
	Display *display;
	xcb_screen_t *screen;
	xcb_screen_iterator_t iter;	
	xcb_connection_t *connection;	
	xcb_shm_segment_info_t shminfo;
	xcb_shm_get_image_cookie_t cookie;
	xcb_shm_get_image_reply_t *reply;
	xcb_image_t *img;
#endif
	
	uint8_t screen_bit_depth;
	int number;
	int capture_enabled;
	
	// mouse pointer location
	// in window
	
	int mouse_x;
	int mouse_y;
	int mouse_clicked;

};




krad_x11_t *krad_x11_create();
void krad_x11_destroy(krad_x11_t *krad_x11);


int krad_x11_capture(krad_x11_t *krad_x11, unsigned char *buffer);
void krad_x11_disable_capture(krad_x11_t *krad_x11);
void krad_x11_enable_capture(krad_x11_t *krad_x11, int width, int height);

