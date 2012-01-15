#include <krad_dirac.h>
#include <krad_sdl_opengl_display.h>

#define TEST_COUNT1 1
#define TEST_COUNT2 1

void dirac_encode_test() {

	krad_dirac_t *krad_dirac;

	int count;
	
	for (count = 0; count < TEST_COUNT1; count++) {
		krad_dirac = krad_dirac_encoder_create(1280, 720);
		krad_dirac_encode (krad_dirac);
		krad_dirac_encoder_destroy(krad_dirac);
	}

}
	
void dirac_decode_display_test() {
	
	krad_dirac_t *krad_dirac;
	krad_sdl_opengl_display_t *krad_opengl_display;

	char filename[512];

	//char *filename = "/home/oneman/Downloads/bar.drc";
	strcpy(filename, "/home/oneman/Videos/dirac.kw.bbc.co.uk/dirac.kw.bbc.co.uk/download/vc2-ref-streams-20090707/real-pictures/");
	//strcat(filename, "rp-bvfi9-cs-lvl3-d13-noac-nocb-pcmp-cr422-10b.drc");
	strcat(filename, "rp-bvfi13-cs-lvl3-d13-noac-nocb-pcmp-cr422-10b.drc");

	krad_dirac = krad_dirac_decoder_create();


	krad_dirac_test_video_open (krad_dirac, filename);

	krad_opengl_display = krad_sdl_opengl_display_create(krad_dirac->width, krad_dirac->height, krad_dirac->width, krad_dirac->height);


	switch (krad_dirac->format->chroma_format) {
	
		case SCHRO_CHROMA_444:
			krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV444P);
		case SCHRO_CHROMA_422:
			krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV422P);
		case SCHRO_CHROMA_420:
			break;
			// default
			//krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display, PIX_FMT_YUV420P);

	}

	while (1) {

		krad_dirac_decode(krad_dirac);
		
		if (krad_dirac->frame == NULL) { 
			break;
		}
		
		krad_sdl_opengl_display_render(krad_opengl_display, krad_dirac->frame->components[0].data, krad_dirac->frame->components[0].stride, krad_dirac->frame->components[1].data, krad_dirac->frame->components[1].stride, krad_dirac->frame->components[2].data, krad_dirac->frame->components[2].stride);
		krad_sdl_opengl_draw_screen( krad_opengl_display );
		
		usleep(2000000);
	}

	krad_sdl_opengl_display_destroy(krad_opengl_display);
	krad_dirac_decoder_destroy(krad_dirac);

}

int main (int argc, char *argv[]) {

	int count;
	
	for (count = 0; count < TEST_COUNT2; count++) {
		//dirac_encode_test();
		dirac_decode_display_test();
	}

	return 0;

}
