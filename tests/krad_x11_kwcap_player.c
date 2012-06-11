#include "krad_gui.h"
#include "krad_x11.h"
#include "krad_system.h"

#include "kwcap-decode.h"

typedef struct krad_kwcap_player_St krad_kwcap_player_t;

struct krad_kwcap_player_St {

	kradgui_t *krad_gui;
	krad_x11_t *krad_x11;
	struct kwcap_decoder *decoder;

	int width;
	int height;

	uint32_t start_time;

	uint32_t frame_num;

	void *buffer;
	
	int shutdown;

};

static void krad_kwcap_player_play (char *filename);

int krad_kwcap_player_render_callback (void *pointer, uint32_t time) {

	krad_kwcap_player_t *krad_kwcap_player;
	int updated;
	uint32_t reltime;

	krad_kwcap_player = (krad_kwcap_player_t *)pointer;
	updated = 0;

	reltime = krad_kwcap_player->decoder->msecs - krad_kwcap_player->start_time;

	kradgui_update_elapsed_time (krad_kwcap_player->krad_gui);

	while (krad_kwcap_player->krad_gui->elapsed_time_ms > reltime) {


	reltime = krad_kwcap_player->decoder->msecs - krad_kwcap_player->start_time;

	kradgui_update_elapsed_time (krad_kwcap_player->krad_gui);

	//	printf("Elapsed time: %ums Frame time: %ums Frame: %u\r",
	//		krad_kwcap_player->krad_gui->elapsed_time_ms,
	//		reltime,
	//		krad_kwcap_player->frame_num++);

	//	fflush (stdout);

		//kradgui_render (krad_kwcap_player->krad_gui);
		//kradgui_render_meter ( krad_kwcap_player->krad_gui, krad_kwcap_player->x, 
		//						 krad_kwcap_player->y, krad_kwcap_player->size,
		//						 krad_kwcap_player->cpu_usage );

//		memcpy (buffer, krad_kwcap_player->krad_gui->data, krad_kwcap_player->width * krad_kwcap_player->height * 4);
		updated = 1;

		if (kwcap_decoder_get_frame (krad_kwcap_player->decoder)) {

		} else {
			printf ("\n");
			return -1;
		}
	}

	return updated;
}


static void krad_kwcap_player_play (char *filename) {

	krad_kwcap_player_t *krad_kwcap_player;

	krad_kwcap_player = calloc (1, sizeof (krad_kwcap_player_t));

	krad_kwcap_player->decoder = kwcap_decoder_create (filename, 1);


	krad_kwcap_player->decoder = kwcap_decoder_listen (12000, 1);

	if (krad_kwcap_player->decoder == NULL) {
		fprintf (stderr, "Couldn't open file: %s\n", filename);
		free (krad_kwcap_player);
		exit (1);
	}

	krad_kwcap_player->width = krad_kwcap_player->decoder->width;
	krad_kwcap_player->height = krad_kwcap_player->decoder->height;

	printf ("Krad WCAP Player \n");
	printf ("Playing: %s\n", filename);
	printf ("Size: %dx%d\n", krad_kwcap_player->width, krad_kwcap_player->height);

	krad_kwcap_player->krad_x11 = krad_x11_create ();

	krad_x11_create_glx_window (krad_kwcap_player->krad_x11, "Krad WCAP Player", 
								krad_kwcap_player->width, krad_kwcap_player->height, &krad_kwcap_player->shutdown);

	krad_kwcap_player->buffer = krad_kwcap_player->krad_x11->pixels;

	kwcap_decoder_set_external_buffer (krad_kwcap_player->decoder, krad_kwcap_player->buffer);

	krad_kwcap_player->krad_gui = kradgui_create_with_external_surface (krad_kwcap_player->width,
																	   krad_kwcap_player->height,
													  (unsigned char *)krad_kwcap_player->buffer);



	kwcap_decoder_get_frame (krad_kwcap_player->decoder);

	krad_kwcap_player->start_time = krad_kwcap_player->decoder->msecs;


	krad_kwcap_player->krad_gui->clear = 0;
	//kradgui_go_live (krad_kwcap_player->krad_gui);

	//krad_wayland_set_frame_callback (krad_kwcap_player->krad_wayland, krad_kwcap_player_render_callback, krad_kwcap_player);

	//krad_wayland_open_window (krad_kwcap_player->krad_wayland);

	while ((krad_kwcap_player_render_callback (krad_kwcap_player, 0) > -1) &&
		   (krad_kwcap_player->shutdown != 1)) {
		krad_x11_glx_render (krad_kwcap_player->krad_x11);
	}
	
	usleep (100000);
	
	krad_x11_destroy_glx_window (krad_kwcap_player->krad_x11);

	krad_x11_destroy (krad_kwcap_player->krad_x11);

	kradgui_destroy (krad_kwcap_player->krad_gui);

	kwcap_decoder_destroy (krad_kwcap_player->decoder);

	free (krad_kwcap_player);

}

int main (int argc, char *argv[]) {

	krad_system_init ();

	if (argc == 2) {
		krad_kwcap_player_play (argv[1]);
	} else {
		printf ("Need filename\n");
	}

	printf ("Clean Exit\n");

    return 0;
}
