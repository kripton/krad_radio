#include <krad_sdl_opengl_display.h>
#include <krad_ebml.h>
#include <krad_vpx.h>
#include "SDL.h"

int main (int argc, char *argv[]) {


	krad_sdl_opengl_display_t *krad_opengl_display;
	krad_vpx_decoder_t *krad_vpx_decoder;
	krad_ebml_t *krad_ebml;
	
	int len;
	
	unsigned char *buffer;

	buffer = malloc(1000000);
	krad_vpx_decoder = krad_vpx_decoder_create();
	krad_ebml = kradebml_create();

	kradebml_open_input_file(krad_ebml, argv[1]);
	//kradebml_open_input_stream(krad_ebml_player->ebml, "192.168.1.2", 9080, "/teststream.krado");
	
	kradebml_debug(krad_ebml);

	krad_opengl_display = krad_sdl_opengl_display_create(1920, 1080, krad_ebml->vparams.width, krad_ebml->vparams.height);
	
	while ((len = kradebml_read_video(krad_ebml, buffer)) > 0) {
		

		//printf("got len of %d\n", len);
		krad_vpx_decoder_decode(krad_vpx_decoder, buffer, len);
		if (krad_vpx_decoder->img != NULL) {

			//printf("vpx img: %d %d %d\n", krad_vpx_decoder->img->stride[0],  krad_vpx_decoder->img->stride[1],  krad_vpx_decoder->img->stride[2]); 

			krad_sdl_opengl_display_render(krad_opengl_display, krad_vpx_decoder->img->planes[0], krad_vpx_decoder->img->stride[0], krad_vpx_decoder->img->planes[1], krad_vpx_decoder->img->stride[1], krad_vpx_decoder->img->planes[2], krad_vpx_decoder->img->stride[2]);
			krad_sdl_opengl_draw_screen( krad_opengl_display );
			usleep(13000);


		}
	}

	free(buffer);
	krad_vpx_decoder_destroy(krad_vpx_decoder);
	kradebml_destroy(krad_ebml);
	krad_sdl_opengl_display_destroy(krad_opengl_display);

	return 0;

}
