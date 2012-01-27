#include <krad_sdl_opengl_display.h>
#include <krad_gui.h>

#include "SDL.h"

#define TEST_COUNT 433


void test1(int times) {


	krad_sdl_opengl_display_t *krad_sdl_opengl_display;
	kradgui_t *kradgui;
	
	int width, height;
	int count;
	int stride;
	int gui_byte_size;
	unsigned char *gui_data;
	
	char *test_info;
	
	width = 1280;
	height = 720;

	width = 1920;
	height = 1080;

	count = 0;
	
	cairo_surface_t *cst;
	cairo_t *cr;
	
	stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);
	gui_byte_size = stride * height;
	gui_data = calloc (1, gui_byte_size);
	
	cst = cairo_image_surface_create_for_data (gui_data, CAIRO_FORMAT_ARGB32, width, height, stride);
	
	krad_sdl_opengl_display = krad_sdl_opengl_display_create(width, height, width, height);

	kradgui = kradgui_create(width, height);

	test_info = "This is a test of the krad opengl display system 1";

	//kradgui_test_screen(kradgui, test_info);
	
	kradgui->update_drawtime = 1;
	kradgui->print_drawtime = 1;
	//kradgui->render_tearbar = 1;
	
	kradgui->render_ftest = 1;
	kradgui_add_item(kradgui, REEL_TO_REEL);
	while (count < times) {

		cr = cairo_create(cst);
		kradgui->cr = cr;
		kradgui_render(kradgui);
		cairo_destroy(cr);
	
		memcpy(krad_sdl_opengl_display->rgb_frame_data, gui_data, gui_byte_size);
	
		//usleep(20000);
	
		krad_sdl_opengl_draw_screen( krad_sdl_opengl_display );

		count++;

		kradgui_add_current_track_time_ms(kradgui, 1000 / 60);

	}

	krad_sdl_opengl_display_destroy(krad_sdl_opengl_display);
	kradgui_destroy(kradgui);
	
	cairo_surface_destroy(cst);
	free(gui_data);
}

void test2(int times) {


	krad_sdl_opengl_display_t *krad_sdl_opengl_display;
	kradgui_t *kradgui;

	SDL_Event event;	

	int width, height;
	int count;
	int shutdown;
	char *test_info;
	
	shutdown = 0;

	width = 1280;
	height = 720;

//	width = 1920;
//	height = 1080;

	count = 0;
	
	krad_sdl_opengl_display = krad_sdl_opengl_display_create(width, height, width, height);

	kradgui = kradgui_create_with_internal_surface(width, height);

	test_info = "This is a test of the krad opengl display system 2";

	//kradgui_test_screen(kradgui, test_info);
	
	//kradgui->update_drawtime = 1;
	//kradgui->print_drawtime = 1;
	//kradgui->render_tearbar = 1;
	
	kradgui->render_ftest = 1;

	while (count < times) {

		kradgui_start_draw_time(kradgui);
	
		kradgui_render(kradgui);

	
		memcpy(krad_sdl_opengl_display->rgb_frame_data, kradgui->data, kradgui->bytes);		

		krad_sdl_opengl_draw_screen( krad_sdl_opengl_display );

		kradgui_end_draw_time(kradgui);
		printf("Frame: %llu :: %s\r", kradgui->frame, kradgui->draw_time_string);
		fflush(stdout);

		count++;

		if (count % 300 == 0) {
			kradgui_update_elapsed_time(kradgui);
			kradgui_update_elapsed_time_timecode_string(kradgui);
			printf("\n\n300 Frames in :: %s\n\n", kradgui->elapsed_time_timecode_string);
			fflush(stdout);
			kradgui_reset_elapsed_time(kradgui);

		}

		usleep(5500);

		while ( SDL_PollEvent( &event ) ){
			switch( event.type ){
				/* Look for a keypress */
				case SDL_KEYDOWN:
				    /* Check the SDLKey values and move change the coords */
				    switch( event.key.keysym.sym ){
				        case SDLK_LEFT:
				            break;
				        case SDLK_RIGHT:
				            break;
				        case SDLK_UP:
				            break;
				        case SDLK_q:
				        	shutdown = 1;
				            break;
				        case SDLK_f:
				        	
				            break;
				        default:
				            break;
				    }
				    break;

				case SDL_KEYUP:
				    switch( event.key.keysym.sym ) {
				        case SDLK_LEFT:
				            break;
				        case SDLK_RIGHT:
				            break;
				        case SDLK_UP:
				            break;
				        case SDLK_DOWN:
				            break;
				        default:
				            break;
				    }
				    break;

				case SDL_MOUSEMOTION:

					kradgui->cursor_x = event.motion.x;
					kradgui->cursor_y = event.motion.y;
				
					//printf("mouse! %d %d\n", kradgui->cursor_x, kradgui->cursor_y);

					break;	
				
				default:
				    break;
			}
		}

		if (shutdown == 1) {
			break;
		}

	
	}

	krad_sdl_opengl_display_destroy(krad_sdl_opengl_display);
	kradgui_destroy(kradgui);

}

int main (int argc, char *argv[]) {

	int times;

	if (argc == 2) {
		times = atoi(argv[1]);
	} else {
		times = TEST_COUNT;
	}


//	test1(times);
	
	test2(times);
	
	return 0;

}
