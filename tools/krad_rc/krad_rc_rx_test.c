#include "krad_rc_rx.h"





int main ( int argc, char *argv[] ) {


	krad_rc_t *krad_rc;
	krad_rc_rx_t *krad_rc_rx;
	
	krad_rc = krad_rc_create ("Pololu Maestro", "/dev/ttyACM0");
	
	if (krad_rc == NULL) {
		printf ("krad rc was null :/\n");
		return 1;
	}
	

	krad_rc_rx = krad_rc_rx_create ( krad_rc, atoi(argv[1]) );

	krad_rc_rx_run ( krad_rc_rx );
	
	krad_rc_rx_destroy ( krad_rc_rx);
	
	krad_rc_destroy ( krad_rc );



	return 0;

}

