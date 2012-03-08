#define _XOPEN_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_image.h>
#include <xcb/shm.h>

#include <GL/glx.h>
#include <GL/gl.h>

#include <cairo.h>

#define KRAD_X11_XCB_ONLY 0


typedef struct krad_x11_St krad_x11_t;

struct krad_x11_St {

	Display *display;
	int screen_number;
	xcb_screen_iterator_t iter;
	xcb_screen_t *screen;
	xcb_connection_t *connection;
	xcb_window_t window;
	GLXDrawable drawable;

};




krad_x11_t *krad_x11_create();
void krad_x11_destroy(krad_x11_t *krad_x11);

void krad_x11_test_capture(krad_x11_t *krad_x11);
int krad_x11_create_glx_window (krad_x11_t *krad_x11);
int krad_x11_gl_run (krad_x11_t *krad_x11);
void krad_x11_gl_draw();
