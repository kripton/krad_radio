#include "krad_xpdr.h"

extern int kxpdr_shutdown;


int main (int argc, char *argv[]) {

	kxpdr_t *kxpdr;

	kxpdr_shutdown = 0;
	
	kxpdr = NULL;


	if (argc == 1) {
		kxpdr = kxpdr_create(DEFAULT_RECEIVER_PORT, DEFAULT_TRANSMITTER_PORT);
	} else if (argc == 2) {
		kxpdr = kxpdr_create(argv[1], DEFAULT_TRANSMITTER_PORT);
	} else if (argc == 3) {
		kxpdr = kxpdr_create(argv[1], argv[2]);
	}
  
	if (kxpdr != NULL) {
		kxpdr_destroy(kxpdr);
	}
	
	return 0;

}
