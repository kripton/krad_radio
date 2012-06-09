#include "krad_gui.h"
#include "krad_wayland.h"
#include "krad_system.h"

#include "wcap-decode.h"

typedef struct krad_wcap_player_St krad_wcap_player_t;

struct krad_wcap_player_St {

	kradgui_t *krad_gui;
	krad_wayland_t *krad_wayland;
	struct wcap_decoder *decoder;

	int width;
	int height;
	
};

static void krad_wcap_player_play (char *filename);

int krad_wcap_player_render_callback (void *buffer, void *pointer) {

	krad_wcap_player_t *krad_wcap_player;
	int updated;

	krad_wcap_player = (krad_wcap_player_t *)pointer;
	updated = 0;

	if (wcap_decoder_get_frame (krad_wcap_player->decoder)) {
		kradgui_render (krad_wcap_player->krad_gui);
		//kradgui_render_meter ( krad_wcap_player->krad_gui, krad_wcap_player->x, 
		//						 krad_wcap_player->y, krad_wcap_player->size,
		//						 krad_wcap_player->cpu_usage );
		
		memcpy (buffer, krad_wcap_player->krad_gui->data, krad_wcap_player->width * krad_wcap_player->height * 4);
		updated = 1;
	} else {
		krad_wcap_player->krad_wayland->running = 0;
	}

	return updated;
}


static void krad_wcap_player_play (char *filename) {

	krad_wcap_player_t *krad_wcap_player;

	krad_wcap_player = calloc (1, sizeof (krad_wcap_player_t));

	krad_wcap_player->decoder = wcap_decoder_create (filename);

	krad_wcap_player->width = krad_wcap_player->decoder->width;
	krad_wcap_player->height = krad_wcap_player->decoder->height;

	wcap_decoder_get_frame (krad_wcap_player->decoder);

	krad_wcap_player->krad_gui = kradgui_create_with_external_surface (krad_wcap_player->width,
																	   krad_wcap_player->height,
													  (unsigned char *)krad_wcap_player->decoder->frame);

	krad_wcap_player->krad_gui->clear = 0;
	kradgui_go_live (krad_wcap_player->krad_gui);
	krad_wcap_player->krad_wayland = krad_wayland_create ();

	krad_wayland_set_frame_callback (krad_wcap_player->krad_wayland, krad_wcap_player_render_callback, krad_wcap_player);
	krad_wayland_open_window (krad_wcap_player->krad_wayland, krad_wcap_player->width, krad_wcap_player->height);

	while (krad_wcap_player->krad_wayland->running == 1) {
		usleep (100000);
	}
	
	usleep (100000);
	
	krad_wayland_destroy (krad_wcap_player->krad_wayland);

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
