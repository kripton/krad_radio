#include "krad_ipc_client.h"

int main (int argc, char *argv[]) {

	kr_client_t *kr;
	kr_videoport_t *kr_videoport;

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

	kr_videoport = kr_videoport_create (kr);

	if (kr_videoport != NULL) {
		printf ("i worked real good\n");
	}
	

	
	kr_videoport_destroy (kr_videoport);

	kr_disconnect (kr);

	return 0;
	
}
