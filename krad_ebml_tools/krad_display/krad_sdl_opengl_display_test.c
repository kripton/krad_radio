#include <krad_sdl_opengl_display.h>


#include "SDL.h"

int main (int argc, char *argv[]) {


	krad_sdl_opengl_display_t *krad_sdl_opengl_display;
	
	int width, height;
	
	width = 1920;
	height = 1080;
	
	krad_sdl_opengl_display = krad_sdl_opengl_display_create(width, height);


	usleep(5000000);


	krad_sdl_opengl_display_destroy(krad_sdl_opengl_display);

	return 0;

}
