#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xatom.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>
#include <xcb/shm.h>

#include <GL/glx.h>
#include <GL/gl.h>

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
    xcb_button_press_event_t *press;
    
	xcb_atom_t wm_protocols;
	xcb_atom_t wm_delete_window;
    xcb_atom_t wm_state;
    xcb_atom_t wm_state_above;
    xcb_atom_t wm_state_below;
    xcb_atom_t wm_state_fullscreen;
    
	GLXDrawable drawable;
	GLXContext context;
	GLXWindow glx_window;

	int pixels_size;
	unsigned char *pixels;
	

	uint64_t frames;
	
	int width;
	int height;

	int screen_width;
	int screen_height;
	int fullscreen;

	int close_window;
	char window_title[512];
	int window_open;

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

void krad_x11_test_capture(krad_x11_t *krad_x11);
int krad_x11_capture(krad_x11_t *krad_x11, unsigned char *buffer);
void krad_x11_disable_capture(krad_x11_t *krad_x11);
void krad_x11_enable_capture(krad_x11_t *krad_x11, int width, int height);

krad_x11_t *krad_x11_create_glx_window (krad_x11_t *krad_x11, char *title, int width, int height, int *closed);
void krad_x11_destroy_glx_window (krad_x11_t *krad_x11);

void krad_x11_glx_check_for_input (krad_x11_t *krad_x11);
void krad_x11_glx_wait (krad_x11_t *krad_x11);
void krad_x11_glx_wait_or_poll (krad_x11_t *krad_x11, int wait);

void krad_x11_glx_sync (krad_x11_t *krad_x11);
void krad_x11_glx_render (krad_x11_t *krad_x11);
