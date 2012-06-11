#include "krad_gui.h"
#include "krad_x11.h"
#include "krad_system.h"

#include "wcap-decode.h"

typedef struct krad_wcap_player_St krad_wcap_player_t;

struct krad_wcap_player_St {

	kradgui_t *krad_gui;
	krad_x11_t *krad_x11;
	struct wcap_decoder *decoder;

	int width;
	int height;

	uint32_t start_time;

	uint32_t frame_num;

	void *buffer;
	
	int shutdown;

};

static void krad_wcap_player_play (char *filename);

int krad_wcap_player_render_callback (void *pointer, uint32_t time) {

	krad_wcap_player_t *krad_wcap_player;
	int updated;
	uint32_t reltime;

	krad_wcap_player = (krad_wcap_player_t *)pointer;
	updated = 0;

	reltime = krad_wcap_player->decoder->msecs - krad_wcap_player->start_time;

	kradgui_update_elapsed_time (krad_wcap_player->krad_gui);

	if (krad_wcap_player->krad_gui->elapsed_time_ms > reltime) {

	//	printf("Elapsed time: %ums Frame time: %ums Frame: %u\r",
	//		krad_wcap_player->krad_gui->elapsed_time_ms,
	//		reltime,
	//		krad_wcap_player->frame_num++);

		fflush (stdout);

		//kradgui_render (krad_wcap_player->krad_gui);
		//kradgui_render_meter ( krad_wcap_player->krad_gui, krad_wcap_player->x, 
		//						 krad_wcap_player->y, krad_wcap_player->size,
		//						 krad_wcap_player->cpu_usage );

//		memcpy (buffer, krad_wcap_player->krad_gui->data, krad_wcap_player->width * krad_wcap_player->height * 4);
		updated = 1;

		if (wcap_decoder_get_frame (krad_wcap_player->decoder)) {

		} else {
			printf ("\n");
			return -1;
		}
	}

	return updated;
}


static void krad_wcap_player_play (char *filename) {

	krad_wcap_player_t *krad_wcap_player;

	krad_wcap_player = calloc (1, sizeof (krad_wcap_player_t));

	krad_wcap_player->decoder = wcap_decoder_create (filename, 1);

	if (krad_wcap_player->decoder == NULL) {
		fprintf (stderr, "Couldn't open file: %s\n", filename);
		free (krad_wcap_player);
		exit (1);
	}

	krad_wcap_player->width = krad_wcap_player->decoder->width;
	krad_wcap_player->height = krad_wcap_player->decoder->height;

	krad_wcap_player->krad_x11 = krad_x11_create ();

	krad_x11_create_glx_window (krad_wcap_player->krad_x11, "Krad WCAP Player", 
								krad_wcap_player->width, krad_wcap_player->height, &krad_wcap_player->shutdown);

	krad_wcap_player->buffer = krad_wcap_player->krad_x11->pixels;

	wcap_decoder_set_external_buffer (krad_wcap_player->decoder, krad_wcap_player->buffer);

	krad_wcap_player->krad_gui = kradgui_create_with_external_surface (krad_wcap_player->width,
																	   krad_wcap_player->height,
													  (unsigned char *)krad_wcap_player->buffer);



	wcap_decoder_get_frame (krad_wcap_player->decoder);

	krad_wcap_player->start_time = krad_wcap_player->decoder->msecs;

	printf ("Krad WCAP Player \n");
	printf ("Playing: %s\n", filename);
	printf ("Size: %dx%d\n", krad_wcap_player->width, krad_wcap_player->height);



	krad_wcap_player->krad_gui->clear = 0;
	//kradgui_go_live (krad_wcap_player->krad_gui);

	//krad_wayland_set_frame_callback (krad_wcap_player->krad_wayland, krad_wcap_player_render_callback, krad_wcap_player);

	//krad_wayland_open_window (krad_wcap_player->krad_wayland);

	while ((krad_wcap_player_render_callback (krad_wcap_player, 0) > -1) &&
		   (krad_wcap_player->shutdown != 1)) {
		krad_x11_glx_render (krad_wcap_player->krad_x11);
	}
	
	usleep (100000);
	
	krad_x11_destroy_glx_window (krad_wcap_player->krad_x11);

	krad_x11_destroy (krad_wcap_player->krad_x11);

	kradgui_destroy (krad_wcap_player->krad_gui);

	wcap_decoder_destroy (krad_wcap_player->decoder);

	free (krad_wcap_player);

}

int main (int argc, char *argv[]) {

	krad_system_init ();

	if (argc == 2) {
		krad_wcap_player_play (argv[1]);
	} else {
		printf ("Need filename\n");
	}

	printf ("Clean Exit\n");

    return 0;
}
