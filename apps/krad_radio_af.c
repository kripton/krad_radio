#include "krad_ipc_client.h"

int main (int argc, char *argv[]) {

	krad_ipc_client_t *client;
	kr_shm_t *kr_shm;

	if (argc == 2) {
	
		client = krad_ipc_connect (argv[1]);
	
		if (client != NULL) {
	
			kr_shm = kr_shm_create (client);
		
			if (kr_shm != NULL) {
				printf ("i worked real good\n");
			}
			
			kr_shm_destroy (kr_shm);
		
			krad_ipc_disconnect (client);
		} else {
			fprintf (stderr, "Could not connect to %s krad radio daemon\n", argv[1]);
			return 1;
		}
	}

	return 0;
	
}
