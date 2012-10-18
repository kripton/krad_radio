#include "krad_ipc_client.h"
#include "krad_tone.h"

krad_tone_t *krad_tone;

int audioport_process (uint32_t nframes, void *arg) {

	float *buffer;
	kr_audioport_t *kr_audioport;
	
	kr_audioport = (kr_audioport_t *)arg;
	
	buffer = kr_audioport_get_buffer (kr_audioport);
	
	krad_tone_run (krad_tone, buffer, nframes);

	return 0;

}

int main (int argc, char *argv[]) {

	kr_client_t *kr;
	kr_audioport_t *kr_audioport;

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

	krad_tone = krad_tone_create(48000);
	krad_tone_add_preset (krad_tone, "3");

	kr_audioport = kr_audioport_create (kr);

	if (kr_audioport != NULL) {
		printf ("i worked real good\n");
	}
	
	kr_audioport_set_callback (kr_audioport, audioport_process, kr_audioport);
	
	kr_audioport_activate (kr_audioport);
	
	usleep (8000000);
	
	kr_audioport_deactivate (kr_audioport);
	
	kr_audioport_destroy (kr_audioport);

	krad_tone_destroy (krad_tone);

	kr_disconnect (kr);

	return 0;
	
}
