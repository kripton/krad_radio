#include <stdlib.h>
#include <stdio.h>
#include "SDL.h"
#include "libswscale/swscale.h"
#include <unistd.h>

typedef struct krad_overlay_display_St krad_overlay_display_t;

struct krad_overlay_display_St {

	int width;
	int height;
	int displaywidth;
	int displayheight;

	Uint32 video_flags;
	Uint32 overlay_format;

	SDL_Surface *screen, *pic;
	SDL_Overlay *overlay;
	int scale;
	int monochrome;
	int luminance;

	SDL_Rect rect;

	int colormode;

	struct SwsContext *sws_context;

};

void krad_overlay_display_destroy(krad_overlay_display_t *krad_overlay_display);
void krad_overlay_display_test(krad_overlay_display_t *krad_overlay_display);
krad_overlay_display_t *krad_overlay_display_create(int displaywidth, int displayheight, int width, int height);
void krad_overlay_display_render(krad_overlay_display_t *krad_overlay_display, unsigned char *y, int ys, unsigned char *u, int us, unsigned char *v, int vs);

void krad_overlay_display_render_rgb(krad_overlay_display_t *krad_overlay_display, unsigned char *pRGBData, int src_w, int src_h);
