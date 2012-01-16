#include "krad_gui.h"
#include "krad_gui_gtk.h"

int main (int argc, char *argv[]) {


	kradgui_t *kradgui;

	int count = 0;

	kradgui = kradgui_create(0, 0);

	kradgui_add_item(kradgui, REEL_TO_REEL);
	kradgui_add_item(kradgui, PLAYBACK_STATE_STATUS);
	kradgui_set_total_track_time_ms(kradgui, 5 * 60 * 1000);

	kradgui_gtk_start(kradgui);
	

	printf("hi!\n");
	
	while (count < 55) {

		kradgui_add_current_track_time_ms(kradgui, 6 *  5);
	
		if (kradgui->current_track_time_ms >= kradgui->total_track_time_ms) {
			kradgui_set_current_track_time_ms(kradgui, 0);
		}
	
		count++;

		usleep(50000);
	}
	
	kradgui_gtk_end(kradgui);


    return 0;
}

