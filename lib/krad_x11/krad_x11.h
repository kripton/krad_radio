#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>

#include <X11/Xlib-xcb.h>
//#include <X11/Xatom.h>
#include <xcb/xcb.h>
//#include <xcb/xcb_atom.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>
#include <xcb/shm.h>


#include <cairo.h>

#include "krad_system.h"

#define KRAD_X11_XCB_ONLY 0


typedef struct krad_x11_St krad_x11_t;

struct krad_x11_St {

	Display *display;
	int screen_number;

	xcb_screen_iterator_t iter;
	xcb_screen_t *screen;
	xcb_connection_t *connection;
	xcb_colormap_t colormap;
	xcb_window_t window;
    xcb_generic_event_t *event;
//    xcb_button_press_event_t *press;


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
	xcb_shm_segment_info_t shminfo;
	xcb_shm_get_image_cookie_t cookie;
	xcb_shm_get_image_reply_t *reply;
	xcb_image_t *img;
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

