#include "krad_radio.h"


int main (int argc, char *argv[]) {
	
	if (argc != 2) {
		printf ("Usage: %s [station callsign or config file path]\n", argv[0]);
		exit (1);
	} else {
		krad_radio ( argv[1] );
	}

	return 0;

}
