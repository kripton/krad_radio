#include "krad_radio_client.h"
#include "krad_tone.h"

krad_tone_t *krad_tone;
krad_tone_t *krad_tone2;

int audioport_process (uint32_t nframes, void *arg) {

	int s;
	float *buffer;
	kr_audioport_t *kr_audioport;
	
	kr_audioport = (kr_audioport_t *)arg;
	
	buffer = kr_audioport_get_buffer (kr_audioport, 0);
	if (kr_audioport->direction == INPUT) {
		krad_tone_run (krad_tone, buffer, nframes);
		buffer = kr_audioport_get_buffer (kr_audioport, 1);
		krad_tone_run (krad_tone2, buffer, nframes);
	} else {
		for (s = 0; s < 1600; s++) {
			if (buffer[s] > 0.3) {
				printf("signal!\n");
				break;
			}
		}
	}
	
	return 0;

}

int main (int argc, char *argv[]) {

	kr_client_t *kr;
	kr_audioport_t *kr_audioport;
	krad_mixer_portgroup_direction_t direction;

	direction = INPUT;

	if (argc != 2) {
		if (argc > 2) {
			fprintf (stderr, "Only takes station argument.\n");
		} else {
			fprintf (stderr, "No station specified.\n");
		}
		return 1;
	}
		
	kr = kr_connect (argv[1]);
	
	if (kr == NULL) {
		fprintf (stderr, "Could not connect to %s krad radio daemon.\n", argv[1]);
		return 1;
	}	

	if (direction == INPUT) {
		krad_tone = krad_tone_create(48000);
		krad_tone_add_preset (krad_tone, "3");
		krad_tone2 = krad_tone_create(48000);
		krad_tone_add_preset (krad_tone2, "3");
	}

	kr_audioport = kr_audioport_create (kr, direction);

	if (kr_audioport != NULL) {
		printf ("i worked real good\n");
	}
	
	kr_audioport_set_callback (kr_audioport, audioport_process, kr_audioport);
	
	kr_audioport_activate (kr_audioport);
	
	usleep (8000000);
	
	kr_audioport_deactivate (kr_audioport);
	
	kr_audioport_destroy (kr_audioport);

	if (direction == INPUT) {
		krad_tone_destroy (krad_tone);
		krad_tone_destroy (krad_tone2);
	}
	
	kr_disconnect (kr);

	return 0;
	
}
