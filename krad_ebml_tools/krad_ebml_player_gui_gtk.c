#include <krad_ebml_player.h>

#include "krad_gui.h"
#include "krad_gui_gtk.h"


void help () {

	printf("Hi. Welcome to krad ebml player cmd line edition\n");
	printf("krad_ebml_player_cmd opts filename.krad\n");
	printf("-a for alsa, -j for jack and -p for pulse [default]\n");
	exit(0);

}

int main (int argc, char *argv[]) {


	kradgui_t *kradgui;

	krad_ebml_player_t *krad_ebml_player;
	
	int c;
	char *filename = NULL;
	
	krad_audio_api_t krad_audio_api = PULSE;
     
	while ((c = getopt (argc, argv, "ajp")) != -1) {
		switch (c) {
			case 'a':
				krad_audio_api = ALSA;
				break;
			case 'j':
				krad_audio_api = JACK;
				break;
			case 'p':
				krad_audio_api = PULSE;
				break;
			default:			
				break;
		}
	}
	
	if (optind < argc) {
         filename = argv[optind];
    } else {
    	help();
    }

	krad_ebml_player = krad_ebml_player_create(krad_audio_api);

	kradgui = kradgui_create(640, 360);

	kradgui_add_item(kradgui, REEL_TO_REEL);
	kradgui_add_item(kradgui, PLAYBACK_STATE_STATUS);
	kradgui_set_total_track_time_ms(kradgui, 5 * 60 * 1000);

	kradgui_set_control_speed_down_callback(kradgui, krad_ebml_player_speed_down);
	kradgui_set_control_speed_up_callback(kradgui, krad_ebml_player_speed_up);
	kradgui_set_control_speed_callback(kradgui, krad_ebml_player_set_speed);
	kradgui_set_callback_pointer(kradgui, krad_ebml_player);


	kradgui_gtk_start(kradgui);
	

	printf("hi!\n");

	//krad_ebml_player_play_stream(krad_ebml_player, "blah");
	krad_ebml_player_play_file(krad_ebml_player, filename);
	
	printf("Playing %s: %s\n", krad_ebml_player_get_input_type(krad_ebml_player), krad_ebml_player_get_input_name(krad_ebml_player));
	
	while (krad_ebml_player_get_state(krad_ebml_player)) {

		kradgui_set_current_track_time_ms(kradgui, (int)krad_ebml_player_get_playback_time_ms(krad_ebml_player));

		usleep(50000);
	}
	
	kradgui_gtk_end(kradgui);

	krad_ebml_player_destroy(krad_ebml_player);

    return 0;
}
