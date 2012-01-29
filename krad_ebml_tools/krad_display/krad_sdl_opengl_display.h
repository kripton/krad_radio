#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <SDL/SDL.h>

#define GL_GLEXT_PROTOTYPES

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "libswscale/swscale.h"

typedef struct krad_sdl_opengl_display_St krad_sdl_opengl_display_t;

struct krad_sdl_opengl_display_St {

	int width;
	int height;

	int videowidth;
	int videoheight;
	
	Uint32 video_flags;

	SDL_Surface *screen;


	struct SwsContext *sws_context;
	int frame_byte_size;
	unsigned char *rgb_frame_data;
	
	pthread_rwlock_t frame_lock;
	
	
	
	int hud_width;
	int hud_height;
	unsigned char *hud_data;

};


// PIX_FMT_YUYV422 PIX_FMT_YUYV420 PIX_FMT_YUV444P
void krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display_t *krad_sdl_opengl_display, enum PixelFormat format);

void krad_sdl_opengl_display_destroy(krad_sdl_opengl_display_t *krad_sdl_opengl_display);
void krad_sdl_opengl_display_test(krad_sdl_opengl_display_t *krad_sdl_opengl_display);
krad_sdl_opengl_display_t *krad_sdl_opengl_display_create(int width, int height, int videowidth, int videoheight);
void krad_sdl_opengl_display_render(krad_sdl_opengl_display_t *krad_sdl_opengl_display, unsigned char *y, int ys, unsigned char *u, int us, unsigned char *v, int vs);
void krad_sdl_opengl_draw_screen(krad_sdl_opengl_display_t *krad_sdl_opengl_display);

void krad_sdl_opengl_read_screen(krad_sdl_opengl_display_t *krad_sdl_opengl_display, void *buffer);
